#include "pal.h"

#if defined(_WIN32)
  #include "windows/pal_windows.c"
#elif defined(__linux__)
  #include "linux/pal_linux.c"
#elif defined(__APPLE__)
  #include "macos/pal_macos.c"
#else
  #error Unsupported platform
#endif
