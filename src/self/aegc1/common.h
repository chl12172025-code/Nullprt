#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct A1StringView {
  const char* ptr;
  size_t len;
} A1StringView;

typedef struct A1SourcePos {
  uint32_t line;
  uint32_t col;
} A1SourcePos;

typedef struct A1Span {
  A1SourcePos start;
  A1SourcePos end;
} A1Span;

typedef struct A1Diag {
  A1Span span;
  const char* message;
} A1Diag;
