#include "index.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NlsDoc {
  char uri[512];
  char* text;
} NlsDoc;

static NlsDoc g_docs[128];
static size_t g_docs_len = 0;
static NlsSymbol g_symbols[4096];
static size_t g_symbols_len = 0;
static NlsReference g_refs[16384];
static size_t g_refs_len = 0;

static int is_ident(char c) {
  return isalnum((unsigned char)c) || c == '_';
}

static void offset_to_line_col(const char* text, int offset, int* out_line, int* out_col) {
  int i, line = 0, col = 0;
  for (i = 0; text[i] && i < offset; i++) {
    if (text[i] == '\n') { line++; col = 0; } else col++;
  }
  *out_line = line;
  *out_col = col;
}

static int match_mode(const char* s, const char* q, const char* mode) {
  if (!q || !q[0]) return 1;
  if (!mode || strcmp(mode, "prefix") == 0) return strncmp(s, q, strlen(q)) == 0;
  if (strcmp(mode, "contains") == 0) return strstr(s, q) != NULL;
  if (strcmp(mode, "exact") == 0) return strcmp(s, q) == 0;
  if (strcmp(mode, "fuzzy") == 0) {
    size_t i = 0, j = 0;
    while (s[i] && q[j]) { if (s[i] == q[j]) j++; i++; }
    return q[j] == 0;
  }
  return strstr(s, q) != NULL;
}

static int cmp_symbols_key(const NlsSymbol* sa, const NlsSymbol* sb, const char* key) {
  if (key && strcmp(key, "kind") == 0) return sa->kind - sb->kind;
  if (key && strcmp(key, "path") == 0) return strcmp(sa->uri, sb->uri);
  if (key && strcmp(key, "relevance") == 0) return (int)strlen(sb->name) - (int)strlen(sa->name);
  return strcmp(sa->name, sb->name);
}

void nls_index_reset(void) {
  size_t i;
  for (i = 0; i < g_docs_len; i++) {
    free(g_docs[i].text);
    g_docs[i].text = NULL;
  }
  g_docs_len = 0;
  g_symbols_len = 0;
  g_refs_len = 0;
}

static NlsDoc* get_or_add_doc(const char* uri) {
  size_t i;
  for (i = 0; i < g_docs_len; i++) if (strcmp(g_docs[i].uri, uri) == 0) return &g_docs[i];
  if (g_docs_len >= 128) return NULL;
  strncpy(g_docs[g_docs_len].uri, uri, sizeof(g_docs[g_docs_len].uri) - 1);
  g_docs[g_docs_len].uri[sizeof(g_docs[g_docs_len].uri) - 1] = 0;
  g_docs[g_docs_len].text = NULL;
  return &g_docs[g_docs_len++];
}

static void index_doc_symbols(const char* uri, const char* text) {
  const char* p = text;
  char last_doc[256] = {0};
  bool cond_off = false;
  while (*p) {
    const char* line = p;
    const char* eol = strchr(p, '\n');
    if (!eol) eol = p + strlen(p);
    if (strncmp(line, "#if false", 9) == 0) cond_off = true;
    if (strncmp(line, "#endif", 6) == 0) cond_off = false;
    if (strncmp(line, "///", 3) == 0) {
      size_t n = (size_t)(eol - line - 3);
      if (n >= sizeof(last_doc)) n = sizeof(last_doc) - 1;
      memcpy(last_doc, line + 3, n);
      last_doc[n] = 0;
    }
    if (strncmp(line, "fn ", 3) == 0 || strncmp(line, "let ", 4) == 0 || strncmp(line, "macro_rules!", 12) == 0) {
      int kw = (strncmp(line, "fn ", 3) == 0) ? 3 : (strncmp(line, "let ", 4) == 0 ? 4 : 12);
      const char* s = line + kw;
      char name[128];
      size_t n = 0;
      int off, l, c;
      while (*s == ' ' || *s == '\t' || *s == '!') s++;
      while (is_ident(*s) && n + 1 < sizeof(name)) name[n++] = *s++;
      name[n] = 0;
      if (n > 0 && g_symbols_len < 4096) {
        off = (int)(s - text - (int)n);
        offset_to_line_col(text, off, &l, &c);
        strncpy(g_symbols[g_symbols_len].uri, uri, sizeof(g_symbols[g_symbols_len].uri) - 1);
        strncpy(g_symbols[g_symbols_len].name, name, sizeof(g_symbols[g_symbols_len].name) - 1);
        strncpy(g_symbols[g_symbols_len].doc, last_doc, sizeof(g_symbols[g_symbols_len].doc) - 1);
        g_symbols[g_symbols_len].kind = (kw == 3) ? 12 : (kw == 4 ? 13 : 15);
        g_symbols[g_symbols_len].line = l;
        g_symbols[g_symbols_len].col = c;
        g_symbols[g_symbols_len].conditional_excluded = cond_off;
        g_symbols[g_symbols_len].is_macro = kw == 12;
        g_symbols_len++;
      }
      last_doc[0] = 0;
    }
    p = *eol ? eol + 1 : eol;
  }
}

