#include "../ir/nprt_ir.h"

#include <stdbool.h>
#include <stdio.h>

bool a1_link_emit_lazy_stub_table(const A1IrModule* ir, const char* out_path) {
  FILE* f;
  if (!ir || !out_path) return false;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "stub_table_version=1\nentries=%zu\npolicy=lazy\n", ir->len);
  fclose(f);
  return true;
}
