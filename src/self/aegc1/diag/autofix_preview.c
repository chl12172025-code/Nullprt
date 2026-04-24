#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool a1_diag_emit_autofix_preview(const char* file, const char* before, const char* after, const char* out_path) {
  FILE* f;
  if (!file || !before || !after || !out_path) return false;
  f = fopen(out_path, "wb");
  if (!f) return false;
  fprintf(f, "file=%s\n--- before ---\n%s\n--- after ---\n%s\n", file, before, after);
  fclose(f);
  return true;
}

bool a1_diag_has_autofix_diff(const char* before, const char* after) {
  if (!before || !after) return false;
  return strcmp(before, after) != 0;
}