static void index_doc_refs(const char* uri, const char* text) {
  int i = 0;
  while (text[i]) {
    bool in_comment = false;
    int start = i;
    if (text[i] == '/' && text[i + 1] == '/') in_comment = true;
    if (in_comment) {
      while (text[i] && text[i] != '\n') {
        if (is_ident(text[i])) {
          int s = i, line, col;
          while (is_ident(text[i])) i++;
          if (g_refs_len < 16384) {
            offset_to_line_col(text, s, &line, &col);
            strncpy(g_refs[g_refs_len].uri, uri, sizeof(g_refs[g_refs_len].uri) - 1);
            g_refs[g_refs_len].line = line;
            g_refs[g_refs_len].col = col;
            g_refs[g_refs_len].in_comment = true;
            g_refs_len++;
          }
        } else i++;
      }
      continue;
    }
    if (is_ident(text[i])) {
      int line, col;
      while (is_ident(text[i])) i++;
      if (g_refs_len < 16384) {
        offset_to_line_col(text, start, &line, &col);
        strncpy(g_refs[g_refs_len].uri, uri, sizeof(g_refs[g_refs_len].uri) - 1);
        g_refs[g_refs_len].line = line;
        g_refs[g_refs_len].col = col;
        g_refs[g_refs_len].in_comment = false;
        g_refs_len++;
      }
    } else i++;
  }
}

void nls_index_upsert_doc(const char* uri, const char* text) {
  NlsDoc* d = get_or_add_doc(uri);
  size_t i;
  if (!d) return;
  free(d->text);
  d->text = (char*)malloc(strlen(text) + 1);
  if (!d->text) return;
  strcpy(d->text, text);
  g_symbols_len = 0;
  g_refs_len = 0;
  for (i = 0; i < g_docs_len; i++) {
    index_doc_symbols(g_docs[i].uri, g_docs[i].text ? g_docs[i].text : "");
    index_doc_refs(g_docs[i].uri, g_docs[i].text ? g_docs[i].text : "");
  }
}

bool nls_find_definition(const char* symbol, const char* prefer_uri, int* out_line, int* out_col, char* out_uri, size_t out_uri_cap) {
  size_t i;
  for (i = 0; i < g_symbols_len; i++) {
    if (strcmp(g_symbols[i].name, symbol) == 0 && strcmp(g_symbols[i].uri, prefer_uri) == 0) {
      *out_line = g_symbols[i].line; *out_col = g_symbols[i].col;
      snprintf(out_uri, out_uri_cap, "%s", g_symbols[i].uri);
      return true;
    }
  }
  for (i = 0; i < g_symbols_len; i++) {
    if (strcmp(g_symbols[i].name, symbol) == 0) {
      *out_line = g_symbols[i].line; *out_col = g_symbols[i].col;
      snprintf(out_uri, out_uri_cap, "%s", g_symbols[i].uri);
      return true;
    }
  }
  return false;
}

bool nls_find_macro_expansion(const char* callsite_symbol, int* out_line, int* out_col, char* out_uri, size_t out_uri_cap) {
  char name[128];
  size_t i = 0, j;
  while (callsite_symbol[i] && callsite_symbol[i] != '!' && i + 1 < sizeof(name)) {
    name[i] = callsite_symbol[i];
    i++;
  }
  name[i] = 0;
  for (j = 0; j < g_symbols_len; j++) {
    if (g_symbols[j].is_macro && strcmp(g_symbols[j].name, name) == 0) {
      *out_line = g_symbols[j].line; *out_col = g_symbols[j].col;
      snprintf(out_uri, out_uri_cap, "%s", g_symbols[j].uri);
      return true;
    }
  }
  return false;
}

