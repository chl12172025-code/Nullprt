#pragma once

#include "../frontend/ast.h"

typedef struct A1TypecheckResult {
  bool ok;
  size_t errors;
} A1TypecheckResult;

A1TypecheckResult a1_typecheck_module(const A1AstModule* m);
