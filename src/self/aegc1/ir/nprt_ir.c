#include "nprt_ir.h"

#include <stdlib.h>
#include <string.h>

static uint64_t fnv1a64_sv(A1StringView sv) {
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < sv.len; i++) {
    h ^= (unsigned char)sv.ptr[i];
    h *= 1099511628211ull;
  }
  return h;
}

A1IrModule a1_ir_lower(const A1AstModule* ast) {
  A1IrModule ir;
  ir.ir_version = 1;
  ir.pointer_bits = 64;
  ir.feature_bits = ast->feature_bits;
  ir.nested_pattern_depth_limit = ast->nested_pattern_depth_limit;
  ir.fns = NULL;
  ir.len = 0;
  for (size_t i = 0; i < ast->len; i++) {
    if (ast->items[i].kind != A1_ITEM_FN && ast->items[i].kind != A1_ITEM_STRUCT && ast->items[i].kind != A1_ITEM_ENUM) continue;
    A1IrFunction* n = (A1IrFunction*)realloc(ir.fns, sizeof(A1IrFunction) * (ir.len + 1));
    if (!n) break;
    ir.fns = n;
    ir.fns[ir.len].name = ast->items[i].name;
    ir.fns[ir.len].source_kind = ast->items[i].kind;
    ir.fns[ir.len].is_generic_instance = ast->items[i].is_generic;
    ir.fns[ir.len].is_result_like = (ast->items[i].name.len >= 6 && memcmp(ast->items[i].name.ptr, "Result", 6) == 0);
    ir.fns[ir.len].is_async = ast->items[i].is_async;
    ir.fns[ir.len].is_generator = ast->items[i].is_generator;
    ir.fns[ir.len].is_coroutine = ast->items[i].is_coroutine;
    ir.fns[ir.len].has_contract = ast->items[i].has_contract;
    ir.fns[ir.len].has_invariant = ast->items[i].has_invariant;
    ir.fns[ir.len].cfg_enabled = !ast->items[i].has_cfg;
    ir.fns[ir.len].feature_bits = ast->items[i].feature_bits;
    ir.fns[ir.len].nested_pattern_depth_limit = ast->nested_pattern_depth_limit;
    ir.fns[ir.len].diagnostics_emitted = 0;
    ir.fns[ir.len].basic_blocks = 1;
    ir.fns[ir.len].flattened_cfg = false;
    ir.fns[ir.len].opaque_predicate = false;
    ir.fns[ir.len].string_obfuscated = false;
    ir.fns[ir.len].antidebug_guard = false;
    ir.fns[ir.len].integrity_tag = fnv1a64_sv(ast->items[i].name) ^ 0x9E3779B97F4A7C15ull;
    ir.len++;
  }
  return ir;
}

void a1_ir_free(A1IrModule* ir) {
  free(ir->fns);
  ir->fns = NULL;
  ir->len = 0;
}

bool a1_ir_validate(const A1IrModule* ir) {
  if (!ir || ir->ir_version != 1 || ir->pointer_bits != 64 || ir->len == 0) return false;
  if (ir->nested_pattern_depth_limit == 0 || ir->nested_pattern_depth_limit > 256) return false;
  for (size_t i = 0; i < ir->len; i++) {
    const A1IrFunction* fn = &ir->fns[i];
    if (fn->is_async && !fn->cfg_enabled) return false;
    if (fn->has_contract && fn->name.len == 0) return false;
  }
  return true;
}
