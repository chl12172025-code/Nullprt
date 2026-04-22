#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SrcLoc {
  uint32_t line;
  uint32_t col;
} SrcLoc;

typedef struct Span {
  SrcLoc start;
  SrcLoc end;
} Span;

typedef enum DiagLevel {
  DIAG_ERROR = 1,
  DIAG_WARN = 2,
} DiagLevel;

typedef struct Diag {
  DiagLevel level;
  Span span;
  const char* message;
} Diag;

typedef struct Str {
  const char* ptr;
  size_t len;
} Str;

static inline Str str_from_c(const char* s) {
  Str out = {s, 0};
  while (s && s[out.len]) out.len++;
  return out;
}

static inline bool str_eq(Str a, Str b) {
  if (a.len != b.len) return false;
  for (size_t i = 0; i < a.len; i++) {
    if (a.ptr[i] != b.ptr[i]) return false;
  }
  return true;
}
