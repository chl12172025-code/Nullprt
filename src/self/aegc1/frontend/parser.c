#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void next(A1Parser* p) { p->cur = a1_lexer_next(&p->lx); }

static void parse_error(A1Parser* p, const char* msg) {
  fprintf(stderr, "aegc1 parser error:%u:%u: %s\n", p->cur.span.start.line, p->cur.span.start.col, msg);
  p->had_error = true;
}

static bool accept(A1Parser* p, A1TokenKind k) {
  if (p->cur.kind == k) { next(p); return true; }
  return false;
}

static bool expect(A1Parser* p, A1TokenKind k, const char* msg) {
  if (!accept(p, k)) { parse_error(p, msg); return false; }
  return true;
}

static void skip_balanced(A1Parser* p, A1TokenKind open, A1TokenKind close) {
  int depth = 0;
  if (accept(p, open)) depth = 1;
  while (p->cur.kind != A1_TK_EOF && depth > 0) {
    if (p->cur.kind == open) depth++;
    else if (p->cur.kind == close) depth--;
    next(p);
  }
}

static void skip_item_body(A1Parser* p) {
  if (p->cur.kind == A1_TK_LBRACE) {
    skip_balanced(p, A1_TK_LBRACE, A1_TK_RBRACE);
    return;
  }
  while (p->cur.kind != A1_TK_EOF && p->cur.kind != A1_TK_SEMI) next(p);
  accept(p, A1_TK_SEMI);
}

static void push_item(A1AstModule* m, A1AstItem it) {
  A1AstItem* n = (A1AstItem*)realloc(m->items, sizeof(A1AstItem) * (m->len + 1));
  if (!n) return;
  n[m->len] = it;
  m->items = n;
  m->len++;
}

static void parse_attributes_or_cfg(A1Parser* p) {
  while (p->cur.kind == A1_TK_HASH) {
    next(p);
    accept(p, A1_TK_BANG);
    if (accept(p, A1_TK_LBRACK)) {
      while (p->cur.kind != A1_TK_EOF && p->cur.kind != A1_TK_RBRACK) next(p);
      accept(p, A1_TK_RBRACK);
    }
  }
}

static void parse_generic_params_if_any(A1Parser* p) {
  if (!accept(p, A1_TK_LT)) return;
  int depth = 1;
  while (p->cur.kind != A1_TK_EOF && depth > 0) {
    if (p->cur.kind == A1_TK_LT) depth++;
    else if (p->cur.kind == A1_TK_GT) depth--;
    next(p);
  }
}

static void parse_where_clause_if_any(A1Parser* p) {
  if (!accept(p, A1_TK_KW_WHERE)) return;
  while (p->cur.kind != A1_TK_EOF && p->cur.kind != A1_TK_LBRACE && p->cur.kind != A1_TK_SEMI) next(p);
}

void a1_parser_init(A1Parser* p, const char* src, size_t len) {
  memset(p, 0, sizeof(*p));
  a1_lexer_init(&p->lx, src, len);
  next(p);
}

A1AstModule a1_parse_module(A1Parser* p) {
  A1AstModule m = {0};

  while (p->cur.kind != A1_TK_EOF) {
    bool saw_attrs = false;
    bool saw_cfg = false;
    if (p->cur.kind == A1_TK_HASH) saw_attrs = true;
    parse_attributes_or_cfg(p);
    saw_cfg = saw_attrs; // current parser treats attrs/cfg together in token phase

    A1AstItem it;
    memset(&it, 0, sizeof(it));
    it.span.start = p->cur.span.start;
    it.has_macro_attrs = saw_attrs;
    it.has_cfg = saw_cfg;

    if (accept(p, A1_TK_KW_IMPORT)) {
      it.kind = A1_ITEM_IMPORT;
      if (p->cur.kind == A1_TK_IDENT) { it.name = p->cur.text; next(p); }
      while (p->cur.kind != A1_TK_EOF && p->cur.kind != A1_TK_SEMI) next(p);
      accept(p, A1_TK_SEMI);
      it.span.end = p->cur.span.end;
      push_item(&m, it);
      continue;
    }

    bool is_extern = accept(p, A1_TK_KW_EXTERN);
    bool is_async = accept(p, A1_TK_KW_ASYNC);
    (void)is_extern;
    (void)is_async;

    if (accept(p, A1_TK_KW_FN)) {
      it.kind = A1_ITEM_FN;
      it.is_extern_c = is_extern;
      if (p->cur.kind == A1_TK_IDENT) { it.name = p->cur.text; next(p); }
      A1Token beforeGen = p->cur;
      parse_generic_params_if_any(p);
      if (beforeGen.kind == A1_TK_LT) it.is_generic = true;
      if (p->cur.kind == A1_TK_LPAREN) skip_balanced(p, A1_TK_LPAREN, A1_TK_RPAREN);
      if (accept(p, A1_TK_ARROW)) { while (p->cur.kind != A1_TK_EOF && p->cur.kind != A1_TK_LBRACE && p->cur.kind != A1_TK_SEMI && p->cur.kind != A1_TK_KW_WHERE) next(p); }
      if (p->cur.kind == A1_TK_KW_WHERE) it.has_where = true;
      parse_where_clause_if_any(p);
      skip_item_body(p);
      it.span.end = p->cur.span.end;
      push_item(&m, it);
      continue;
    }

    if (accept(p, A1_TK_KW_STRUCT)) {
      it.kind = A1_ITEM_STRUCT;
      if (p->cur.kind == A1_TK_IDENT) { it.name = p->cur.text; next(p); }
      A1Token beforeGen = p->cur;
      parse_generic_params_if_any(p);
      if (beforeGen.kind == A1_TK_LT) it.is_generic = true;
      skip_item_body(p);
      it.span.end = p->cur.span.end;
      push_item(&m, it);
      continue;
    }

    if (accept(p, A1_TK_KW_ENUM)) {
      it.kind = A1_ITEM_ENUM;
      if (p->cur.kind == A1_TK_IDENT) { it.name = p->cur.text; next(p); }
      A1Token beforeGen = p->cur;
      parse_generic_params_if_any(p);
      if (beforeGen.kind == A1_TK_LT) it.is_generic = true;
      skip_item_body(p);
      it.span.end = p->cur.span.end;
      push_item(&m, it);
      continue;
    }

    if (p->cur.kind == A1_TK_IDENT && p->cur.text.len >= 3 && strncmp(p->cur.text.ptr, "mac", 3) == 0) {
      it.kind = A1_ITEM_MACRO;
      it.name = p->cur.text;
      next(p);
      skip_item_body(p);
      it.span.end = p->cur.span.end;
      push_item(&m, it);
      continue;
    }

    parse_error(p, "unexpected top-level token; recovering");
    next(p);
  }

  return m;
}
