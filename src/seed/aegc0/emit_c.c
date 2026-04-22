#include "emit_c.h"

#include <stdio.h>

static const char* c_type(TypeRef t) {
  switch (t.kind) {
    case TY_I32: return "int32_t";
    case TY_U64: return "uint64_t";
    case TY_VOID: return "void";
    default: return "int32_t";
  }
}

static void emit_expr(FILE* f, const Expr* e) {
  switch (e->kind) {
    case EX_INT:
      fprintf(f, "%llu", (unsigned long long)e->int_lit.value);
      break;
    case EX_VAR:
      fprintf(f, "%.*s", (int)e->var.name.len, e->var.name.ptr);
      break;
    case EX_CALL:
      fprintf(f, "%.*s(", (int)e->call.callee.len, e->call.callee.ptr);
      for (size_t i = 0; i < e->call.args.len; i++) {
        if (i) fprintf(f, ", ");
        emit_expr(f, e->call.args.items[i]);
      }
      fprintf(f, ")");
      break;
    case EX_UNARY:
      if (e->unary.op == UOP_NEG) fprintf(f, "-");
      if (e->unary.op == UOP_NOT) fprintf(f, "!");
      emit_expr(f, e->unary.rhs);
      break;
    case EX_BINARY: {
      const char* op = "+";
      switch (e->binary.op) {
        case BOP_ADD: op = "+"; break;
        case BOP_SUB: op = "-"; break;
        case BOP_MUL: op = "*"; break;
        case BOP_DIV: op = "/"; break;
        case BOP_MOD: op = "%"; break;
        case BOP_EQ: op = "=="; break;
        case BOP_NEQ: op = "!="; break;
        case BOP_LT: op = "<"; break;
        case BOP_LTE: op = "<="; break;
        case BOP_GT: op = ">"; break;
        case BOP_GTE: op = ">="; break;
        case BOP_AND: op = "&&"; break;
        case BOP_OR: op = "||"; break;
        default: break;
      }
      fprintf(f, "(");
      emit_expr(f, e->binary.lhs);
      fprintf(f, " %s ", op);
      emit_expr(f, e->binary.rhs);
      fprintf(f, ")");
      break;
    }
    default:
      fprintf(f, "0");
      break;
  }
}

static void emit_stmt_list(FILE* f, const StmtList list, int indent);

static void ind(FILE* f, int n) {
  for (int i = 0; i < n; i++) fputc(' ', f);
}

static void emit_stmt(FILE* f, const Stmt* s, int indent) {
  switch (s->kind) {
    case ST_LET:
      ind(f, indent);
      fprintf(f, "%s %.*s = ", c_type(s->let_stmt.ty), (int)s->let_stmt.name.len, s->let_stmt.name.ptr);
      emit_expr(f, s->let_stmt.init);
      fprintf(f, ";\n");
      break;
    case ST_ASSIGN:
      ind(f, indent);
      fprintf(f, "%.*s = ", (int)s->assign_stmt.name.len, s->assign_stmt.name.ptr);
      emit_expr(f, s->assign_stmt.value);
      fprintf(f, ";\n");
      break;
    case ST_RETURN:
      ind(f, indent);
      fprintf(f, "return ");
      emit_expr(f, s->ret_stmt.value);
      fprintf(f, ";\n");
      break;
    case ST_EXPR:
      ind(f, indent);
      emit_expr(f, s->expr_stmt.expr);
      fprintf(f, ";\n");
      break;
    case ST_IF:
      ind(f, indent);
      fprintf(f, "if (");
      emit_expr(f, s->if_stmt.cond);
      fprintf(f, ") {\n");
      emit_stmt_list(f, s->if_stmt.then_body, indent + 2);
      ind(f, indent);
      fprintf(f, "}\n");
      if (s->if_stmt.has_else) {
        ind(f, indent);
        fprintf(f, "else {\n");
        emit_stmt_list(f, s->if_stmt.else_body, indent + 2);
        ind(f, indent);
        fprintf(f, "}\n");
      }
      break;
    case ST_WHILE:
      ind(f, indent);
      fprintf(f, "while (");
      emit_expr(f, s->while_stmt.cond);
      fprintf(f, ") {\n");
      emit_stmt_list(f, s->while_stmt.body, indent + 2);
      ind(f, indent);
      fprintf(f, "}\n");
      break;
    default:
      break;
  }
}

static void emit_stmt_list(FILE* f, const StmtList list, int indent) {
  for (size_t i = 0; i < list.len; i++) emit_stmt(f, list.items[i], indent);
}

bool emit_c_program(const Program* prog, const CEmitOptions* opt) {
  FILE* f = fopen(opt->out_c_path, "wb");
  if (!f) return false;

  fprintf(f, "#include <stdint.h>\n");
  fprintf(f, "#include <stdbool.h>\n\n");

  // externs
  for (size_t i = 0; i < prog->fn_len; i++) {
    const FnDecl* fn = &prog->fns[i];
    if (!fn->is_extern_c) continue;
    fprintf(f, "extern %s %.*s(", c_type(fn->ret), (int)fn->name.len, fn->name.ptr);
    for (size_t p = 0; p < fn->params.len; p++) {
      if (p) fprintf(f, ", ");
      fprintf(f, "%s %.*s", c_type(fn->params.items[p].ty), (int)fn->params.items[p].name.len, fn->params.items[p].name.ptr);
    }
    fprintf(f, ");\n");
  }
  if (prog->fn_len) fprintf(f, "\n");

  // functions
  for (size_t i = 0; i < prog->fn_len; i++) {
    const FnDecl* fn = &prog->fns[i];
    if (fn->is_extern_c) continue;
    if (fn->name.len == 4 && fn->name.ptr[0] == 'm' && fn->name.ptr[1] == 'a' && fn->name.ptr[2] == 'i' && fn->name.ptr[3] == 'n') {
      fprintf(f, "%s main(void)", c_type(fn->ret));
    } else {
      fprintf(f, "%s %.*s(", c_type(fn->ret), (int)fn->name.len, fn->name.ptr);
      for (size_t p = 0; p < fn->params.len; p++) {
        if (p) fprintf(f, ", ");
        fprintf(f, "%s %.*s", c_type(fn->params.items[p].ty), (int)fn->params.items[p].name.len, fn->params.items[p].name.ptr);
      }
      fprintf(f, ")");
    }
    fprintf(f, " {\n");
    emit_stmt_list(f, fn->body, 2);
    fprintf(f, "}\n\n");
  }

  fclose(f);
  return true;
}
