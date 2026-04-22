#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../seed/aegc0/arena.h"
#include "../../seed/aegc0/parser.h"

typedef struct Doc {
  char* uri;
  char* text;
} Doc;

static Doc g_docs[64];
static size_t g_docs_len = 0;
static int g_next_id = 1;

static char* dup_s(const char* s) {
  size_t n = strlen(s);
  char* p = (char*)malloc(n + 1);
  if (!p) return NULL;
  memcpy(p, s, n + 1);
  return p;
}

static Doc* get_or_add_doc(const char* uri) {
  for (size_t i = 0; i < g_docs_len; i++) {
    if (strcmp(g_docs[i].uri, uri) == 0) return &g_docs[i];
  }
  if (g_docs_len >= 64) return NULL;
  g_docs[g_docs_len].uri = dup_s(uri);
  g_docs[g_docs_len].text = dup_s("");
  return &g_docs[g_docs_len++];
}

static char* json_escape(const char* s) {
  size_t n = strlen(s);
  char* out = (char*)malloc(n * 2 + 1);
  size_t j = 0;
  for (size_t i = 0; i < n; i++) {
    char c = s[i];
    if (c == '"' || c == '\\') out[j++] = '\\';
    if (c == '\n') { out[j++]='\\'; out[j++]='n'; continue; }
    if (c == '\r') { out[j++]='\\'; out[j++]='r'; continue; }
    if (c == '\t') { out[j++]='\\'; out[j++]='t'; continue; }
    out[j++] = c;
  }
  out[j] = 0;
  return out;
}

static void write_lsp_message(const char* json) {
  size_t n = strlen(json);
  printf("Content-Length: %zu\r\n\r\n%s", n, json);
  fflush(stdout);
}

static char* read_lsp_message(void) {
  char line[1024];
  int content_len = -1;
  while (fgets(line, sizeof(line), stdin)) {
    if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) break;
    if (_strnicmp(line, "Content-Length:", 15) == 0) {
      content_len = atoi(line + 15);
    }
  }
  if (content_len <= 0) return NULL;
  char* body = (char*)malloc((size_t)content_len + 1);
  if (!body) return NULL;
  size_t got = fread(body, 1, (size_t)content_len, stdin);
  body[got] = 0;
  return body;
}

static char* json_find_string_value(const char* json, const char* key) {
  char pat[128];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char* p = strstr(json, pat);
  if (!p) return NULL;
  p = strchr(p, ':');
  if (!p) return NULL;
  p++;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  if (*p != '"') return NULL;
  p++;
  const char* q = p;
  while (*q && *q != '"') q++;
  size_t n = (size_t)(q - p);
  char* out = (char*)malloc(n + 1);
  memcpy(out, p, n);
  out[n] = 0;
  return out;
}

static int json_find_int_value(const char* json, const char* key, int defv) {
  char pat[128];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char* p = strstr(json, pat);
  if (!p) return defv;
  p = strchr(p, ':');
  if (!p) return defv;
  p++;
  return atoi(p);
}

static void send_initialize_result(int id) {
  char buf[4096];
  snprintf(
      buf, sizeof(buf),
      "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"capabilities\":{"
      "\"textDocumentSync\":2,"
      "\"definitionProvider\":true,"
      "\"typeDefinitionProvider\":true,"
      "\"implementationProvider\":true,"
      "\"referencesProvider\":true,"
      "\"documentSymbolProvider\":true,"
      "\"workspaceSymbolProvider\":true,"
      "\"hoverProvider\":true,"
      "\"signatureHelpProvider\":{\"triggerCharacters\":[\"(\",\",\"]},"
      "\"renameProvider\":{\"prepareProvider\":true},"
      "\"foldingRangeProvider\":true,"
      "\"selectionRangeProvider\":true,"
      "\"semanticTokensProvider\":{\"full\":true,\"legend\":{\"tokenTypes\":[\"class\",\"function\",\"variable\"],\"tokenModifiers\":[\"declaration\",\"readonly\"]}},"
      "\"inlayHintProvider\":true,"
      "\"codeActionProvider\":true,"
      "\"completionProvider\":{\"resolveProvider\":false,\"triggerCharacters\":[\".\",\":\"]},"
      "\"documentFormattingProvider\":true,"
      "\"documentRangeFormattingProvider\":true"
      "}}}",
      id);
  write_lsp_message(buf);
}

