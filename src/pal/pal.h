#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum NprtSysErrorKind {
  NPRT_SYS_OTHER = 0,
  NPRT_SYS_NOT_FOUND,
  NPRT_SYS_PERMISSION_DENIED,
  NPRT_SYS_INVALID_INPUT,
  NPRT_SYS_TIMED_OUT,
  NPRT_SYS_INTERRUPTED,
  NPRT_SYS_ALREADY_EXISTS,
  NPRT_SYS_UNSUPPORTED,
} NprtSysErrorKind;

typedef struct NprtSysError {
  NprtSysErrorKind kind;
  int32_t code; // platform native errno/GetLastError
} NprtSysError;

typedef struct NprtResultVoid {
  bool ok;
  NprtSysError err;
} NprtResultVoid;

typedef struct NprtResultU64 {
  bool ok;
  uint64_t value;
  NprtSysError err;
} NprtResultU64;

typedef struct NprtResultI32 {
  bool ok;
  int32_t value;
  NprtSysError err;
} NprtResultI32;

// ---------------- process ----------------
NprtResultVoid nprt_sys_exit(int32_t code);

// ---------------- time ----------------
// monotonic_now returns nanoseconds since an unspecified epoch.
NprtResultU64 nprt_sys_monotonic_now_ns(void);
NprtResultVoid nprt_sys_sleep_ms(uint32_t ms);

// ---------------- fs ----------------
typedef struct NprtFile {
  void* handle;
} NprtFile;

typedef enum NprtOpenMode {
  NPRT_OPEN_READ = 1,
  NPRT_OPEN_WRITE = 2,
  NPRT_OPEN_CREATE = 4,
  NPRT_OPEN_TRUNC = 8,
} NprtOpenMode;

typedef struct NprtResultFile {
  bool ok;
  NprtFile file;
  NprtSysError err;
} NprtResultFile;

typedef struct NprtResultSize {
  bool ok;
  size_t value;
  NprtSysError err;
} NprtResultSize;

NprtResultFile nprt_sys_open(const char* path_utf8, uint32_t mode_flags);
NprtResultSize nprt_sys_read(NprtFile f, void* buf, size_t cap);
NprtResultSize nprt_sys_write(NprtFile f, const void* buf, size_t len);
NprtResultVoid nprt_sys_close(NprtFile f);
