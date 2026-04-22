#pragma once

#include "nprt_ir.h"

typedef struct A1ProtectOptions {
  bool enable;
  bool flatten_cfg;
  bool opaque_predicate;
  bool string_obfuscation;
  bool integrity_hash;
  bool antidebug;
  const char* optimization_profile;
} A1ProtectOptions;

void a1_protect_options_default(A1ProtectOptions* opt);
bool a1_ir_apply_protection_passes(A1IrModule* ir, const A1ProtectOptions* opt);
