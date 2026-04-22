#pragma once

#include "arena.h"
#include "ast.h"
#include "lexer.h"

typedef struct Parser {
  Arena* arena;
  Lexer lx;
  Token cur;
  const char* filename;
  bool had_error;
} Parser;

void parser_init(Parser* p, Arena* arena, const char* filename, const char* src, size_t len);
Program* parse_program(Parser* p);