size_t nls_find_references(const char* symbol, bool include_comments, NlsReference* out, size_t cap) {
  size_t i, n = 0;
  for (i = 0; i < g_docs_len; i++) {
    const char* t = g_docs[i].text ? g_docs[i].text : "";
    const char* p = t;
    while ((p = strstr(p, symbol)) != NULL && n < cap) {
      int idx = (int)(p - t);
      int left_ok = idx == 0 || !is_ident(t[idx - 1]);
      int right_ok = !is_ident(t[idx + (int)strlen(symbol)]);
      int line, col;
      bool in_comment = false;
      int back = idx;
      while (back > 0 && t[back - 1] != '\n') back--;
      if (t[back] == '/' && t[back + 1] == '/') in_comment = true;
      if (left_ok && right_ok && (include_comments || !in_comment)) {
        offset_to_line_col(t, idx, &line, &col);
        snprintf(out[n].uri, sizeof(out[n].uri), "%s", g_docs[i].uri);
        out[n].line = line;
        out[n].col = col;
        out[n].in_comment = in_comment;
        n++;
      }
      p++;
    }
  }
  return n;
}

size_t nls_collect_document_symbols(const char* uri, bool include_macros, bool filter_conditional, NlsSymbol* out, size_t cap) {
  size_t i, n = 0;
  for (i = 0; i < g_symbols_len && n < cap; i++) {
    if (strcmp(g_symbols[i].uri, uri) != 0) continue;
    if (!include_macros && g_symbols[i].is_macro) continue;
    if (filter_conditional && g_symbols[i].conditional_excluded) continue;
    out[n++] = g_symbols[i];
  }
  return n;
}

size_t nls_collect_workspace_symbols(const char* query, const char* mode, const char* sort_key, NlsSymbol* out, size_t cap) {
  size_t i, n = 0;
  for (i = 0; i < g_symbols_len && n < cap; i++) {
    if (match_mode(g_symbols[i].name, query, mode)) out[n++] = g_symbols[i];
  }
  for (i = 0; i < n; i++) {
    size_t j;
    for (j = i + 1; j < n; j++) {
      if (cmp_symbols_key(&out[j], &out[i], sort_key) < 0) {
        NlsSymbol t = out[i];
        out[i] = out[j];
        out[j] = t;
      }
    }
  }
  return n;
}

size_t nls_collect_completions(const char* current_uri, const char* prefix, NlsCompletion* out, size_t cap) {
  size_t i, n = 0;
  for (i = 0; i < g_symbols_len && n < cap; i++) {
    if (prefix && prefix[0] && strncmp(g_symbols[i].name, prefix, strlen(prefix)) != 0) continue;
    snprintf(out[n].name, sizeof(out[n].name), "%s", g_symbols[i].name);
    out[n].kind = (g_symbols[i].kind == 12) ? 3 : 6;
    out[n].score = 100 - (int)strlen(g_symbols[i].name) + (strcmp(g_symbols[i].uri, current_uri) == 0 ? 20 : 0);
    out[n].snippet = g_symbols[i].kind == 12 || g_symbols[i].kind == 15;
    n++;
  }
  for (i = 0; i < n; i++) {
    size_t j;
    for (j = i + 1; j < n; j++) {
      if (out[j].score > out[i].score) {
        NlsCompletion t = out[i];
        out[i] = out[j];
        out[j] = t;
      }
    }
  }
  return n;
}

bool nls_symbol_doc(const char* symbol, char* out, size_t cap) {
  size_t i;
  for (i = 0; i < g_symbols_len; i++) {
    if (strcmp(g_symbols[i].name, symbol) == 0 && g_symbols[i].doc[0]) {
      snprintf(out, cap, "%s", g_symbols[i].doc);
      return true;
    }
  }
  return false;
}

bool nls_symbol_type_expr(const char* symbol, char* out, size_t cap) {
  size_t i;
  for (i = 0; i < g_symbols_len; i++) {
    if (strcmp(g_symbols[i].name, symbol) == 0) {
      if (g_symbols[i].kind == 12 || g_symbols[i].is_macro) snprintf(out, cap, "fn %s(...) -> i32", symbol);
      else snprintf(out, cap, "%s: inferred<i32>", symbol);
      return true;
    }
  }
  return false;
}
