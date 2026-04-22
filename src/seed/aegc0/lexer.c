#include "lexer.h"

#include <ctype.h>
#include <string.h>

static SrcLoc loc_here(Lexer* lx) {
  SrcLoc l = {lx->line, lx->col};
  return l;
}

static char peek(Lexer* lx) {
  if (lx->i >= lx->len) return 0;
  return lx->src[lx->i];
}

static char peek2(Lexer* lx) {
  if (lx->i + 1 >= lx->len) return 0;
  return lx->src[lx->i + 1];
}

static char bump(Lexer* lx) {
  char c = peek(lx);
  if (!c) return 0;
  lx->i++;
  if (c == '\n') {
    lx->line++;
    lx->col = 1;
  } else {
    lx->col++;
  }
  return c;
}

static void skip_ws_and_comments(Lexer* lx) {
  for (;;) {
    char c = peek(lx);
    if (c == 0) return;
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      bump(lx);
      continue;
    }
    if (c == '/' && peek2(lx) == '/') {
      bump(lx);
      bump(lx);
      while (peek(lx) && peek(lx) != '\n') bump(lx);
      continue;
    }
    return;
  }
}

void lexer_init(Lexer* lx, const char* src, size_t len) {
  lx->src = src;
  lx->len = len;
  lx->i = 0;
  lx->line = 1;
  lx->col = 1;
}

static Token make_tok(Lexer* lx, TokenKind k, SrcLoc start, SrcLoc end, const char* s, size_t n) {
  Token t;
  t.kind = k;
  t.span.start = start;
  t.span.end = end;
  t.text.ptr = s;
  t.text.len = n;
  t.int_value = 0;
  return t;
}

static bool is_ident_start(char c) { return (c == '_') || isalpha((unsigned char)c); }
static bool is_ident_continue(char c) { return (c == '_') || isalnum((unsigned char)c); }

static TokenKind kw_kind(Str s) {
  if (str_eq(s, str_from_c("fn"))) return TK_KW_FN;
  if (str_eq(s, str_from_c("let"))) return TK_KW_LET;
  if (str_eq(s, str_from_c("extern"))) return TK_KW_EXTERN;
  if (str_eq(s, str_from_c("return"))) return TK_KW_RETURN;
  if (str_eq(s, str_from_c("if"))) return TK_KW_IF;
  if (str_eq(s, str_from_c("else"))) return TK_KW_ELSE;
  if (str_eq(s, str_from_c("while"))) return TK_KW_WHILE;
  return TK_IDENT;
}

Token lexer_next(Lexer* lx) {
  skip_ws_and_comments(lx);
  SrcLoc start = loc_here(lx);
  char c = peek(lx);
  if (!c) return make_tok(lx, TK_EOF, start, start, lx->src + lx->i, 0);

  // punct / ops
  if (c == '(') { bump(lx); return make_tok(lx, TK_LPAREN, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == ')') { bump(lx); return make_tok(lx, TK_RPAREN, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '{') { bump(lx); return make_tok(lx, TK_LBRACE, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '}') { bump(lx); return make_tok(lx, TK_RBRACE, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == ':') { bump(lx); return make_tok(lx, TK_COLON, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == ';') { bump(lx); return make_tok(lx, TK_SEMI, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == ',') { bump(lx); return make_tok(lx, TK_COMMA, start, loc_here(lx), lx->src + lx->i - 1, 1); }

  if (c == '-' && peek2(lx) == '>') {
    bump(lx); bump(lx);
    return make_tok(lx, TK_ARROW, start, loc_here(lx), lx->src + lx->i - 2, 2);
  }

  if (c == '=' && peek2(lx) == '=') { bump(lx); bump(lx); return make_tok(lx, TK_EQEQ, start, loc_here(lx), lx->src + lx->i - 2, 2); }
  if (c == '!' && peek2(lx) == '=') { bump(lx); bump(lx); return make_tok(lx, TK_NEQ, start, loc_here(lx), lx->src + lx->i - 2, 2); }
  if (c == '<' && peek2(lx) == '=') { bump(lx); bump(lx); return make_tok(lx, TK_LTE, start, loc_here(lx), lx->src + lx->i - 2, 2); }
  if (c == '>' && peek2(lx) == '=') { bump(lx); bump(lx); return make_tok(lx, TK_GTE, start, loc_here(lx), lx->src + lx->i - 2, 2); }
  if (c == '&' && peek2(lx) == '&') { bump(lx); bump(lx); return make_tok(lx, TK_ANDAND, start, loc_here(lx), lx->src + lx->i - 2, 2); }
  if (c == '|' && peek2(lx) == '|') { bump(lx); bump(lx); return make_tok(lx, TK_OROR, start, loc_here(lx), lx->src + lx->i - 2, 2); }

  if (c == '=') { bump(lx); return make_tok(lx, TK_ASSIGN, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '+') { bump(lx); return make_tok(lx, TK_PLUS, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '-') { bump(lx); return make_tok(lx, TK_MINUS, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '*') { bump(lx); return make_tok(lx, TK_STAR, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '/') { bump(lx); return make_tok(lx, TK_SLASH, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '%') { bump(lx); return make_tok(lx, TK_PERCENT, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '<') { bump(lx); return make_tok(lx, TK_LT, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '>') { bump(lx); return make_tok(lx, TK_GT, start, loc_here(lx), lx->src + lx->i - 1, 1); }
  if (c == '!') { bump(lx); return make_tok(lx, TK_BANG, start, loc_here(lx), lx->src + lx->i - 1, 1); }

  // string
  if (c == '"') {
    bump(lx);
    size_t start_i = lx->i;
    while (peek(lx) && peek(lx) != '"') {
      if (peek(lx) == '\\' && peek2(lx)) {
        bump(lx);
        bump(lx);
        continue;
      }
      bump(lx);
    }
    size_t end_i = lx->i;
    if (peek(lx) == '"') bump(lx);
    Token t = make_tok(lx, TK_STRING, start, loc_here(lx), lx->src + start_i, end_i - start_i);
    return t;
  }

  // ident / kw
  if (is_ident_start(c)) {
    size_t start_i = lx->i;
    bump(lx);
    while (is_ident_continue(peek(lx))) bump(lx);
    size_t end_i = lx->i;
    Str s = {lx->src + start_i, end_i - start_i};
    Token t = make_tok(lx, kw_kind(s), start, loc_here(lx), s.ptr, s.len);
    return t;
  }

  // int
  if (isdigit((unsigned char)c)) {
    size_t start_i = lx->i;
    bump(lx);
    while (isdigit((unsigned char)peek(lx))) bump(lx);
    size_t end_i = lx->i;
    uint64_t v = 0;
    for (size_t j = start_i; j < end_i; j++) v = v * 10 + (uint64_t)(lx->src[j] - '0');
    Token t = make_tok(lx, TK_INT, start, loc_here(lx), lx->src + start_i, end_i - start_i);
    t.int_value = v;
    return t;
  }

  bump(lx);
  return make_tok(lx, TK_EOF, start, loc_here(lx), lx->src + lx->i - 1, 1);
}
