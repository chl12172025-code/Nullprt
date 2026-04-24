#pragma once

#include <stdbool.h>

typedef struct ProfPmuStats {
  long long custom_events;
  double l1_hit_rate;
  double l2_hit_rate;
  double l3_hit_rate;
  long long branch_forward;
  long long branch_backward;
  double branch_mispredict_penalty;
  long long micro_ops;
} ProfPmuStats;

bool prof_pmu_collect(ProfPmuStats* out, const char* event_combo);
