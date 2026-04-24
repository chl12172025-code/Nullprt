#include "../ir/nprt_ir.h"

#include <stdbool.h>
#include <stdio.h>

bool a1_link_emit_postlink_rewrite_report(const A1IrModule* ir, const char* out_path) {
  FILE* f;
  if (!ir || !out_path) return false;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "postlink_version=1\nrewrites=%zu\nintegrity_fixups=enabled\n", ir->len);
  fclose(f);
  return true;
}