static void send_empty_result(int id, const char* json_value) {
  char out[256];
  snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}", id, json_value);
  write_lsp_message(out);
}

static void send_publish_diagnostics(const char* uri, const char* message, int line0, int col0) {
  char* uri_esc = json_escape(uri);
  char* msg_esc = json_escape(message);
  char buf[4096];
  snprintf(
      buf, sizeof(buf),
      "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{"
      "\"uri\":\"%s\",\"diagnostics\":[{\"range\":{\"start\":{\"line\":%d,\"character\":%d},"
      "\"end\":{\"line\":%d,\"character\":%d}},\"severity\":1,\"source\":\"nprt-lsp\","
      "\"message\":\"%s\"}]}}",
      uri_esc, line0, col0, line0, col0 + 1, msg_esc);
  write_lsp_message(buf);
  free(uri_esc);
  free(msg_esc);
}

static void send_clear_diagnostics(const char* uri) {
  char* uri_esc = json_escape(uri);
  char buf[1024];
  snprintf(
      buf, sizeof(buf),
      "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":\"%s\",\"diagnostics\":[]}}",
      uri_esc);
  write_lsp_message(buf);
  free(uri_esc);
}

static void analyze_and_publish(const char* uri, const char* text) {
  Arena arena;
  arena_init(&arena);
  Parser p;
  parser_init(&p, &arena, uri, text, strlen(text));
  (void)parse_program(&p);
  if (p.had_error) {
    send_publish_diagnostics(uri, "syntax/parse error in minimal subset parser", 0, 0);
  } else {
    send_clear_diagnostics(uri);
  }
  arena_free(&arena);
}

static void on_did_open_or_change(const char* body) {
  char* uri = json_find_string_value(body, "uri");
  if (!uri) return;
  char* text = json_find_string_value(body, "text");
  if (!text) {
    free(uri);
    return;
  }
  Doc* d = get_or_add_doc(uri);
  if (d) {
    free(d->text);
    d->text = text;
    analyze_and_publish(uri, d->text);
  } else {
    free(text);
  }
  free(uri);
}

int main(void) {
  for (;;) {
    char* body = read_lsp_message();
    if (!body) break;

    char* method = json_find_string_value(body, "method");
    int id = json_find_int_value(body, "id", 0);

    if (method && strcmp(method, "initialize") == 0) {
      send_initialize_result(id ? id : g_next_id++);
    } else if (method && strcmp(method, "textDocument/didOpen") == 0) {
      on_did_open_or_change(body);
    } else if (method && strcmp(method, "textDocument/didChange") == 0) {
      on_did_open_or_change(body);
    } else if (method && strcmp(method, "shutdown") == 0) {
      char out[128];
      snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":null}", id);
      write_lsp_message(out);
    } else if (method && strcmp(method, "exit") == 0) {
      free(method);
      free(body);
      break;
    } else if (method && strcmp(method, "textDocument/definition") == 0) {
      send_empty_result(id, "null");
    } else if (method && strcmp(method, "textDocument/typeDefinition") == 0) {
      send_empty_result(id, "null");
    } else if (method && strcmp(method, "textDocument/implementation") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/references") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/documentSymbol") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "workspace/symbol") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/hover") == 0) {
      send_empty_result(id, "null");
    } else if (method && strcmp(method, "textDocument/semanticTokens/full") == 0) {
      send_empty_result(id, "{\"data\":[]}");
    } else if (method && strcmp(method, "textDocument/inlayHint") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/codeAction") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/formatting") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/rangeFormatting") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/rename") == 0) {
      send_empty_result(id, "null");
    } else if (method && strcmp(method, "textDocument/prepareRename") == 0) {
      send_empty_result(id, "null");
    } else if (method && strcmp(method, "textDocument/foldingRange") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/selectionRange") == 0) {
      send_empty_result(id, "[]");
    } else if (method && strcmp(method, "textDocument/signatureHelp") == 0) {
      send_empty_result(id, "null");
    } else if (method && strcmp(method, "textDocument/completion") == 0) {
      char out[128];
      snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"isIncomplete\":false,\"items\":[]}}", id);
      write_lsp_message(out);
    }

    free(method);
    free(body);
  }
  return 0;
}
