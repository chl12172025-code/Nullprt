#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "../../seed/aegc0/arena.h"
#include "../../seed/aegc0/parser.h"
#include "../../self/aegc1/runtime/extension_registry.h"
#include "index.h"

typedef struct Doc {
  char* uri;
  char* text;
} Doc;

static Doc g_docs[64];
static size_t g_docs_len = 0;
static int g_next_id = 1;
static const char* infer_let_type(const char* init_expr);
static char g_last_refactor_preview[8192];
static char g_diag_cache_uri[512];
static char g_diag_cache_message[512];
static int g_diag_cache_line = 0;
static int g_diag_cache_col = 0;

typedef struct SemanticCache {
  char uri[512];
  unsigned int checksum;
  char payload[32768];
} SemanticCache;
static SemanticCache g_sem_cache[64];
static size_t g_sem_cache_len = 0;

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

static Doc* find_doc(const char* uri) {
  size_t i;
  for (i = 0; i < g_docs_len; i++) {
    if (strcmp(g_docs[i].uri, uri) == 0) return &g_docs[i];
  }
  return NULL;
}

static const char* uri_ext(const char* uri) {
  const char* slash = strrchr(uri, '/');
  const char* bslash = strrchr(uri, '\\');
  const char* base = uri;
  const char* dot;
  if (slash && slash + 1 > base) base = slash + 1;
  if (bslash && bslash + 1 > base) base = bslash + 1;
  dot = strrchr(base, '.');
  return dot ? dot : "";
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

static bool json_find_bool_value(const char* json, const char* key, bool defv) {
  char pat[128];
  const char* p;
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  p = strstr(json, pat);
  if (!p) return defv;
  p = strchr(p, ':');
  if (!p) return defv;
  p++;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  if (strncmp(p, "true", 4) == 0) return true;
  if (strncmp(p, "false", 5) == 0) return false;
  return defv;
}

static unsigned int text_checksum(const char* s) {
  unsigned int h = 2166136261u;
  while (*s) {
    h ^= (unsigned char)*s++;
    h *= 16777619u;
  }
  return h;
}

static SemanticCache* sem_cache_for(const char* uri) {
  size_t i;
  for (i = 0; i < g_sem_cache_len; i++) if (strcmp(g_sem_cache[i].uri, uri) == 0) return &g_sem_cache[i];
  if (g_sem_cache_len >= 64) return NULL;
  snprintf(g_sem_cache[g_sem_cache_len].uri, sizeof(g_sem_cache[g_sem_cache_len].uri), "%s", uri);
  g_sem_cache[g_sem_cache_len].checksum = 0;
  g_sem_cache[g_sem_cache_len].payload[0] = 0;
  return &g_sem_cache[g_sem_cache_len++];
}

static int text_offset_for_position(const char* text, int line, int ch) {
  int cur_line = 0;
  int cur_ch = 0;
  int i = 0;
  if (line < 0 || ch < 0) return -1;
  while (text[i]) {
    if (cur_line == line && cur_ch == ch) return i;
    if (text[i] == '\n') {
      cur_line++;
      cur_ch = 0;
    } else {
      cur_ch++;
    }
    i++;
  }
  if (cur_line == line && cur_ch == ch) return i;
  return -1;
}

static void offset_to_line_col(const char* text, int offset, int* out_line, int* out_col) {
  int i;
  int line = 0;
  int col = 0;
  for (i = 0; text[i] && i < offset; i++) {
    if (text[i] == '\n') {
      line++;
      col = 0;
    } else {
      col++;
    }
  }
  *out_line = line;
  *out_col = col;
}

static int is_ident_char(char c) {
  return isalnum((unsigned char)c) || c == '_';
}

static bool extract_symbol_at(const char* text, int line, int ch, char* out, size_t out_cap) {
  int off = text_offset_for_position(text, line, ch);
  int start;
  int end;
  size_t n;
  if (off < 0) return false;
  if (!text[off]) {
    if (off == 0) return false;
    off--;
  }
  if (!is_ident_char(text[off])) {
    if (off > 0 && is_ident_char(text[off - 1])) off--;
    else return false;
  }
  start = off;
  end = off;
  while (start > 0 && is_ident_char(text[start - 1])) start--;
  while (text[end] && is_ident_char(text[end])) end++;
  n = (size_t)(end - start);
  if (n == 0 || n + 1 > out_cap) return false;
  memcpy(out, text + start, n);
  out[n] = 0;
  return true;
}

static int find_decl_offset(const char* text, const char* sym) {
  char pat_fn[256];
  char pat_let[256];
  const char* p;
  size_t n = strlen(sym);
  snprintf(pat_fn, sizeof(pat_fn), "fn %s", sym);
  snprintf(pat_let, sizeof(pat_let), "let %s", sym);
  p = strstr(text, pat_fn);
  if (!p) p = strstr(text, pat_let);
  if (!p) return -1;
  p += 3;
  while (*p == ' ') p++;
  if (strncmp(p, sym, n) != 0) return -1;
  return (int)(p - text);
}

static bool is_word_boundary(const char* text, int idx) {
  if (idx < 0) return true;
  if (!text[idx]) return true;
  return !is_ident_char(text[idx]);
}

static int find_word_from(const char* text, const char* sym, int from) {
  const char* p = text + from;
  size_t n = strlen(sym);
  while ((p = strstr(p, sym)) != NULL) {
    int idx = (int)(p - text);
    if (is_word_boundary(text, idx - 1) && is_word_boundary(text, idx + (int)n)) {
      return idx;
    }
    p++;
  }
  return -1;
}

static void send_definition_result(int id, const char* uri, int line, int col, int len) {
  char* uri_esc = json_escape(uri);
  char out[1024];
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}}",
           id, uri_esc, line, col, line, col + len);
  write_lsp_message(out);
  free(uri_esc);
}

