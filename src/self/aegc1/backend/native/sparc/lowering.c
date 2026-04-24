#include "../../../ir/nprt_ir.h"

#include <stdio.h>
#include <stdbool.h>

bool a1_sparc_window_lower_report(const A1IrModule* ir, const char* out_path) {
  FILE* f;
  if (!ir || !out_path) return false;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "backend=sparc\nfeature=register_window\nfunctions=%zu\nwindow_depth=8\n", ir->len);
  fclose(f);
  return true;
}
