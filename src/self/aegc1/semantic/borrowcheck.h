#pragma once

#include "../frontend/ast.h"

typedef struct A1BorrowcheckResult {
  bool ok;
  size_t violations;
} A1BorrowcheckResult;

A1BorrowcheckResult a1_borrowcheck_module(const A1AstModule* m);
