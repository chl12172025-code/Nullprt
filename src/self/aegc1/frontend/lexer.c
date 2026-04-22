#include "lexer.h"

#include <ctype.h>
#include <string.h>

static A1SourcePos pos_now(A1Lexer* lx) { A1SourcePos p = {lx->line, lx->col}; return p; }
static char p1(A1Lexer* lx) { return lx->i < lx->len ? lx->src[lx->i] : 0; }
static char p2(A1Lexer* lx) { return (lx->i + 1) < lx->len ? lx->src[lx->i + 1] : 0; }
static char bump(A1Lexer* lx) {
  char c = p1(lx);
  if (!c) return 0;
  lx->i++;
  if (c == '\n') { lx->line++; lx->col = 1; } else { lx->col++; }
  return c;
}

static bool sv_eq(A1StringView a, const char* lit) {
  size_t n = strlen(lit);
  if (a.len != n) return false;
  return memcmp(a.ptr, lit, n) == 0;
}

bool a1_is_reserved_keyword(A1StringView sv) {
  static const char* kws[] = {
    "fn","let","mut","const","static","type","struct","enum","union","trait","impl",
    "mod","use","import","export","extern","as","where","pub","crate","super","self",
    "if","else","unless","when","cond","switch","match","loop","while","for","in",
    "break","continue","return","yield","await","try","catch","finally","defer","throw",
    "panic","recover","async","channel","send","recv","select","atomic","fence",
    "unsafe","derive","macro","macro_rules","cfg","cfg_attr","test","bench","doc",
    "reflect","meta","comptime","effect","generic","associated","exists","linear","session"
  };
  for (size_t i = 0; i < sizeof(kws) / sizeof(kws[0]); i++) {
    if (sv_eq(sv, kws[i])) return true;
  }
  return false;
}

static A1Token tk(A1TokenKind kind, A1SourcePos s, A1SourcePos e, const char* p, size_t n) {
  A1Token t;
  t.kind = kind;
  t.span.start = s;
  t.span.end = e;
  t.text.ptr = p;
  t.text.len = n;
  return t;
}

static void skip_ws_comments(A1Lexer* lx) {
  for (;;) {
    char c = p1(lx);
    if (!c) return;
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { bump(lx); continue; }
    if (c == '/' && p2(lx) == '/') { while (p1(lx) && p1(lx) != '\n') bump(lx); continue; }
    if (c == '/' && p2(lx) == '*') {
      bump(lx); bump(lx);
      while (p1(lx) && !(p1(lx) == '*' && p2(lx) == '/')) bump(lx);
      if (p1(lx)) { bump(lx); bump(lx); }
      continue;
    }
    return;
  }
}

void a1_lexer_init(A1Lexer* lx, const char* src, size_t len) {
  lx->src = src; lx->len = len; lx->i = 0; lx->line = 1; lx->col = 1;
}

