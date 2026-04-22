#pragma once

#include "../frontend/ast.h"

typedef struct A1MonomorphizeResult {
  bool ok;
  size_t instantiated_items;
} A1MonomorphizeResult;

A1MonomorphizeResult a1_monomorphize_prep(const A1AstModule* m);
