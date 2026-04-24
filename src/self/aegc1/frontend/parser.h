#pragma once

#include "ast.h"
#include "lexer.h"

typedef struct A1Parser {
  A1Lexer lx;
  A1Token cur;
  bool had_error;
  const char* src;
  size_t src_len;
} A1Parser;

void a1_parser_init(A1Parser* p, const char* src, size_t len);
A1AstModule a1_parse_module(A1Parser* p);
