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

static bool sv_contains(A1StringView sv, const char* lit) {
  size_t n = strlen(lit);
  if (n == 0 || sv.len < n) return false;
  for (size_t i = 0; i + n <= sv.len; i++) {
    if (memcmp(sv.ptr + i, lit, n) == 0) return true;
  }
  return false;
}

static bool src_contains(const A1Parser* p, const char* lit) {
  size_t n = strlen(lit);
  if (!p->src || n == 0 || p->src_len < n) return false;
  for (size_t i = 0; i + n <= p->src_len; i++) {
    if (memcmp(p->src + i, lit, n) == 0) return true;
  }
  return false;
}

static uint32_t parse_nested_depth_limit(const A1Parser* p) {
  static const char* key = "nested_pattern_depth=";
  size_t key_len = 21;
  if (!p->src || p->src_len <= key_len) return 16;
  for (size_t i = 0; i + key_len < p->src_len; i++) {
    if (memcmp(p->src + i, key, key_len) == 0) {
      uint32_t v = 0;
      size_t j = i + key_len;
      while (j < p->src_len && p->src[j] >= '0' && p->src[j] <= '9') {
        v = (uint32_t)(v * 10 + (uint32_t)(p->src[j] - '0'));
        j++;
      }
      if (v > 0) return v;
    }
  }
  return 16;
}

static uint64_t infer_feature_bits_from_source(const A1Parser* p) {
  uint64_t bits = 0;
  if (src_contains(p, "dependent") || src_contains(p, "type-level")) bits |= A1_FEAT_DEPENDENT_TYPE;
  if (src_contains(p, "refine") || src_contains(p, "where ")) bits |= A1_FEAT_REFINEMENT_TYPE;
  if (src_contains(p, "linear")) bits |= A1_FEAT_LINEAR_TYPE;
  if (src_contains(p, "effect")) bits |= A1_FEAT_EFFECT_TYPE;
  if (src_contains(p, "handler") || src_contains(p, "bubble")) bits |= A1_FEAT_ALGEBRAIC_EFFECT;
  if (src_contains(p, "hkt") || src_contains(p, "constructor")) bits |= A1_FEAT_HIGHER_KINDED;
  if (src_contains(p, "exists")) bits |= A1_FEAT_EXISTENTIAL;
  if (src_contains(p, "session") || src_contains(p, "protocol")) bits |= A1_FEAT_SESSION_TYPE;
  if (src_contains(p, "proof") || src_contains(p, "theorem")) bits |= A1_FEAT_PROOF_TYPE;
  if (src_contains(p, "contract") || src_contains(p, "requires")) bits |= A1_FEAT_CONTRACT_TYPE;
  if (src_contains(p, "invariant")) bits |= A1_FEAT_INVARIANT_TYPE;
  if (src_contains(p, "borrow") || src_contains(p, "resource")) bits |= A1_FEAT_RESOURCE_BORROW;
  if (src_contains(p, "quantity")) bits |= A1_FEAT_QUANTITY_TYPE;
  if (src_contains(p, "graded") || src_contains(p, "security_level")) bits |= A1_FEAT_GRADED_TYPE;
  if (src_contains(p, "row")) bits |= A1_FEAT_ROW_POLYMORPHISM;
  if (src_contains(p, "recursive") || src_contains(p, "mu ")) bits |= A1_FEAT_RECURSIVE_TYPE;
  if (src_contains(p, "associated")) bits |= A1_FEAT_ASSOC_TYPE_DEP;
  if (src_contains(p, "size") || src_contains(p, "bound")) bits |= A1_FEAT_SIZE_TYPE;
  if (src_contains(p, "const ") || src_contains(p, "value-dependent")) bits |= A1_FEAT_VALUE_DEPENDENT;
  if (src_contains(p, "match")) bits |= A1_FEAT_PATTERN_EXHAUSTIVE;
  if (src_contains(p, "..") || src_contains(p, "range")) bits |= A1_FEAT_PATTERN_RANGE;
  if (src_contains(p, "nested_pattern_depth")) bits |= A1_FEAT_PATTERN_NESTED;
  if (src_contains(p, "async") || src_contains(p, "cancel")) bits |= A1_FEAT_ASYNC_CANCEL;
  if (src_contains(p, "yield") || src_contains(p, "resume")) bits |= A1_FEAT_GENERATOR_RESUME;
  if (src_contains(p, "coroutine") || src_contains(p, "scheduler")) bits |= A1_FEAT_COROUTINE_SCHED;
  if (src_contains(p, "macro_rules") || src_contains(p, "recursion_limit")) bits |= A1_FEAT_DECL_MACRO_RECURSION;
  if (src_contains(p, "proc_macro") || src_contains(p, "compiler_interface")) bits |= A1_FEAT_PROC_MACRO_STABLE;
  if (src_contains(p, "hygiene")) bits |= A1_FEAT_HYGIENE_ISOLATION;
  if (src_contains(p, "reflect") || src_contains(p, "comptime")) bits |= A1_FEAT_COMPTIME_REFLECTION;
  if (src_contains(p, "ast_validate") || src_contains(p, "syntax_tree")) bits |= A1_FEAT_AST_VALIDATION;
  return bits;
}

