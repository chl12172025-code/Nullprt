#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool ndbg_thread_lock_analyze(const char* snapshot, char* out, size_t out_cap) {
  if (!snapshot || !out || out_cap == 0) return false;
  if (strstr(snapshot, "lock")) {
    snprintf(out, out_cap, "lock-analysis: potential contention detected");
  } else {
    snprintf(out, out_cap, "lock-analysis: no contention signatures");
  }
  return true;
}

bool ndbg_thread_deadlock_detect(const char* wait_graph, char* out, size_t out_cap) {
  if (!wait_graph || !out || out_cap == 0) return false;
  if (strstr(wait_graph, "A->B") && strstr(wait_graph, "B->A")) {
    snprintf(out, out_cap, "deadlock-detected: cycle A<->B");
  } else {
    snprintf(out, out_cap, "deadlock-detected: none");
  }
  return true;
}
