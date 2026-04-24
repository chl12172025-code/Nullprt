#include "../../../ir/nprt_ir.h"

#include <stdio.h>
#include <stdbool.h>

bool a1_mips_delay_slot_lower_report(const A1IrModule* ir, const char* out_path) {
  FILE* f;
  if (!ir || !out_path) return false;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "backend=mips\nfeature=delay_slot\nfunctions=%zu\nscheduler=enabled\n", ir->len);
  fclose(f);
  return true;
}
