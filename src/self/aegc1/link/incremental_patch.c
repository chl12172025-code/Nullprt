#include "../ir/nprt_ir.h"

#include <stdbool.h>
#include <stdio.h>

bool a1_link_emit_incremental_patch(const A1IrModule* ir, const char* out_path) {
  FILE* f;
  if (!ir || !out_path) return false;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "patch_version=1\nchanged_functions=%zu\n", ir->len);
  fclose(f);
  return true;
}
