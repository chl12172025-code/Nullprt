#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct NdbgEvalResult {
  bool ok;
  long long value;
  char message[128];
} NdbgEvalResult;

NdbgEvalResult ndbg_eval_expr(const char* expr);
void ndbg_eval_highlight_tokens(const char* expr, char* out, size_t out_cap);
