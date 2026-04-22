#pragma once

#include "../common.h"

typedef enum A1TokenKind {
  A1_TK_EOF = 0,
  A1_TK_IDENT,
  A1_TK_INT,
  A1_TK_STRING,

  A1_TK_KW_FN,
  A1_TK_KW_LET,
  A1_TK_KW_MUT,
  A1_TK_KW_STRUCT,
  A1_TK_KW_ENUM,
  A1_TK_KW_MATCH,
  A1_TK_KW_EXTERN,
  A1_TK_KW_IMPORT,
  A1_TK_KW_IF,
  A1_TK_KW_ELSE,
  A1_TK_KW_WHILE,
  A1_TK_KW_RETURN,
  A1_TK_KW_WHERE,
  A1_TK_KW_ASYNC,
  A1_TK_KW_AWAIT,
  A1_TK_KW_CFG,

  A1_TK_LPAREN,
  A1_TK_RPAREN,
  A1_TK_LBRACE,
  A1_TK_RBRACE,
  A1_TK_LBRACK,
  A1_TK_RBRACK,
  A1_TK_LT,
  A1_TK_GT,
  A1_TK_COLON,
  A1_TK_SEMI,
  A1_TK_COMMA,
  A1_TK_DOT,
  A1_TK_ARROW,
  A1_TK_FATARROW,
  A1_TK_HASH,
  A1_TK_BANG,
  A1_TK_ASSIGN,
  A1_TK_PLUS,
  A1_TK_MINUS,
  A1_TK_STAR,
  A1_TK_SLASH,
  A1_TK_PERCENT,
  A1_TK_EQEQ,
  A1_TK_NEQ,
  A1_TK_LTE,
  A1_TK_GTE,
  A1_TK_ANDAND,
  A1_TK_OROR,
  A1_TK_PIPE,
} A1TokenKind;

typedef struct A1Token {
  A1TokenKind kind;
  A1StringView text;
  A1Span span;
} A1Token;

typedef struct A1Lexer {
  const char* src;
  size_t len;
  size_t i;
  uint32_t line;
  uint32_t col;
} A1Lexer;

void a1_lexer_init(A1Lexer* lx, const char* src, size_t len);
A1Token a1_lexer_next(A1Lexer* lx);