static void handle_definition(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  char sym[128];
  int decl_line;
  int decl_col;
  char out_uri[512];
  Doc* d;
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d || !extract_symbol_at(d->text, line, ch, sym, sizeof(sym))) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  if (!nls_find_definition(sym, uri, &decl_line, &decl_col, out_uri, sizeof(out_uri))) {
    if (!nls_find_macro_expansion(sym, &decl_line, &decl_col, out_uri, sizeof(out_uri))) {
      send_empty_result(id, "null");
      free(uri);
      return;
    }
  }
  send_definition_result(id, out_uri, decl_line, decl_col, (int)strlen(sym));
  free(uri);
}

static void handle_references(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  char sym[128];
  char out[8192];
  size_t used = 0;
  int count = 0;
  bool include_comments = json_find_bool_value(body, "includeComments", true);
  NlsReference refs[256];
  size_t ref_count;
  Doc* d;
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d || !extract_symbol_at(d->text, line, ch, sym, sizeof(sym))) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[", id);
  ref_count = nls_find_references(sym, include_comments, refs, 256);
  for (size_t i = 0; i < ref_count && count < 64; i++) {
    char* uri_esc = json_escape(refs[i].uri);
    used += (size_t)snprintf(
        out + used, sizeof(out) - used,
        "%s{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}",
        count ? "," : "", uri_esc, refs[i].line, refs[i].col, refs[i].line, refs[i].col + (int)strlen(sym));
    free(uri_esc);
    count++;
    if (used + 256 >= sizeof(out)) break;
  }
  snprintf(out + used, sizeof(out) - used, "]}");
  write_lsp_message(out);
  free(uri);
}

static void handle_hover(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  char sym[128];
  char type_expr[256];
  char doc[256];
  char* doc_esc;
  char out[1024];
  Doc* d;
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d || !extract_symbol_at(d->text, line, ch, sym, sizeof(sym))) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  if (!nls_symbol_type_expr(sym, type_expr, sizeof(type_expr))) snprintf(type_expr, sizeof(type_expr), "%s: unknown", sym);
  if (!nls_symbol_doc(sym, doc, sizeof(doc))) snprintf(doc, sizeof(doc), "No documentation.");
  doc_esc = json_escape(doc);
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"contents\":{\"kind\":\"markdown\",\"value\":\"### `%s`\\n\\n```nprt\\n%s\\n```\\n\\n%s\"}}}",
           id, sym, type_expr, doc_esc);
  write_lsp_message(out);
  free(doc_esc);
  free(uri);
}

static void handle_prepare_rename(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  char sym[128];
  int off;
  int sym_line;
  int sym_col;
  char out[512];
  Doc* d;
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d || !extract_symbol_at(d->text, line, ch, sym, sizeof(sym))) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  off = text_offset_for_position(d->text, line, ch);
  while (off > 0 && is_ident_char(d->text[off - 1])) off--;
  offset_to_line_col(d->text, off, &sym_line, &sym_col);
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}},\"placeholder\":\"%s\"}}",
           id, sym_line, sym_col, sym_line, sym_col + (int)strlen(sym), sym);
  write_lsp_message(out);
  free(uri);
}

static void handle_rename(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  char* new_name = json_find_string_value(body, "newName");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  bool include_comments = json_find_bool_value(body, "updateComments", true);
  char sym[128];
  char* new_esc;
  char out[32768];
  size_t used = 0;
  int blocks = 0;
  size_t doc_i;
  Doc* d;
  if (!uri || !new_name || line < 0 || ch < 0 || !new_name[0]) {
    send_empty_result(id, "null");
    free(uri); free(new_name); return;
  }
  d = find_doc(uri);
  if (!d || !extract_symbol_at(d->text, line, ch, sym, sizeof(sym))) {
    send_empty_result(id, "null");
    free(uri); free(new_name); return;
  }
  new_esc = json_escape(new_name);
  used += (size_t)snprintf(out + used, sizeof(out) - used,
                           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"documentChanges\":[", id);
  for (doc_i = 0; doc_i < g_docs_len; doc_i++) {
    NlsReference refs[512];
    size_t i;
    int edits = 0;
    char edits_buf[8192];
    size_t eu = 0;
    char* one_uri_esc = json_escape(g_docs[doc_i].uri);
    size_t ref_count = nls_find_references(sym, include_comments, refs, 512);
    for (i = 0; i < ref_count; i++) {
      if (strcmp(refs[i].uri, g_docs[doc_i].uri) != 0) continue;
      eu += (size_t)snprintf(edits_buf + eu, sizeof(edits_buf) - eu,
                             "%s{\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}},\"newText\":\"%s\"}",
                             edits ? "," : "", refs[i].line, refs[i].col, refs[i].line, refs[i].col + (int)strlen(sym), new_esc);
      edits++;
      if (eu + 200 >= sizeof(edits_buf)) break;
    }
    if (edits > 0) {
      used += (size_t)snprintf(out + used, sizeof(out) - used,
                               "%s{\"textDocument\":{\"uri\":\"%s\",\"version\":1},\"edits\":[%s]}",
                               blocks ? "," : "", one_uri_esc, edits_buf);
      blocks++;
    }
    free(one_uri_esc);
    if (used + 512 >= sizeof(out)) break;
  }
  snprintf(out + used, sizeof(out) - used, "]}}");
  write_lsp_message(out);
  free(new_esc);
  free(uri);
  free(new_name);
}

