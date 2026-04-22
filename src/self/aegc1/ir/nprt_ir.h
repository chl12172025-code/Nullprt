#pragma once

#include "../frontend/ast.h"

typedef struct A1IrFunction {
  A1StringView name;
  A1AstItemKind source_kind;
  bool is_generic_instance;
  bool is_result_like;
  bool cfg_enabled;
  uint32_t basic_blocks;
  bool flattened_cfg;
  bool opaque_predicate;
  bool string_obfuscated;
  bool antidebug_guard;
  uint64_t integrity_tag;
} A1IrFunction;

typedef struct A1IrModule {
  uint32_t ir_version;
  uint32_t pointer_bits;
  A1IrFunction* fns;
  size_t len;
} A1IrModule;

A1IrModule a1_ir_lower(const A1AstModule* ast);
void a1_ir_free(A1IrModule* ir);
bool a1_ir_validate(const A1IrModule* ir);
