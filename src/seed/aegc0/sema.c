#include "sema.h"

#include <stdio.h>
#include <string.h>

typedef struct Var {
  Str name;
  TypeRef ty;
} Var;

typedef struct VarTable {
  Var* items;
  size_t len;
} VarTable;

static bool var_eq(Str a, Str b) { return str_eq(a, b); }

static void vadd(Sema* s, VarTable* vt, Str name, TypeRef ty) {
  vt->len++;
  Var* new_items = (Var*)arena_alloc(s->arena, sizeof(Var) * vt->len, _Alignof(Var));
  if (vt->items) memcpy(new_items, vt->items, sizeof(Var) * (vt->len - 1));
  new_items[vt->len - 1].name = name;
  new_items[vt->len - 1].ty = ty;
  vt->items = new_items;
}

static bool vget(VarTable* vt, Str name, TypeRef* out) {
  for (size_t i = 0; i < vt->len; i++) {
    if (var_eq(vt->items[i].name, name)) {
      *out = vt->items[i].ty;
      return true;
    }
  }
  return false;
}

static void err_span(Span sp, const char* msg) {
  fprintf(stderr, "aegc0: error:%u:%u: %s\n", sp.start.line, sp.start.col, msg);
}

static TypeRef type_of_expr(Sema* s, VarTable* vt, Expr* e) {
  TypeRef t = {TY_I32};
  switch (e->kind) {
    case EX_INT:
      t.kind = TY_I32;
      break;
    case EX_VAR: {
      if (!vget(vt, e->var.name, &t)) {
        err_span(e->span, "use of undeclared variable");
        s->had_error = true;
        t.kind = TY_I32;
      }
      break;
    }
    case EX_CALL:
      // seed: assume extern calls return i32 unless known otherwise; conservative default i32.
      t.kind = TY_I32;
      break;
    case EX_UNARY:
      t = type_of_expr(s, vt, e->unary.rhs);
      break;
    case EX_BINARY: {
      TypeRef lt = type_of_expr(s, vt, e->binary.lhs);
      TypeRef rt = type_of_expr(s, vt, e->binary.rhs);
      // comparisons/logical → i32 (as bool)
      if (e->binary.op == BOP_EQ || e->binary.op == BOP_NEQ || e->binary.op == BOP_LT ||
          e->binary.op == BOP_LTE || e->binary.op == BOP_GT || e->binary.op == BOP_GTE ||
          e->binary.op == BOP_AND || e->binary.op == BOP_OR) {
        t.kind = TY_I32;
      } else {
        // arithmetic: require same kind
        if (lt.kind != rt.kind) {
          err_span(e->span, "type mismatch in binary arithmetic");
          s->had_error = true;
        }
        t = lt;
      }
      break;
    }
    default:
      t.kind = TY_I32;
      break;
  }
  e->ty = t;
  return t;
}

static void check_stmt_list(Sema* s, VarTable* vt, StmtList list, TypeRef fn_ret);

static void check_stmt(Sema* s, VarTable* vt, Stmt* st, TypeRef fn_ret) {
  switch (st->kind) {
    case ST_LET: {
      TypeRef it = type_of_expr(s, vt, st->let_stmt.init);
      if (it.kind != st->let_stmt.ty.kind) {
        err_span(st->span, "type mismatch in let initializer");
        s->had_error = true;
      }
      vadd(s, vt, st->let_stmt.name, st->let_stmt.ty);
      break;
    }
    case ST_ASSIGN: {
      TypeRef vt_ty;
      if (!vget(vt, st->assign_stmt.name, &vt_ty)) {
        err_span(st->span, "assignment to undeclared variable");
        s->had_error = true;
        vt_ty.kind = TY_I32;
      }
      TypeRef rhs = type_of_expr(s, vt, st->assign_stmt.value);
      if (rhs.kind != vt_ty.kind) {
        err_span(st->span, "type mismatch in assignment");
        s->had_error = true;
      }
      break;
    }
    case ST_IF: {
      (void)type_of_expr(s, vt, st->if_stmt.cond);
      check_stmt_list(s, vt, st->if_stmt.then_body, fn_ret);
      if (st->if_stmt.has_else) check_stmt_list(s, vt, st->if_stmt.else_body, fn_ret);
      break;
    }
    case ST_WHILE: {
      (void)type_of_expr(s, vt, st->while_stmt.cond);
      check_stmt_list(s, vt, st->while_stmt.body, fn_ret);
      break;
    }
    case ST_RETURN: {
      TypeRef r = type_of_expr(s, vt, st->ret_stmt.value);
      if (r.kind != fn_ret.kind) {
        err_span(st->span, "return type mismatch");
        s->had_error = true;
      }
      break;
    }
    case ST_EXPR:
      (void)type_of_expr(s, vt, st->expr_stmt.expr);
      break;
    default:
      break;
  }
}

static void check_stmt_list(Sema* s, VarTable* vt, StmtList list, TypeRef fn_ret) {
  for (size_t i = 0; i < list.len; i++) check_stmt(s, vt, list.items[i], fn_ret);
}

void sema_init(Sema* s, Arena* arena) {
  s->arena = arena;
  s->had_error = false;
}

bool sema_check_program(Sema* s, Program* prog) {
  for (size_t i = 0; i < prog->fn_len; i++) {
    FnDecl* fn = &prog->fns[i];
    if (fn->is_extern_c) continue;
    VarTable vt = {0};
    // params in scope
    for (size_t p = 0; p < fn->params.len; p++) vadd(s, &vt, fn->params.items[p].name, fn->params.items[p].ty);
    check_stmt_list(s, &vt, fn->body, fn->ret);
  }
  return !s->had_error;
}
