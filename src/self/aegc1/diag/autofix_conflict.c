#include <stdbool.h>
#include <stddef.h>

typedef struct A1FixRange {
  size_t start;
  size_t end;
} A1FixRange;

bool a1_diag_fix_conflict(A1FixRange a, A1FixRange b) {
  if (a.end <= b.start) return false;
  if (b.end <= a.start) return false;
  return true;
}

size_t a1_diag_count_conflicts(const A1FixRange* ranges, size_t len) {
  size_t i, j, n = 0;
  if (!ranges) return 0;
  for (i = 0; i < len; i++) {
    for (j = i + 1; j < len; j++) {
      if (a1_diag_fix_conflict(ranges[i], ranges[j])) n++;
    }
  }
  return n;
}
