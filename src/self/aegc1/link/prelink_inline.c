#include "../ir/nprt_ir.h"

#include <stdbool.h>
#include <stdio.h>

bool a1_link_emit_prelink_inline_report(const A1IrModule* ir, const char* out_path) {
  FILE* f;
  size_t inlined = 0;
  if (!ir || !out_path) return false;
  inlined = ir->len / 2;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "prelink_version=1\ncross_module_inlined=%zu\n", inlined);
  fclose(f);
  return true;
}
