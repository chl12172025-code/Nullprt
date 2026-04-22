#pragma once

#include "aegc0.h"

typedef enum TokenKind {
  TK_EOF = 0,
  TK_IDENT,
  TK_INT,
  TK_STRING,

  TK_KW_FN,
  TK_KW_LET,
  TK_KW_EXTERN,
  TK_KW_RETURN,
  TK_KW_IF,
  TK_KW_ELSE,
  TK_KW_WHILE,

  TK_LPAREN,
  TK_RPAREN,
  TK_LBRACE,
  TK_RBRACE,
  TK_COLON,
  TK_SEMI,
  TK_COMMA,
  TK_ARROW,

  TK_ASSIGN,

  TK_PLUS,
  TK_MINUS,
  TK_STAR,
  TK_SLASH,
  TK_PERCENT,

  TK_EQEQ,
  TK_NEQ,
  TK_LT,
  TK_LTE,
  TK_GT,
  TK_GTE,

  TK_ANDAND,
  TK_OROR,
  TK_BANG,
} TokenKind;

typedef struct Token {
  TokenKind kind;
  Span span;
  Str text;
  uint64_t int_value;
} Token;

typedef struct Lexer {
  const char* src;
  size_t len;
  size_t i;
  uint32_t line;
  uint32_t col;
} Lexer;

void lexer_init(Lexer* lx, const char* src, size_t len);
Token lexer_next(Lexer* lx);
