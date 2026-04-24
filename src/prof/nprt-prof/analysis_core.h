#pragma once

#include <stdbool.h>

typedef struct ProfEntry {
  char name[128];
  long long total_us;
  long long calls;
  long long self_us;
  long long alloc_bytes;
  int inline_expanded;
  int loop_lines;
} ProfEntry;

typedef struct ProfRunStats {
  int adaptive_hz;
  double instr_overhead_pct;
  int callgraph_cycles_unrolled;
  int leak_false_positive_filtered;
  int region_nesting_depth;
  int async_region_links;
} ProfRunStats;

bool prof_parse_trace_extended(const char* path, ProfEntry* entries, int* count, long long* total_us, ProfRunStats* rs);
int prof_hottest_index(const ProfEntry* entries, int count);
void prof_build_histogram(const ProfEntry* entries, int count, int bins[8]);
