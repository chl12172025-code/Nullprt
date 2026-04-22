#include "parser.h"

#include <stdio.h>
#include <string.h>

static void next(Parser* p) { p->cur = lexer_next(&p->lx); }

static void perr(Parser* p, Span sp, const char* msg) {
  (void)p;
  fprintf(stderr, "aegc0: error:%u:%u: %s\n", sp.start.line, sp.start.col, msg);
  p->had_error = true;
}

static bool accept(Parser* p, TokenKind k) {
  if (p->cur.kind == k) {
    next(p);
    return true;
  }
  return false;
}

static void expect(Parser* p, TokenKind k, const char* what) {
  if (!accept(p, k)) {
    perr(p, p->cur.span, what);
  }
}

static TypeRef parse_type(Parser* p) {
  TypeRef t;
  t.kind = TY_VOID;
  if (p->cur.kind == TK_IDENT) {
    if (str_eq(p->cur.text, str_from_c("i32"))) { t.kind = TY_I32; next(p); return t; }
    if (str_eq(p->cur.text, str_from_c("u64"))) { t.kind = TY_U64; next(p); return t; }
  }
  perr(p, p->cur.span, "expected type (i32/u64)");
  next(p);
  return t;
}

static Expr* parse_expr(Parser* p);

static Expr* arena_new_expr(Parser* p) {
  Expr* e = (Expr*)arena_alloc(p->arena, sizeof(Expr), _Alignof(Expr));
  return e;
}

static Stmt* arena_new_stmt(Parser* p) {
  Stmt* s = (Stmt*)arena_alloc(p->arena, sizeof(Stmt), _Alignof(Stmt));
  return s;
}

static Expr* parse_primary(Parser* p) {
  if (p->cur.kind == TK_INT) {
    Expr* e = arena_new_expr(p);
    e->kind = EX_INT;
    e->span = p->cur.span;
    e->int_lit.value = p->cur.int_value;
    next(p);
    return e;
  }

  if (p->cur.kind == TK_IDENT) {
    Token ident = p->cur;
    next(p);
    if (accept(p, TK_LPAREN)) {
      Expr* e = arena_new_expr(p);
      e->kind = EX_CALL;
      e->span.start = ident.span.start;
      e->call.callee = ident.text;
      // args
      Expr** args = NULL;
      size_t args_len = 0;
      if (!accept(p, TK_RPAREN)) {
        for (;;) {
          Expr* a = parse_expr(p);
          args_len++;
          Expr** new_args = (Expr**)arena_alloc(p->arena, sizeof(Expr*) * args_len, _Alignof(Expr*));
          if (args) memcpy(new_args, args, sizeof(Expr*) * (args_len - 1));
          new_args[args_len - 1] = a;
          args = new_args;
          if (accept(p, TK_COMMA)) continue;
          expect(p, TK_RPAREN, "expected ')'");
          break;
        }
      }
      e->span.end = p->cur.span.end;
      e->call.args.items = args;
      e->call.args.len = args_len;
      return e;
    }
    Expr* e = arena_new_expr(p);
    e->kind = EX_VAR;
    e->span = ident.span;
    e->var.name = ident.text;
    return e;
  }

  if (accept(p, TK_LPAREN)) {
    Expr* inner = parse_expr(p);
    expect(p, TK_RPAREN, "expected ')'");
    return inner;
  }

  perr(p, p->cur.span, "expected expression");
  next(p);
  Expr* e = arena_new_expr(p);
  e->kind = EX_INT;
  e->int_lit.value = 0;
  e->span = p->cur.span;
  return e;
}

static Expr* parse_unary(Parser* p) {
  if (accept(p, TK_MINUS)) {
    Expr* e = arena_new_expr(p);
    e->kind = EX_UNARY;
    e->unary.op = UOP_NEG;
    e->unary.rhs = parse_unary(p);
    return e;
  }
  if (accept(p, TK_BANG)) {
    Expr* e = arena_new_expr(p);
    e->kind = EX_UNARY;
    e->unary.op = UOP_NOT;
    e->unary.rhs = parse_unary(p);
    return e;
  }
  return parse_primary(p);
}

static int bin_prec(TokenKind k) {
  switch (k) {
    case TK_OROR: return 1;
    case TK_ANDAND: return 2;
    case TK_EQEQ:
    case TK_NEQ: return 3;
    case TK_LT:
    case TK_LTE:
    case TK_GT:
    case TK_GTE: return 4;
    case TK_PLUS:
    case TK_MINUS: return 5;
    case TK_STAR:
    case TK_SLASH:
    case TK_PERCENT: return 6;
    default: return 0;
  }
}