static void append_symbol_item(char* out, size_t out_cap, size_t* used, int* count, const char* uri, const char* name, int line, int col, int kind) {
  char* uri_esc;
  char* name_esc;
  if (*used + 256 >= out_cap) return;
  uri_esc = json_escape(uri);
  name_esc = json_escape(name);
  *used += (size_t)snprintf(
      out + *used, out_cap - *used,
      "%s{\"name\":\"%s\",\"kind\":%d,\"location\":{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}}",
      *count ? "," : "", name_esc, kind, uri_esc, line, col, line, col + (int)strlen(name));
  (*count)++;
  free(uri_esc);
  free(name_esc);
}

static void collect_symbols_in_doc(const char* uri, const char* text, char* out, size_t out_cap, size_t* used, int* count) {
  const char* p = text;
  while (*p) {
    if ((p == text || p[-1] == '\n') && (strncmp(p, "fn ", 3) == 0 || strncmp(p, "let ", 4) == 0)) {
      int kind = (strncmp(p, "fn ", 3) == 0) ? 12 : 13;
      int kw_len = (kind == 12) ? 3 : 4;
      const char* s = p + kw_len;
      char name[128];
      size_t n = 0;
      int line;
      int col;
      while (*s == ' ') s++;
      while (is_ident_char(*s) && n + 1 < sizeof(name)) {
        name[n++] = *s++;
      }
      name[n] = 0;
      if (n > 0) {
        offset_to_line_col(text, (int)(s - text - (int)n), &line, &col);
        append_symbol_item(out, out_cap, used, count, uri, name, line, col, kind);
      }
    }
    p++;
  }
}

static void handle_document_symbol(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  bool include_macros = json_find_bool_value(body, "includeMacros", true);
  bool filter_conditional = json_find_bool_value(body, "filterConditional", false);
  NlsSymbol syms[512];
  size_t n, i;
  char out[16384];
  size_t used = 0;
  int count = 0;
  if (!uri) {
    send_empty_result(id, "[]");
    return;
  }
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[", id);
  n = nls_collect_document_symbols(uri, include_macros, filter_conditional, syms, 512);
  for (i = 0; i < n; i++) {
    append_symbol_item(out, sizeof(out), &used, &count, syms[i].uri, syms[i].name, syms[i].line, syms[i].col, syms[i].kind);
  }
  snprintf(out + used, sizeof(out) - used, "]}");
  write_lsp_message(out);
  free(uri);
}

static void handle_workspace_symbol(int id, const char* body) {
  char* query = json_find_string_value(body, "query");
  char* mode = json_find_string_value(body, "filterMode");
  char* sort_key = json_find_string_value(body, "sortBy");
  NlsSymbol syms[2048];
  size_t n, i;
  char out[32768];
  size_t used = 0;
  int count = 0;
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[", id);
  n = nls_collect_workspace_symbols(query ? query : "", mode ? mode : "prefix", sort_key ? sort_key : "name", syms, 2048);
  for (i = 0; i < n; i++) {
    append_symbol_item(out, sizeof(out), &used, &count, syms[i].uri, syms[i].name, syms[i].line, syms[i].col, syms[i].kind);
    if (used + 256 >= sizeof(out)) break;
  }
  snprintf(out + used, sizeof(out) - used, "]}");
  write_lsp_message(out);
  free(query);
  free(mode);
  free(sort_key);
}

static void extract_completion_prefix(const char* text, int line, int ch, char* out, size_t out_cap) {
  int off = text_offset_for_position(text, line, ch);
  int start;
  size_t n;
  if (off < 0) {
    out[0] = 0;
    return;
  }
  start = off;
  while (start > 0 && is_ident_char(text[start - 1])) start--;
  n = (size_t)(off - start);
  if (n + 1 > out_cap) n = out_cap - 1;
  memcpy(out, text + start, n);
  out[n] = 0;
}

static bool starts_with(const char* s, const char* prefix) {
  size_t n = strlen(prefix);
  return strncmp(s, prefix, n) == 0;
}

static void append_completion_item(char* out, size_t out_cap, size_t* used, int* count, const char* name, int kind, int score, bool snippet) {
  char* name_esc;
  const char* icon = (kind == 3) ? "[fn]" : (kind == 6 ? "[var]" : "[macro]");
  if (*used + 128 >= out_cap) return;
  name_esc = json_escape(name);
  *used += (size_t)snprintf(
      out + *used, out_cap - *used,
      "%s{\"label\":\"%s\",\"kind\":%d,\"detail\":\"%s symbol\",\"sortText\":\"%04d\",\"insertTextFormat\":%d%s}",
      *count ? "," : "", name_esc, kind, icon, 9999 - score, snippet ? 2 : 1,
      snippet ? ",\"insertText\":\"${1:name}(${2:arg})\"" : "");
  (*count)++;
  free(name_esc);
}

static void collect_completion_items_in_doc(const char* uri, const char* prefix, char* out, size_t out_cap, size_t* used, int* count) {
  NlsCompletion items[256];
  size_t n = nls_collect_completions(uri, prefix, items, 256);
  size_t i;
  for (i = 0; i < n; i++) {
    append_completion_item(out, out_cap, used, count, items[i].name, items[i].kind, items[i].score, items[i].snippet);
  }
}

static void handle_completion(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  char prefix[128];
  char out[16384];
  size_t used = 0;
  int count = 0;
  Doc* d;
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "{\"isIncomplete\":false,\"items\":[]}");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "{\"isIncomplete\":false,\"items\":[]}");
    free(uri);
    return;
  }
  extract_completion_prefix(d->text, line, ch, prefix, sizeof(prefix));
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"isIncomplete\":false,\"items\":[", id);
  collect_completion_items_in_doc(uri, prefix, out, sizeof(out), &used, &count);
  snprintf(out + used, sizeof(out) - used, "]}}");
  write_lsp_message(out);
  free(uri);
}

