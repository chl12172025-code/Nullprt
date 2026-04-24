#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool prof_load_benchmark_env(const char* path, char* out_env, size_t out_cap) {
  FILE* f;
  size_t n;
  if (!path || !out_env || out_cap == 0) return false;
  f = fopen(path, "rb");
  if (!f) return false;
  n = fread(out_env, 1, out_cap - 1, f);
  out_env[n] = 0;
  fclose(f);
  return true;
}

bool prof_budget_alert(double measured_us, double budget_us, char* out, size_t out_cap) {
  if (!out || out_cap == 0) return false;
  if (measured_us > budget_us) {
    snprintf(out, out_cap, "budget-exceeded measured=%.2f budget=%.2f", measured_us, budget_us);
  } else {
    snprintf(out, out_cap, "budget-ok measured=%.2f budget=%.2f", measured_us, budget_us);
  }
  return true;
}
