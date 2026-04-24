#pragma once

#include "../frontend/ast.h"

typedef struct A1IrFunction {
  A1StringView name;
  A1AstItemKind source_kind;
  bool is_generic_instance;
  bool is_result_like;
  bool is_async;
  bool is_generator;
  bool is_coroutine;
  bool has_contract;
  bool has_invariant;
  bool cfg_enabled;
  uint64_t feature_bits;
  uint32_t nested_pattern_depth_limit;
  uint32_t diagnostics_emitted;
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
  uint64_t feature_bits;
  uint32_t nested_pattern_depth_limit;
  A1IrFunction* fns;
  size_t len;
} A1IrModule;

A1IrModule a1_ir_lower(const A1AstModule* ast);
void a1_ir_free(A1IrModule* ir);
bool a1_ir_validate(const A1IrModule* ir);