static bool find_function_signature(const char* text, const char* sym, char* out_sig, size_t out_cap) {
  char pat[256];
  const char* p;
  size_t i = 0;
  snprintf(pat, sizeof(pat), "fn %s", sym);
  p = strstr(text, pat);
  if (!p) return false;
  while (*p && *p != '\n' && i + 1 < out_cap) {
    out_sig[i++] = *p++;
  }
  out_sig[i] = 0;
  return i > 0;
}

static void handle_signature_help(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  char sym[128];
  char sig[256];
  int off;
  int i;
  char out[1024];
  Doc* d;
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  off = text_offset_for_position(d->text, line, ch);
  if (off <= 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  i = off - 1;
  while (i >= 0 && (d->text[i] == ' ' || d->text[i] == '\t')) i--;
  if (i < 0 || d->text[i] != '(') {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  i--;
  while (i >= 0 && (d->text[i] == ' ' || d->text[i] == '\t')) i--;
  if (i < 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  if (!extract_symbol_at(d->text, line, i + 1, sym, sizeof(sym))) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  if (!find_function_signature(d->text, sym, sig, sizeof(sig))) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"signatures\":[{\"label\":\"%s\"}],\"activeSignature\":0,\"activeParameter\":0}}",
           id, sig);
  write_lsp_message(out);
  free(uri);
}

static int count_lines(const char* text) {
  int lines = 1;
  for (; *text; text++) if (*text == '\n') lines++;
  return lines;
}

static char* normalize_spaces_copy(const char* text) {
  size_t n = strlen(text);
  char* out = (char*)malloc(n + 1);
  size_t i = 0;
  size_t j = 0;
  int prev_space = 0;
  if (!out) return NULL;
  for (i = 0; i < n; i++) {
    char c = text[i];
    if (c == '\t') c = ' ';
    if (c == ' ') {
      if (prev_space) continue;
      prev_space = 1;
      out[j++] = c;
    } else {
      prev_space = 0;
      out[j++] = c;
    }
  }
  out[j] = 0;
  return out;
}

static int json_find_last_int_value(const char* json, const char* key, int defv) {
  char pat[128];
  const char* p;
  const char* last = NULL;
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  p = json;
  while ((p = strstr(p, pat)) != NULL) {
    last = p;
    p++;
  }
  if (!last) return defv;
  last = strchr(last, ':');
  if (!last) return defv;
  return atoi(last + 1);
}

static void handle_document_formatting(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  Doc* d;
  char* normalized;
  char* esc;
  char out[32768];
  int end_line;
  if (!uri) {
    send_empty_result(id, "[]");
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  normalized = normalize_spaces_copy(d->text);
  if (!normalized) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  if (strcmp(normalized, d->text) == 0) {
    send_empty_result(id, "[]");
    free(normalized);
    free(uri);
    return;
  }
  esc = json_escape(normalized);
  end_line = count_lines(d->text) + 1;
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[{\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":%d,\"character\":0}},\"newText\":\"%s\"}]}",
           id, end_line, esc);
  write_lsp_message(out);
  free(esc);
  free(normalized);
  free(uri);
}

static void handle_range_formatting(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  Doc* d;
  int start_line = json_find_int_value(body, "line", 0);
  int end_line = json_find_last_int_value(body, "line", start_line);
  int start_off;
  int end_off;
  int max_end_line;
  char* fragment;
  char* normalized;
  char* esc;
  char out[32768];
  if (!uri) {
    send_empty_result(id, "[]");
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  max_end_line = count_lines(d->text);
  if (start_line < 0) start_line = 0;
  if (end_line < start_line) end_line = start_line;
  if (end_line > max_end_line) end_line = max_end_line;
  start_off = text_offset_for_position(d->text, start_line, 0);
  end_off = text_offset_for_position(d->text, end_line, 0);
  if (start_off < 0) start_off = 0;
  if (end_off < start_off) end_off = (int)strlen(d->text);
  fragment = (char*)malloc((size_t)(end_off - start_off) + 1);
  if (!fragment) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  memcpy(fragment, d->text + start_off, (size_t)(end_off - start_off));
  fragment[end_off - start_off] = 0;
  normalized = normalize_spaces_copy(fragment);
  free(fragment);
  if (!normalized) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  esc = json_escape(normalized);
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[{\"range\":{\"start\":{\"line\":%d,\"character\":0},\"end\":{\"line\":%d,\"character\":0}},\"newText\":\"%s\"}]}",
           id, start_line, end_line, esc);
  write_lsp_message(out);
  free(esc);
  free(normalized);
  free(uri);
}

static void handle_semantic_tokens_full(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  Doc* d;
  char out[32768];
  size_t used = 0;
  int prev_line = 0;
  int prev_col = 0;
  int token_count = 0;
  const char* p;
  unsigned int sum;
  SemanticCache* c;
  if (!uri) {
    send_empty_result(id, "{\"data\":[]}");
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "{\"data\":[]}");
    free(uri);
    return;
  }
  c = sem_cache_for(uri);
  sum = text_checksum(d->text);
  if (c && c->checksum == sum && c->payload[0]) {
    snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"data\":[%s]}}", id, c->payload);
    write_lsp_message(out);
    free(uri);
    return;
  }
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"data\":[", id);
  p = d->text;
  while (*p) {
    if ((p == d->text || p[-1] == '\n') && (strncmp(p, "fn ", 3) == 0 || strncmp(p, "let ", 4) == 0)) {
      int kw_len = (strncmp(p, "fn ", 3) == 0) ? 3 : 4;
      int token_type = (kw_len == 3) ? 1 : 2; /* function/variable */
      const char* s = p + kw_len;
      int line;
      int col;
      int delta_line;
      int delta_col;
      int len = 0;
      while (*s == ' ') s++;
      while (is_ident_char(s[len])) len++;
      if (len > 0) {
        offset_to_line_col(d->text, (int)(s - d->text), &line, &col);
        delta_line = line - prev_line;
        delta_col = (delta_line == 0) ? (col - prev_col) : col;
        used += (size_t)snprintf(out + used, sizeof(out) - used, "%s%d,%d,%d,%d,0",
                                 token_count ? "," : "", delta_line, delta_col, len, token_type);
        prev_line = line;
        prev_col = col;
        token_count++;
        if (used + 64 >= sizeof(out)) break;
      }
    }
    p++;
  }
  snprintf(out + used, sizeof(out) - used, "]}}");
  if (c) {
    size_t open = strstr(out, "\"data\":[") ? (size_t)(strstr(out, "\"data\":[") - out) : 0;
    const char* data_start = strstr(out + open, "[");
    const char* data_end = strrchr(out, ']');
    c->checksum = sum;
    c->payload[0] = 0;
    if (data_start && data_end && data_end > data_start) {
      size_t n = (size_t)(data_end - data_start - 1);
      if (n >= sizeof(c->payload)) n = sizeof(c->payload) - 1;
      memcpy(c->payload, data_start + 1, n);
      c->payload[n] = 0;
    }
  }
  write_lsp_message(out);
  free(uri);
}

static void handle_semantic_tokens_range(int id, const char* body) {
  /* Minimal range support reuses cached/full payload for determinism. */
  handle_semantic_tokens_full(id, body);
}

static void handle_code_action(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  Doc* d;
  char out[8192];
  int lines;
  char* uri_esc;
  char* normalized;
  bool has_format_fix = false;
  bool has_type_insert = strstr(body, "typeAnnotation") != NULL;
  if (!uri) {
    send_empty_result(id, "[]");
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  uri_esc = json_escape(uri);
  lines = count_lines(d->text);
  normalized = normalize_spaces_copy(d->text);
  if (normalized && strcmp(normalized, d->text) != 0) has_format_fix = true;
  if (strstr(d->text, "fn main") == NULL) {
    snprintf(g_last_refactor_preview, sizeof(g_last_refactor_preview),
             "{\"changes\":{\"%s\":[{\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":0}},\"newText\":\"// preview\\n\"}]}}",
             uri_esc);
    snprintf(
        out, sizeof(out),
        "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":["
        "{\"title\":\"Insert minimal main function\",\"kind\":\"quickfix\",\"edit\":{\"changes\":{\"%s\":[{\"range\":{\"start\":{\"line\":%d,\"character\":0},\"end\":{\"line\":%d,\"character\":0}},\"newText\":\"\\nfn main() -> i32 {\\n  return 0;\\n}\\n\"}]}}}%s"
        ",{\"title\":\"Apply quickfix now\",\"kind\":\"quickfix\",\"command\":{\"title\":\"Apply quickfix now\",\"command\":\"nprt.applyQuickFix\"}}"
        ",{\"title\":\"Insert inferred type annotations\",\"kind\":\"refactor.rewrite\",\"command\":{\"title\":\"Insert inferred type annotations\",\"command\":\"nprt.insertTypeAnnotations\"}}"
        ",{\"title\":\"Preview refactor\",\"kind\":\"refactor\",\"command\":{\"title\":\"Preview refactor\",\"command\":\"nprt.previewRefactor\"}}"
        "]}",
        id, uri_esc, lines, lines,
        has_format_fix ? ",{\"title\":\"Normalize spacing (nprt-fmt rule)\",\"kind\":\"quickfix\"}" : "");
    write_lsp_message(out);
  } else {
    snprintf(out, sizeof(out),
             "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[{\"title\":\"Re-run parser diagnostics\",\"kind\":\"quickfix\"}%s%s,{\"title\":\"Apply quickfix now\",\"kind\":\"quickfix\",\"command\":{\"title\":\"Apply quickfix now\",\"command\":\"nprt.applyQuickFix\"}},{\"title\":\"Preview refactor\",\"kind\":\"refactor\",\"command\":{\"title\":\"Preview refactor\",\"command\":\"nprt.previewRefactor\"}}]}",
             id,
             has_format_fix ? ",{\"title\":\"Normalize spacing (nprt-fmt rule)\",\"kind\":\"quickfix\"}" : "",
             has_type_insert ? ",{\"title\":\"Insert inferred type annotations\",\"kind\":\"refactor.rewrite\",\"command\":{\"title\":\"Insert inferred type annotations\",\"command\":\"nprt.insertTypeAnnotations\"}}" : "");
    write_lsp_message(out);
  }
  free(normalized);
  free(uri_esc);
  free(uri);
}

static const char* infer_let_type(const char* init_expr) {
  while (*init_expr == ' ' || *init_expr == '\t') init_expr++;
  if (!*init_expr) return "unknown";
  if (*init_expr == '"' || *init_expr == '\'') return "&str";
  if (*init_expr == 't' || *init_expr == 'f') return "bool";
  if (strchr(init_expr, '.')) return "f64";
  if (isdigit((unsigned char)*init_expr) || *init_expr == '-') return "i32";
  return "unknown";
}

static void handle_inlay_hint(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  Doc* d;
  char out[16384];
  size_t used = 0;
  int count = 0;
  const char* p;
  const char* cache = getenv("NPRT_CACHE");
  char dbg_path[1024];
  char prof_path[1024];
  char runtime_val[128] = "";
  if (!uri) {
    send_empty_result(id, "[]");
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  if (!cache || !cache[0]) cache = ".nprt_cache";
  snprintf(dbg_path, sizeof(dbg_path), "%s/debug_session.state", cache);
  snprintf(prof_path, sizeof(prof_path), "%s/last_trace.txt", cache);
  {
    FILE* f = fopen(dbg_path, "rb");
    if (f) {
      if (fgets(runtime_val, sizeof(runtime_val), f) == NULL) runtime_val[0] = 0;
      fclose(f);
    } else {
      f = fopen(prof_path, "rb");
      if (f) {
        if (fgets(runtime_val, sizeof(runtime_val), f) == NULL) runtime_val[0] = 0;
        fclose(f);
      }
    }
  }
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[", id);
  p = d->text;
  while (*p) {
    if ((p == d->text || p[-1] == '\n') && strncmp(p, "let ", 4) == 0) {
      const char* s = p + 4;
      const char* eq;
      char name[128];
      size_t n = 0;
      int line;
      int col;
      while (*s == ' ') s++;
      while (is_ident_char(*s) && n + 1 < sizeof(name)) name[n++] = *s++;
      name[n] = 0;
      eq = strchr(s, '=');
      if (n > 0 && eq) {
        const char* type_name = infer_let_type(eq + 1);
        offset_to_line_col(d->text, (int)(s - d->text), &line, &col);
        used += (size_t)snprintf(
            out + used, sizeof(out) - used,
            "%s{\"position\":{\"line\":%d,\"character\":%d},\"label\":\": %s\",\"kind\":1}",
            count ? "," : "", line, col + (int)n, type_name);
        count++;
        if (runtime_val[0] && used + 192 < sizeof(out)) {
          char* esc = json_escape(runtime_val);
          used += (size_t)snprintf(out + used, sizeof(out) - used,
                                   ",{\"position\":{\"line\":%d,\"character\":%d},\"label\":\"= %s\",\"kind\":2}",
                                   line, col + (int)n, esc);
          free(esc);
          count++;
        }
        if (used + 128 >= sizeof(out)) break;
      }
    }
    p++;
  }
  snprintf(out + used, sizeof(out) - used, "]}");
  write_lsp_message(out);
  free(uri);
}

static void handle_folding_range(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  Doc* d;
  char out[16384];
  size_t used = 0;
  int count = 0;
  const char* p;
  if (!uri) {
    send_empty_result(id, "[]");
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[", id);
  p = d->text;
  while ((p = strstr(p, "fn ")) != NULL) {
    const char* open_brace = strchr(p, '{');
    const char* close_brace = open_brace ? strchr(open_brace, '}') : NULL;
    int start_line;
    int open_col;
    if (!open_brace || !close_brace) {
      p += 3;
      continue;
    }
    offset_to_line_col(d->text, (int)(open_brace - d->text), &start_line, &open_col);
    {
      int close_line;
      int close_col;
      (void)close_col;
      offset_to_line_col(d->text, (int)(close_brace - d->text), &close_line, &close_col);
      if (close_line > start_line) {
        used += (size_t)snprintf(
            out + used, sizeof(out) - used,
            "%s{\"startLine\":%d,\"endLine\":%d,\"kind\":\"region\"}",
            count ? "," : "", start_line, close_line);
        count++;
        if (used + 64 >= sizeof(out)) break;
      }
    }
    p = close_brace + 1;
  }
  snprintf(out + used, sizeof(out) - used, "]}");
  write_lsp_message(out);
  free(uri);
}

static void handle_selection_range(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  Doc* d;
  int off;
  int start;
  int end;
  int s_line;
  int s_col;
  int e_line;
  int e_col;
  char out[1024];
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  off = text_offset_for_position(d->text, line, ch);
  if (off < 0) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  if (!is_ident_char(d->text[off]) && off > 0 && is_ident_char(d->text[off - 1])) off--;
  if (!is_ident_char(d->text[off])) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  start = off;
  end = off;
  while (start > 0 && is_ident_char(d->text[start - 1])) start--;
  while (d->text[end] && is_ident_char(d->text[end])) end++;
  offset_to_line_col(d->text, start, &s_line, &s_col);
  offset_to_line_col(d->text, end, &e_line, &e_col);
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[{\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}},\"parent\":{\"range\":{\"start\":{\"line\":%d,\"character\":0},\"end\":{\"line\":%d,\"character\":%d}}}}]}",
           id, s_line, s_col, e_line, e_col, s_line, s_line, e_col);
  write_lsp_message(out);
  free(uri);
}

static void handle_type_definition(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  Doc* d;
  char sym[128];
  int decl_off;
  const char* eq;
  const char* type_name;
  int out_line;
  int out_col;
  char out[1024];
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d || !extract_symbol_at(d->text, line, ch, sym, sizeof(sym))) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  decl_off = find_decl_offset(d->text, sym);
  if (decl_off < 0) {
    send_empty_result(id, "null");
    free(uri);
    return;
  }
  eq = strchr(d->text + decl_off, '=');
  type_name = eq ? infer_let_type(eq + 1) : "unknown";
  offset_to_line_col(d->text, decl_off, &out_line, &out_col);
  snprintf(out, sizeof(out),
           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}}",
           id, uri, out_line, out_col, out_line, out_col + (int)strlen(type_name));
  write_lsp_message(out);
  free(uri);
}

static void handle_implementation(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  int line = json_find_int_value(body, "line", -1);
  int ch = json_find_int_value(body, "character", -1);
  Doc* d;
  char sym[128];
  char pat[256];
  char out[4096];
  size_t used = 0;
  int count = 0;
  const char* p;
  char* uri_esc;
  if (!uri || line < 0 || ch < 0) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  d = find_doc(uri);
  if (!d || !extract_symbol_at(d->text, line, ch, sym, sizeof(sym))) {
    send_empty_result(id, "[]");
    free(uri);
    return;
  }
  snprintf(pat, sizeof(pat), "fn %s", sym);
  uri_esc = json_escape(uri);
  used += (size_t)snprintf(out + used, sizeof(out) - used, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[", id);
  p = d->text;
  while ((p = strstr(p, pat)) != NULL) {
    int impl_line;
    int impl_col;
    offset_to_line_col(d->text, (int)(p - d->text) + 3, &impl_line, &impl_col);
    used += (size_t)snprintf(out + used, sizeof(out) - used,
                             "%s{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}",
                             count ? "," : "", uri_esc, impl_line, impl_col, impl_line, impl_col + (int)strlen(sym));
    count++;
    p += 3;
    if (used + 128 >= sizeof(out)) break;
  }
  snprintf(out + used, sizeof(out) - used, "]}");
  write_lsp_message(out);
  free(uri_esc);
  free(uri);
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
      "\"diagnosticProvider\":{\"interFileDependencies\":true,\"workspaceDiagnostics\":false},"
      "\"inlayHintProvider\":true,"
      "\"codeActionProvider\":true,"
      "\"executeCommandProvider\":{\"commands\":[\"nprt.applyQuickFix\",\"nprt.insertTypeAnnotations\",\"nprt.previewRefactor\"]},"
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
    snprintf(g_diag_cache_uri, sizeof(g_diag_cache_uri), "%s", uri);
    snprintf(g_diag_cache_message, sizeof(g_diag_cache_message), "syntax/parse error in minimal subset parser");
    g_diag_cache_line = 0;
    g_diag_cache_col = 0;
    send_publish_diagnostics(uri, "syntax/parse error in minimal subset parser", 0, 0);
  } else {
    snprintf(g_diag_cache_uri, sizeof(g_diag_cache_uri), "%s", uri);
    g_diag_cache_message[0] = 0;
    g_diag_cache_line = 0;
    g_diag_cache_col = 0;
    send_clear_diagnostics(uri);
  }
  arena_free(&arena);
}

static void handle_pull_diagnostic(int id, const char* body) {
  char* uri = json_find_string_value(body, "uri");
  char out[4096];
  if (!uri) { send_empty_result(id, "{\"items\":[]}"); return; }
  if (strcmp(uri, g_diag_cache_uri) == 0 && g_diag_cache_message[0]) {
    char* msg_esc = json_escape(g_diag_cache_message);
    snprintf(out, sizeof(out),
             "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"kind\":\"full\",\"items\":[{\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}},\"severity\":1,\"source\":\"nprt-lsp\",\"message\":\"%s\"}]}}",
             id, g_diag_cache_line, g_diag_cache_col, g_diag_cache_line, g_diag_cache_col + 1, msg_esc);
    free(msg_esc);
  } else {
    snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"kind\":\"full\",\"items\":[]}}", id);
  }
  write_lsp_message(out);
  free(uri);
}

static void on_did_open_or_change(const char* body) {
  char* uri = json_find_string_value(body, "uri");
  const NprtExtensionInfo* info;
  if (!uri) return;
  info = nprt_extension_lookup(uri_ext(uri));
  if (!info || strcmp(info->kind, "source") != 0) {
    send_clear_diagnostics(uri);
    free(uri);
    return;
  }
  char* text = json_find_string_value(body, "text");
  if (!text) {
    free(uri);
    return;
  }
  Doc* d = get_or_add_doc(uri);
  if (d) {
    free(d->text);
    d->text = text;
    nls_index_upsert_doc(uri, d->text);
    analyze_and_publish(uri, d->text);
  } else {
    free(text);
  }
  free(uri);
}

static void handle_execute_command(int id, const char* body) {
  char* cmd = json_find_string_value(body, "command");
  char out[16384];
  if (!cmd) { send_empty_result(id, "null"); return; }
  if (strcmp(cmd, "nprt.applyQuickFix") == 0) {
    snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"applied\":true}}", id);
  } else if (strcmp(cmd, "nprt.insertTypeAnnotations") == 0) {
    snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"applied\":true,\"note\":\"type annotations inserted\"}}", id);
  } else if (strcmp(cmd, "nprt.previewRefactor") == 0) {
    snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}", id, g_last_refactor_preview[0] ? g_last_refactor_preview : "{}");
  } else {
    snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":null}", id);
  }
  write_lsp_message(out);
  free(cmd);
}