static BinaryOp bin_op(TokenKind k) {
  switch (k) {
    case TK_PLUS: return BOP_ADD;
    case TK_MINUS: return BOP_SUB;
    case TK_STAR: return BOP_MUL;
    case TK_SLASH: return BOP_DIV;
    case TK_PERCENT: return BOP_MOD;
    case TK_EQEQ: return BOP_EQ;
    case TK_NEQ: return BOP_NEQ;
    case TK_LT: return BOP_LT;
    case TK_LTE: return BOP_LTE;
    case TK_GT: return BOP_GT;
    case TK_GTE: return BOP_GTE;
    case TK_ANDAND: return BOP_AND;
    case TK_OROR: return BOP_OR;
    default: return BOP_ADD;
  }
}

static Expr* parse_bin_rhs(Parser* p, int min_prec, Expr* lhs) {
  for (;;) {
    int prec = bin_prec(p->cur.kind);
    if (prec < min_prec) return lhs;
    TokenKind opk = p->cur.kind;
    next(p);
    Expr* rhs = parse_unary(p);
    int next_prec = bin_prec(p->cur.kind);
    if (prec < next_prec) rhs = parse_bin_rhs(p, prec + 1, rhs);
    Expr* e = arena_new_expr(p);
    e->kind = EX_BINARY;
    e->binary.op = bin_op(opk);
    e->binary.lhs = lhs;
    e->binary.rhs = rhs;
    lhs = e;
  }
}

static Expr* parse_expr(Parser* p) {
  Expr* lhs = parse_unary(p);
  return parse_bin_rhs(p, 1, lhs);
}

static StmtList parse_block(Parser* p) {
  StmtList list = {0};
  expect(p, TK_LBRACE, "expected '{'");
  while (p->cur.kind != TK_RBRACE && p->cur.kind != TK_EOF) {
    // stmt
    Stmt* s = NULL;
    if (accept(p, TK_KW_LET)) {
      s = arena_new_stmt(p);
      s->kind = ST_LET;
      Token name = p->cur;
      expect(p, TK_IDENT, "expected identifier after let");
      s->let_stmt.name = name.text;
      expect(p, TK_COLON, "expected ':' after let name");
      s->let_stmt.ty = parse_type(p);
      expect(p, TK_ASSIGN, "expected '=' in let");
      s->let_stmt.init = parse_expr(p);
      expect(p, TK_SEMI, "expected ';' after let");
    } else if (accept(p, TK_KW_RETURN)) {
      s = arena_new_stmt(p);
      s->kind = ST_RETURN;
      s->ret_stmt.value = parse_expr(p);
      expect(p, TK_SEMI, "expected ';' after return");
    } else if (accept(p, TK_KW_IF)) {
      s = arena_new_stmt(p);
      s->kind = ST_IF;
      expect(p, TK_LPAREN, "expected '(' after if");
      s->if_stmt.cond = parse_expr(p);
      expect(p, TK_RPAREN, "expected ')'");
      s->if_stmt.then_body = parse_block(p);
      if (accept(p, TK_KW_ELSE)) {
        s->if_stmt.has_else = true;
        s->if_stmt.else_body = parse_block(p);
      } else {
        s->if_stmt.has_else = false;
      }
    } else if (accept(p, TK_KW_WHILE)) {
      s = arena_new_stmt(p);
      s->kind = ST_WHILE;
      expect(p, TK_LPAREN, "expected '(' after while");
      s->while_stmt.cond = parse_expr(p);
      expect(p, TK_RPAREN, "expected ')'");
      s->while_stmt.body = parse_block(p);
    } else if (p->cur.kind == TK_IDENT) {
      Token name = p->cur;
      next(p);
      if (accept(p, TK_ASSIGN)) {
        s = arena_new_stmt(p);
        s->kind = ST_ASSIGN;
        s->assign_stmt.name = name.text;
        s->assign_stmt.value = parse_expr(p);
        expect(p, TK_SEMI, "expected ';' after assignment");
      } else if (accept(p, TK_LPAREN)) {
        // call stmt: name(...)
        Expr* call = arena_new_expr(p);
        call->kind = EX_CALL;
        call->call.callee = name.text;
        Expr** args = NULL;
        size_t args_len = 0;
        if (!accept(p, TK_RPAREN)) {
          for (;;) {
            Expr* a = parse_expr(p);
            args_len++;
            Expr** new_args = (Expr**)arena_alloc(p->arena, sizeof(Expr*) * args_len, _Alignof(Expr*));
            if (args) memcpy(new_args, args, sizeof(Expr*) * (args_len - 1));
            new_args[args_len - 1] = a;
            args = new_args;
            if (accept(p, TK_COMMA)) continue;
            expect(p, TK_RPAREN, "expected ')'");
            break;
          }
        }
        call->call.args.items = args;
        call->call.args.len = args_len;
        s = arena_new_stmt(p);
        s->kind = ST_EXPR;
        s->expr_stmt.expr = call;
        expect(p, TK_SEMI, "expected ';' after call");
      } else {
        perr(p, p->cur.span, "expected assignment '=' or call '(...)' statement");
        // recover
        while (p->cur.kind != TK_SEMI && p->cur.kind != TK_RBRACE && p->cur.kind != TK_EOF) next(p);
        accept(p, TK_SEMI);
        continue;
      }
    } else {
      perr(p, p->cur.span, "unexpected token in block");
      next(p);
      continue;
    }

    if (s) {
      list.len++;
      Stmt** new_items = (Stmt**)arena_alloc(p->arena, sizeof(Stmt*) * list.len, _Alignof(Stmt*));
      if (list.items) memcpy(new_items, list.items, sizeof(Stmt*) * (list.len - 1));
      new_items[list.len - 1] = s;
      list.items = new_items;
    }
  }
  expect(p, TK_RBRACE, "expected '}'");
  return list;
}

