#if defined(_WIN32)
#include <windows.h>
int nprt_dbg_platform_attach(unsigned long pid) {
  return DebugActiveProcess((DWORD)pid) ? 1 : 0;
}
#endif
