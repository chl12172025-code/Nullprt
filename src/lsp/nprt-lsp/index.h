#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct NlsSymbol {
  char uri[512];
  char name[128];
  char doc[256];
  int kind;
  int line;
  int col;
  bool conditional_excluded;
  bool is_macro;
} NlsSymbol;

typedef struct NlsReference {
  char uri[512];
  int line;
  int col;
  bool in_comment;
} NlsReference;

typedef struct NlsCompletion {
  char name[128];
  int kind;
  int score;
  bool snippet;
} NlsCompletion;

void nls_index_reset(void);
void nls_index_upsert_doc(const char* uri, const char* text);
bool nls_find_definition(const char* symbol, const char* prefer_uri, int* out_line, int* out_col, char* out_uri, size_t out_uri_cap);
bool nls_find_macro_expansion(const char* callsite_symbol, int* out_line, int* out_col, char* out_uri, size_t out_uri_cap);
size_t nls_find_references(const char* symbol, bool include_comments, NlsReference* out, size_t cap);
size_t nls_collect_document_symbols(const char* uri, bool include_macros, bool filter_conditional, NlsSymbol* out, size_t cap);
size_t nls_collect_workspace_symbols(const char* query, const char* mode, const char* sort_key, NlsSymbol* out, size_t cap);
size_t nls_collect_completions(const char* current_uri, const char* prefix, NlsCompletion* out, size_t cap);
bool nls_symbol_doc(const char* symbol, char* out, size_t cap);
bool nls_symbol_type_expr(const char* symbol, char* out, size_t cap);
