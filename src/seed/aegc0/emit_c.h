#pragma once

#include "ast.h"

typedef struct CEmitOptions {
  const char* out_c_path;
} CEmitOptions;

bool emit_c_program(const Program* prog, const CEmitOptions* opt);