A1Token a1_lexer_next(A1Lexer* lx) {
  skip_ws_comments(lx);
  A1SourcePos s = pos_now(lx);
  char c = p1(lx);
  if (!c) return tk(A1_TK_EOF, s, s, lx->src + lx->i, 0);

  // multi-char operators
  if (c == '-' && p2(lx) == '>') { bump(lx); bump(lx); return tk(A1_TK_ARROW, s, pos_now(lx), lx->src + lx->i - 2, 2); }
  if (c == '=' && p2(lx) == '>') { bump(lx); bump(lx); return tk(A1_TK_FATARROW, s, pos_now(lx), lx->src + lx->i - 2, 2); }
  if (c == '=' && p2(lx) == '=') { bump(lx); bump(lx); return tk(A1_TK_EQEQ, s, pos_now(lx), lx->src + lx->i - 2, 2); }
  if (c == '!' && p2(lx) == '=') { bump(lx); bump(lx); return tk(A1_TK_NEQ, s, pos_now(lx), lx->src + lx->i - 2, 2); }
  if (c == '<' && p2(lx) == '=') { bump(lx); bump(lx); return tk(A1_TK_LTE, s, pos_now(lx), lx->src + lx->i - 2, 2); }
  if (c == '>' && p2(lx) == '=') { bump(lx); bump(lx); return tk(A1_TK_GTE, s, pos_now(lx), lx->src + lx->i - 2, 2); }
  if (c == '&' && p2(lx) == '&') { bump(lx); bump(lx); return tk(A1_TK_ANDAND, s, pos_now(lx), lx->src + lx->i - 2, 2); }
  if (c == '|' && p2(lx) == '|') { bump(lx); bump(lx); return tk(A1_TK_OROR, s, pos_now(lx), lx->src + lx->i - 2, 2); }

  switch (c) {
    case '(': bump(lx); return tk(A1_TK_LPAREN, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case ')': bump(lx); return tk(A1_TK_RPAREN, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '{': bump(lx); return tk(A1_TK_LBRACE, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '}': bump(lx); return tk(A1_TK_RBRACE, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '[': bump(lx); return tk(A1_TK_LBRACK, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case ']': bump(lx); return tk(A1_TK_RBRACK, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '<': bump(lx); return tk(A1_TK_LT, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '>': bump(lx); return tk(A1_TK_GT, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case ':': bump(lx); return tk(A1_TK_COLON, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case ';': bump(lx); return tk(A1_TK_SEMI, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case ',': bump(lx); return tk(A1_TK_COMMA, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '.': bump(lx); return tk(A1_TK_DOT, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '#': bump(lx); return tk(A1_TK_HASH, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '!': bump(lx); return tk(A1_TK_BANG, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '=': bump(lx); return tk(A1_TK_ASSIGN, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '+': bump(lx); return tk(A1_TK_PLUS, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '-': bump(lx); return tk(A1_TK_MINUS, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '*': bump(lx); return tk(A1_TK_STAR, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '/': bump(lx); return tk(A1_TK_SLASH, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '%': bump(lx); return tk(A1_TK_PERCENT, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '|': bump(lx); return tk(A1_TK_PIPE, s, pos_now(lx), lx->src + lx->i - 1, 1);
    case '"': {
      bump(lx);
      size_t st = lx->i;
      while (p1(lx) && p1(lx) != '"') {
        if (p1(lx) == '\\' && p2(lx)) { bump(lx); bump(lx); continue; }
        bump(lx);
      }
      size_t ed = lx->i;
      if (p1(lx) == '"') bump(lx);
      return tk(A1_TK_STRING, s, pos_now(lx), lx->src + st, ed - st);
    }
    default: break;
  }

  if (isalpha((unsigned char)c) || c == '_') {
    size_t st = lx->i;
    bump(lx);
    while (isalnum((unsigned char)p1(lx)) || p1(lx) == '_') bump(lx);
    size_t ed = lx->i;
    A1StringView sv = {lx->src + st, ed - st};
    A1TokenKind kind = A1_TK_IDENT;
    if (sv_eq(sv, "fn")) kind = A1_TK_KW_FN;
    else if (sv_eq(sv, "let")) kind = A1_TK_KW_LET;
    else if (sv_eq(sv, "mut")) kind = A1_TK_KW_MUT;
    else if (sv_eq(sv, "struct")) kind = A1_TK_KW_STRUCT;
    else if (sv_eq(sv, "enum")) kind = A1_TK_KW_ENUM;
    else if (sv_eq(sv, "match")) kind = A1_TK_KW_MATCH;
    else if (sv_eq(sv, "extern")) kind = A1_TK_KW_EXTERN;
    else if (sv_eq(sv, "import")) kind = A1_TK_KW_IMPORT;
    else if (sv_eq(sv, "if")) kind = A1_TK_KW_IF;
    else if (sv_eq(sv, "else")) kind = A1_TK_KW_ELSE;
    else if (sv_eq(sv, "while")) kind = A1_TK_KW_WHILE;
    else if (sv_eq(sv, "return")) kind = A1_TK_KW_RETURN;
    else if (sv_eq(sv, "where")) kind = A1_TK_KW_WHERE;
    else if (sv_eq(sv, "async")) kind = A1_TK_KW_ASYNC;
    else if (sv_eq(sv, "await")) kind = A1_TK_KW_AWAIT;
    else if (sv_eq(sv, "cfg")) kind = A1_TK_KW_CFG;
    else if (a1_is_reserved_keyword(sv)) kind = A1_TK_IDENT;
    return tk(kind, s, pos_now(lx), sv.ptr, sv.len);
  }

  if (isdigit((unsigned char)c)) {
    size_t st = lx->i;
    bump(lx);
    while (isdigit((unsigned char)p1(lx))) bump(lx);
    size_t ed = lx->i;
    return tk(A1_TK_INT, s, pos_now(lx), lx->src + st, ed - st);
  }

  bump(lx);
  return tk(A1_TK_EOF, s, pos_now(lx), lx->src + lx->i - 1, 1);
}
