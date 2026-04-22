#include "dump.h"

#include <stdio.h>

bool a1_ir_dump_text(const A1IrModule* ir, const char* out_path) {
  FILE* f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "module v%u ptr=%u {\n", ir->ir_version, ir->pointer_bits);
  for (size_t i = 0; i < ir->len; i++) {
    const char* sk = "fn";
    if (ir->fns[i].source_kind == A1_ITEM_STRUCT) sk = "struct";
    else if (ir->fns[i].source_kind == A1_ITEM_ENUM) sk = "enum";
    fprintf(
      f,
      "  item=%s name=%.*s bb=%u generic=%s result_like=%s cfg_enabled=%s\n",
      sk,
      (int)ir->fns[i].name.len,
      ir->fns[i].name.ptr,
      ir->fns[i].basic_blocks,
      ir->fns[i].is_generic_instance ? "true" : "false",
      ir->fns[i].is_result_like ? "true" : "false",
      ir->fns[i].cfg_enabled ? "true" : "false");
    fprintf(
      f,
      "    protect(flattened_cfg=%s opaque_predicate=%s string_obfuscated=%s antidebug_guard=%s integrity_tag=%llu)\n",
      ir->fns[i].flattened_cfg ? "true" : "false",
      ir->fns[i].opaque_predicate ? "true" : "false",
      ir->fns[i].string_obfuscated ? "true" : "false",
      ir->fns[i].antidebug_guard ? "true" : "false",
      (unsigned long long)ir->fns[i].integrity_tag);
  }
  fprintf(f, "}\n");
  fclose(f);
  return true;
}
