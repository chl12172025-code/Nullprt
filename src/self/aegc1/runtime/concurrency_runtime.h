#ifndef NPRT_CONCURRENCY_RUNTIME_H
#define NPRT_CONCURRENCY_RUNTIME_H

#include <stdbool.h>

typedef struct NprtScheduler {
  const char* name;
  bool supports_cancellation;
  bool supports_coroutine;
} NprtScheduler;

bool nprt_register_scheduler(const NprtScheduler* scheduler);
const NprtScheduler* nprt_current_scheduler(void);
bool nprt_propagate_cancellation(const char* task_name);
bool nprt_generator_resume(const char* generator_name);

#endif
