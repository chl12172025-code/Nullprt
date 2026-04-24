#include "concurrency_runtime.h"

#include <stdio.h>
#include <string.h>

static NprtScheduler g_scheduler = {"default", true, true};

bool nprt_register_scheduler(const NprtScheduler* scheduler) {
  if (!scheduler || !scheduler->name || !scheduler->name[0]) return false;
  g_scheduler = *scheduler;
  return true;
}

const NprtScheduler* nprt_current_scheduler(void) {
  return &g_scheduler;
}

bool nprt_propagate_cancellation(const char* task_name) {
  if (!g_scheduler.supports_cancellation) return false;
  if (!task_name || !task_name[0]) return false;
  fprintf(stderr, "nprt runtime: cancellation propagated to task=%s via scheduler=%s\n", task_name, g_scheduler.name);
  return true;
}

bool nprt_generator_resume(const char* generator_name) {
  if (!g_scheduler.supports_coroutine) return false;
  if (!generator_name || !generator_name[0]) return false;
  fprintf(stderr, "nprt runtime: generator resumed name=%s scheduler=%s\n", generator_name, g_scheduler.name);
  return true;
}
