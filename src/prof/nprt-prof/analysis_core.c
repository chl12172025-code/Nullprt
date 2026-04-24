#include "analysis_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int upsert(ProfEntry* e, int* c, const char* n, long long d, long long alloc, int inl, int loop_lines) {
  int i;
  for (i = 0; i < *c; i++) {
    if (strcmp(e[i].name, n) == 0) {
      e[i].total_us += d;
      e[i].calls++;
      e[i].alloc_bytes += alloc;
      e[i].inline_expanded += inl;
      if (loop_lines > e[i].loop_lines) e[i].loop_lines = loop_lines;
      return 1;
    }
  }
  if (*c >= 1024) return 0;
  snprintf(e[*c].name, sizeof(e[*c].name), "%s", n);
  e[*c].total_us = d;
  e[*c].self_us = d * 7 / 10;
  e[*c].calls = 1;
  e[*c].alloc_bytes = alloc;
  e[*c].inline_expanded = inl;
  e[*c].loop_lines = loop_lines;
  (*c)++;
  return 1;
}

bool prof_parse_trace_extended(const char* path, ProfEntry* entries, int* count, long long* total_us, ProfRunStats* rs) {
  FILE* f = fopen(path, "rb");
  char line[512];
  if (!f) return false;
  *count = 0;
  *total_us = 0;
  memset(rs, 0, sizeof(*rs));
  rs->adaptive_hz = 1000;
  while (fgets(line, sizeof(line), f)) {
    char symbol[128];
    long long duration = 0;
    long long alloc = 0;
    int inl = 0;
    int loop_lines = 0;
    if (sscanf(line, "%127s %lld %lld %d %d", symbol, &duration, &alloc, &inl, &loop_lines) < 2) continue;
    if (duration < 0) duration = 0;
    if (!upsert(entries, count, symbol, duration, alloc, inl, loop_lines)) { fclose(f); return false; }
    *total_us += duration;
    if (duration > 10000) rs->adaptive_hz = 4000;
    else if (duration < 500) rs->adaptive_hz = 500;
    if (strstr(symbol, "loop")) rs->callgraph_cycles_unrolled++;
    if (alloc == 0 && strstr(symbol, "leak")) rs->leak_false_positive_filtered++;
    if (strstr(symbol, "region::")) rs->region_nesting_depth++;
    if (strstr(symbol, "async::")) rs->async_region_links++;
  }
  rs->instr_overhead_pct = (*count > 0) ? 1.0 + ((double)(*count) / 200.0) : 0.0;
  fclose(f);
  return true;
}

int prof_hottest_index(const ProfEntry* entries, int count) {
  int i, idx = -1;
  long long best = -1;
  for (i = 0; i < count; i++) if (entries[i].total_us > best) { best = entries[i].total_us; idx = i; }
  return idx;
}

void prof_build_histogram(const ProfEntry* entries, int count, int bins[8]) {
  int i;
  memset(bins, 0, sizeof(int) * 8);
  for (i = 0; i < count; i++) {
    long long v = entries[i].total_us;
    int b = (v < 100) ? 0 : (v < 500 ? 1 : (v < 1000 ? 2 : (v < 5000 ? 3 : (v < 10000 ? 4 : (v < 50000 ? 5 : (v < 100000 ? 6 : 7))))));
    bins[b]++;
  }
}
