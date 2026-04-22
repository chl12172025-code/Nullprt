#pragma once

#include "../ir/nprt_ir.h"

typedef struct A1CEmitOptions {
  bool debug_comments;
} A1CEmitOptions;

bool a1_emit_c(const A1IrModule* ir, const char* out_c_path, const A1CEmitOptions* opt);
