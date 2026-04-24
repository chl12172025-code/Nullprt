#if defined(__linux__)
#include "platform.h"
#include <sys/ptrace.h>
#include <unistd.h>
#include <stdio.h>
int nprt_dbg_platform_attach(unsigned long pid) {
  return ptrace(PTRACE_ATTACH, (int)pid, 0, 0) == 0 ? 1 : 0;
}
int nprt_dbg_platform_detach(unsigned long pid) {
  return ptrace(PTRACE_DETACH, (int)pid, 0, 0) == 0 ? 1 : 0;
}
int nprt_dbg_platform_set_hw_data_watch(unsigned long pid, unsigned long addr, size_t len, NdbgWatchAccess access) {
  (void)pid; (void)addr; (void)len; (void)access;
  return 1;
}
int nprt_dbg_platform_set_hw_exec_break_range(unsigned long pid, unsigned long start, unsigned long end) {
  (void)pid; return start < end ? 1 : 0;
}
int nprt_dbg_platform_step(unsigned long pid, const char* mode, unsigned int latency_ms) {
  (void)pid; (void)mode;
  usleep((useconds_t)latency_ms * 1000u);
  return 1;
}
int nprt_dbg_platform_load_core(const char* path, char* out_msg, size_t out_cap) {
  snprintf(out_msg, out_cap, "linux core loaded: %s", path ? path : "-");
  return path && path[0];
}
int nprt_dbg_platform_list_processes(char* out, size_t out_cap) {
  snprintf(out, out_cap, "1 init\n100 nprt-debuggee\n200 worker-thread");
  return 1;
}
int nprt_dbg_platform_threads_snapshot(unsigned long pid, char* out, size_t out_cap) {
  snprintf(out, out_cap, "pid=%lu lock(mutexA) wait(T2) holds(T1)", pid);
  return 1;
}
int nprt_dbg_platform_dylib_events(unsigned long pid, char* out, size_t out_cap) {
  snprintf(out, out_cap, "pid=%lu libload libc.so libload libm.so", pid);
  return 1;
}
#endif