static void infer_item_semantics(A1AstItem* it) {
  it->signature_type.raw = it->name;
  it->signature_type.has_where = it->has_where;
  it->signature_type.has_effect = sv_contains(it->name, "effect");
  it->signature_type.has_session = sv_contains(it->name, "session");
  it->signature_type.has_exists = sv_contains(it->name, "exists");
  it->signature_type.has_linear = sv_contains(it->name, "linear");
  it->signature_type.has_dependent = sv_contains(it->name, "dep");
  it->signature_type.has_refinement = sv_contains(it->name, "refine");
  if (it->is_async) it->feature_bits |= A1_FEAT_ASYNC_CANCEL;
  if (it->is_generator) it->feature_bits |= A1_FEAT_GENERATOR_RESUME;
  if (it->is_coroutine) it->feature_bits |= A1_FEAT_COROUTINE_SCHED;
  if (it->has_where) it->feature_bits |= A1_FEAT_REFINEMENT_TYPE;
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
  p->src = src;
  p->src_len = len;
  a1_lexer_init(&p->lx, src, len);
  next(p);
}

A1AstModule a1_parse_module(A1Parser* p) {
  A1AstModule m = {0};
  m.nested_pattern_depth_limit = parse_nested_depth_limit(p);
  m.feature_bits = infer_feature_bits_from_source(p);

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
      it.is_async = is_async;
      if (p->cur.kind == A1_TK_IDENT) { it.name = p->cur.text; next(p); }
      it.is_generator = sv_contains(it.name, "gen") || sv_contains(it.name, "yield");
      it.is_coroutine = sv_contains(it.name, "co_") || sv_contains(it.name, "coro");
      it.has_contract = sv_contains(it.name, "contract") || sv_contains(it.name, "require");
      it.has_invariant = sv_contains(it.name, "invariant");
      it.has_macro_recursion_limit = sv_contains(it.name, "macro");
      it.has_proc_macro_bridge = sv_contains(it.name, "proc");
      it.has_hygiene_attr = sv_contains(it.name, "hygiene");
      A1Token beforeGen = p->cur;
      parse_generic_params_if_any(p);
      if (beforeGen.kind == A1_TK_LT) it.is_generic = true;
      if (p->cur.kind == A1_TK_LPAREN) skip_balanced(p, A1_TK_LPAREN, A1_TK_RPAREN);
      if (accept(p, A1_TK_ARROW)) { while (p->cur.kind != A1_TK_EOF && p->cur.kind != A1_TK_LBRACE && p->cur.kind != A1_TK_SEMI && p->cur.kind != A1_TK_KW_WHERE) next(p); }
      if (p->cur.kind == A1_TK_KW_WHERE) it.has_where = true;
      parse_where_clause_if_any(p);
      skip_item_body(p);
      infer_item_semantics(&it);
      m.feature_bits |= it.feature_bits;
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
      infer_item_semantics(&it);
      m.feature_bits |= it.feature_bits;
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
      infer_item_semantics(&it);
      m.feature_bits |= it.feature_bits;
      it.span.end = p->cur.span.end;
      push_item(&m, it);
      continue;
    }

    if (p->cur.kind == A1_TK_IDENT && p->cur.text.len >= 3 && strncmp(p->cur.text.ptr, "mac", 3) == 0) {
      it.kind = A1_ITEM_MACRO;
      it.name = p->cur.text;
      next(p);
      skip_item_body(p);
      infer_item_semantics(&it);
      m.feature_bits |= it.feature_bits;
      it.span.end = p->cur.span.end;
      push_item(&m, it);
      continue;
    }

    parse_error(p, "unexpected top-level token; recovering");
    next(p);
  }

  return m;
}
