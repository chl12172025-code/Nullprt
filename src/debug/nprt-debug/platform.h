#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum NdbgWatchAccess {
  NDBG_WATCH_READ = 1,
  NDBG_WATCH_WRITE = 2,
  NDBG_WATCH_RW = 3
} NdbgWatchAccess;

int nprt_dbg_platform_attach(unsigned long pid);
int nprt_dbg_platform_detach(unsigned long pid);
int nprt_dbg_platform_set_hw_data_watch(unsigned long pid, unsigned long addr, size_t len, NdbgWatchAccess access);
int nprt_dbg_platform_set_hw_exec_break_range(unsigned long pid, unsigned long start, unsigned long end);
int nprt_dbg_platform_step(unsigned long pid, const char* mode, unsigned int latency_ms);
int nprt_dbg_platform_load_core(const char* path, char* out_msg, size_t out_cap);
int nprt_dbg_platform_list_processes(char* out, size_t out_cap);
int nprt_dbg_platform_threads_snapshot(unsigned long pid, char* out, size_t out_cap);
int nprt_dbg_platform_dylib_events(unsigned long pid, char* out, size_t out_cap);
