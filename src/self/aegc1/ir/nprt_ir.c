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
    ir.fns[ir.len].cfg_enabled = !ast->items[i].has_cfg;
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
  return ir && ir->ir_version == 1 && ir->pointer_bits == 64 && ir->len > 0;
}
