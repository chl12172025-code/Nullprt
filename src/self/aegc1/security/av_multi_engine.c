#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef enum A1ScanVerdict {
  A1_SCAN_CLEAN = 0,
  A1_SCAN_SUSPICIOUS = 1,
  A1_SCAN_MALICIOUS = 2
} A1ScanVerdict;

A1ScanVerdict a1_av_aggregate(const A1ScanVerdict* verdicts, size_t len) {
  size_t i;
  A1ScanVerdict worst = A1_SCAN_CLEAN;
  if (!verdicts) return A1_SCAN_SUSPICIOUS;
  for (i = 0; i < len; i++) {
    if (verdicts[i] > worst) worst = verdicts[i];
  }
  return worst;
}

bool a1_av_engine_supported(const char* name) {
  if (!name) return false;
  return strcmp(name, "clamav") == 0 ||
         strcmp(name, "defender") == 0 ||
         strcmp(name, "yara") == 0;
}
