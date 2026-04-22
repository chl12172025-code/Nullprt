#if defined(__linux__)
#include <sys/ptrace.h>
int nprt_dbg_platform_attach(unsigned long pid) {
  return ptrace(PTRACE_ATTACH, (int)pid, 0, 0) == 0 ? 1 : 0;
}
#endif
