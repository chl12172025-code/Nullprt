#pragma once

#include "aegc0.h"

typedef enum TypeKind {
  TY_I32 = 1,
  TY_U64 = 2,
  TY_VOID = 3,
} TypeKind;

typedef struct TypeRef {
  TypeKind kind;
} TypeRef;

typedef enum ExprKind {
  EX_INT = 1,
  EX_VAR = 2,
  EX_CALL = 3,
  EX_UNARY = 4,
  EX_BINARY = 5,
} ExprKind;

typedef enum UnaryOp {
  UOP_NEG = 1,
  UOP_NOT = 2,
} UnaryOp;

typedef enum BinaryOp {
  BOP_ADD = 1,
  BOP_SUB,
  BOP_MUL,
  BOP_DIV,
  BOP_MOD,
  BOP_EQ,
  BOP_NEQ,
  BOP_LT,
  BOP_LTE,
  BOP_GT,
  BOP_GTE,
  BOP_AND,
  BOP_OR,
} BinaryOp;

typedef struct Expr Expr;

typedef struct ExprList {
  Expr** items;
  size_t len;
} ExprList;

struct Expr {
  ExprKind kind;
  Span span;
  TypeRef ty; // filled by checker (minimal)
  union {
    struct { uint64_t value; } int_lit;
    struct { Str name; } var;
    struct { Str callee; ExprList args; } call;
    struct { UnaryOp op; Expr* rhs; } unary;
    struct { BinaryOp op; Expr* lhs; Expr* rhs; } binary;
  };
};

typedef enum StmtKind {
  ST_LET = 1,
  ST_ASSIGN = 2,
  ST_IF = 3,
  ST_WHILE = 4,
  ST_RETURN = 5,
  ST_EXPR = 6,
} StmtKind;

typedef struct Stmt Stmt;
typedef struct StmtList {
  Stmt** items;
  size_t len;
} StmtList;

struct Stmt {
  StmtKind kind;
  Span span;
  union {
    struct { Str name; TypeRef ty; Expr* init; } let_stmt;
    struct { Str name; Expr* value; } assign_stmt;
    struct { Expr* cond; StmtList then_body; StmtList else_body; bool has_else; } if_stmt;
    struct { Expr* cond; StmtList body; } while_stmt;
    struct { Expr* value; } ret_stmt;
    struct { Expr* expr; } expr_stmt;
  };
};

typedef struct Param {
  Str name;
  TypeRef ty;
} Param;

typedef struct ParamList {
  Param* items;
  size_t len;
} ParamList;

typedef struct FnDecl {
  Str name;
  ParamList params;
  TypeRef ret;
  bool is_extern_c;
  StmtList body; // empty if extern
  Span span;
} FnDecl;

typedef struct Program {
  FnDecl* fns;
  size_t fn_len;
} Program;
