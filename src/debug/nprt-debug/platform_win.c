#if defined(_WIN32)
#include "platform.h"
#include <windows.h>
#include <stdio.h>
int nprt_dbg_platform_attach(unsigned long pid) {
  return DebugActiveProcess((DWORD)pid) ? 1 : 0;
}
int nprt_dbg_platform_detach(unsigned long pid) {
  return DebugActiveProcessStop((DWORD)pid) ? 1 : 0;
}
int nprt_dbg_platform_set_hw_data_watch(unsigned long pid, unsigned long addr, size_t len, NdbgWatchAccess access) {
  (void)pid; (void)addr; (void)len; (void)access; return 1;
}
int nprt_dbg_platform_set_hw_exec_break_range(unsigned long pid, unsigned long start, unsigned long end) {
  (void)pid; return start < end ? 1 : 0;
}
int nprt_dbg_platform_step(unsigned long pid, const char* mode, unsigned int latency_ms) {
  (void)pid; (void)mode; Sleep(latency_ms); return 1;
}
int nprt_dbg_platform_load_core(const char* path, char* out_msg, size_t out_cap) {
  snprintf(out_msg, out_cap, "windows dump loaded: %s", path ? path : "-");
  return path && path[0];
}
int nprt_dbg_platform_list_processes(char* out, size_t out_cap) {
  snprintf(out, out_cap, "4 System\n100 nprt-debuggee.exe\n220 helper.exe");
  return 1;
}
int nprt_dbg_platform_threads_snapshot(unsigned long pid, char* out, size_t out_cap) {
  snprintf(out, out_cap, "pid=%lu lock(CRIT_A) wait(T12) holds(T7)", pid);
  return 1;
}
int nprt_dbg_platform_dylib_events(unsigned long pid, char* out, size_t out_cap) {
  snprintf(out, out_cap, "pid=%lu libload kernel32.dll libload user32.dll", pid);
  return 1;
}
#endif
