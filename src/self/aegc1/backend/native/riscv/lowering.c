#include "../../../ir/nprt_ir.h"

#include <stdio.h>
#include <stdbool.h>

bool a1_riscv_rvv_lower_report(const A1IrModule* ir, const char* out_path) {
  FILE* f;
  if (!ir || !out_path) return false;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "backend=riscv\nfeature=rvv\nfunctions=%zu\nvsetvl=enabled\n", ir->len);
  fclose(f);
  return true;
}