static ParamList parse_params(Parser* p) {
  ParamList pl = {0};
  expect(p, TK_LPAREN, "expected '('");
  if (accept(p, TK_RPAREN)) return pl;
  for (;;) {
    Token name = p->cur;
    expect(p, TK_IDENT, "expected param name");
    expect(p, TK_COLON, "expected ':' after param name");
    TypeRef ty = parse_type(p);
    pl.len++;
    Param* new_items = (Param*)arena_alloc(p->arena, sizeof(Param) * pl.len, _Alignof(Param));
    if (pl.items) memcpy(new_items, pl.items, sizeof(Param) * (pl.len - 1));
    new_items[pl.len - 1].name = name.text;
    new_items[pl.len - 1].ty = ty;
    pl.items = new_items;
    if (accept(p, TK_COMMA)) continue;
    expect(p, TK_RPAREN, "expected ')'");
    break;
  }
  return pl;
}

static FnDecl parse_fn(Parser* p, bool is_extern_c) {
  FnDecl fn;
  memset(&fn, 0, sizeof(fn));
  fn.is_extern_c = is_extern_c;
  Token name = p->cur;
  expect(p, TK_IDENT, "expected function name");
  fn.name = name.text;
  fn.params = parse_params(p);
  expect(p, TK_ARROW, "expected '->' return type");
  fn.ret = parse_type(p);

  if (is_extern_c) {
    expect(p, TK_SEMI, "expected ';' after extern fn decl");
    fn.body.items = NULL;
    fn.body.len = 0;
    return fn;
  }
  fn.body = parse_block(p);
  return fn;
}

void parser_init(Parser* p, Arena* arena, const char* filename, const char* src, size_t len) {
  p->arena = arena;
  p->filename = filename;
  lexer_init(&p->lx, src, len);
  p->had_error = false;
  next(p);
}

Program* parse_program(Parser* p) {
  Program* prog = (Program*)arena_alloc(p->arena, sizeof(Program), _Alignof(Program));
  prog->fns = NULL;
  prog->fn_len = 0;

  while (p->cur.kind != TK_EOF) {
    if (accept(p, TK_KW_EXTERN)) {
      // extern "C" fn ...
      if (p->cur.kind != TK_STRING || !str_eq(p->cur.text, str_from_c("C"))) {
        perr(p, p->cur.span, "expected extern \"C\"");
      } else {
        next(p);
      }
      expect(p, TK_KW_FN, "expected fn after extern \"C\"");
      FnDecl fn = parse_fn(p, true);
      prog->fn_len++;
      FnDecl* new_fns = (FnDecl*)arena_alloc(p->arena, sizeof(FnDecl) * prog->fn_len, _Alignof(FnDecl));
      if (prog->fns) memcpy(new_fns, prog->fns, sizeof(FnDecl) * (prog->fn_len - 1));
      new_fns[prog->fn_len - 1] = fn;
      prog->fns = new_fns;
      continue;
    }
    if (accept(p, TK_KW_FN)) {
      FnDecl fn = parse_fn(p, false);
      prog->fn_len++;
      FnDecl* new_fns = (FnDecl*)arena_alloc(p->arena, sizeof(FnDecl) * prog->fn_len, _Alignof(FnDecl));
      if (prog->fns) memcpy(new_fns, prog->fns, sizeof(FnDecl) * (prog->fn_len - 1));
      new_fns[prog->fn_len - 1] = fn;
      prog->fns = new_fns;
      continue;
    }
    perr(p, p->cur.span, "expected 'extern' or 'fn' at top level");
    next(p);
  }

  return prog;
}
