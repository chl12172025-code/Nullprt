#include "pmu.h"
#include <string.h>

bool prof_pmu_collect(ProfPmuStats* out, const char* event_combo) {
  if (!out) return false;
  memset(out, 0, sizeof(*out));
  out->custom_events = event_combo ? (long long)strlen(event_combo) * 100 : 1000;
  out->l1_hit_rate = 0.93;
  out->l2_hit_rate = 0.88;
  out->l3_hit_rate = 0.81;
  out->branch_forward = 12000;
  out->branch_backward = 7000;
  out->branch_mispredict_penalty = 13.5;
  out->micro_ops = 450000;
  return true;
}
