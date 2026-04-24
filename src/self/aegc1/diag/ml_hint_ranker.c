#include <stddef.h>
#include <string.h>

typedef struct A1HintCandidate {
  const char* id;
  const char* text;
  double confidence;
} A1HintCandidate;

double a1_ml_hint_score(const char* diagnostic_code, const char* context) {
  double base = 0.5;
  if (!diagnostic_code) return 0.0;
  if (strcmp(diagnostic_code, "E1001") == 0) base += 0.25;
  if (context && strstr(context, "generic")) base += 0.15;
  if (context && strstr(context, "where")) base += 0.1;
  if (base > 0.99) base = 0.99;
  return base;
}

void a1_ml_hint_rank(A1HintCandidate* items, size_t len) {
  size_t i, j;
  for (i = 0; i < len; i++) {
    for (j = i + 1; j < len; j++) {
      if (items[j].confidence > items[i].confidence) {
        A1HintCandidate tmp = items[i];
        items[i] = items[j];
        items[j] = tmp;
      }
    }
  }
}