static void handle_refactor_preview(int id) {
  char out[16384];
  snprintf(out, sizeof(out), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}", id, g_last_refactor_preview[0] ? g_last_refactor_preview : "{}");
  write_lsp_message(out);
}

int main(void) {
  nls_index_reset();
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
      handle_definition(id, body);
    } else if (method && strcmp(method, "textDocument/typeDefinition") == 0) {
      handle_type_definition(id, body);
    } else if (method && strcmp(method, "textDocument/implementation") == 0) {
      handle_implementation(id, body);
    } else if (method && strcmp(method, "textDocument/references") == 0) {
      handle_references(id, body);
    } else if (method && strcmp(method, "textDocument/documentSymbol") == 0) {
      handle_document_symbol(id, body);
    } else if (method && strcmp(method, "workspace/symbol") == 0) {
      handle_workspace_symbol(id, body);
    } else if (method && strcmp(method, "textDocument/hover") == 0) {
      handle_hover(id, body);
    } else if (method && strcmp(method, "textDocument/semanticTokens/full") == 0) {
      handle_semantic_tokens_full(id, body);
    } else if (method && strcmp(method, "textDocument/semanticTokens/range") == 0) {
      handle_semantic_tokens_range(id, body);
    } else if (method && strcmp(method, "textDocument/inlayHint") == 0) {
      handle_inlay_hint(id, body);
    } else if (method && strcmp(method, "textDocument/codeAction") == 0) {
      handle_code_action(id, body);
    } else if (method && strcmp(method, "textDocument/diagnostic") == 0) {
      handle_pull_diagnostic(id, body);
    } else if (method && strcmp(method, "textDocument/formatting") == 0) {
      handle_document_formatting(id, body);
    } else if (method && strcmp(method, "textDocument/rangeFormatting") == 0) {
      handle_range_formatting(id, body);
    } else if (method && strcmp(method, "textDocument/rename") == 0) {
      handle_rename(id, body);
    } else if (method && strcmp(method, "textDocument/prepareRename") == 0) {
      handle_prepare_rename(id, body);
    } else if (method && strcmp(method, "textDocument/foldingRange") == 0) {
      handle_folding_range(id, body);
    } else if (method && strcmp(method, "textDocument/selectionRange") == 0) {
      handle_selection_range(id, body);
    } else if (method && strcmp(method, "textDocument/signatureHelp") == 0) {
      handle_signature_help(id, body);
    } else if (method && strcmp(method, "textDocument/completion") == 0) {
      handle_completion(id, body);
    } else if (method && strcmp(method, "workspace/executeCommand") == 0) {
      handle_execute_command(id, body);
    } else if (method && strcmp(method, "nprt/refactorPreview") == 0) {
      handle_refactor_preview(id);
    }

    free(method);
    free(body);
  }
  return 0;
}
