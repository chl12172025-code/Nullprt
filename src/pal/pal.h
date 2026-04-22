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

typedef struct NprtResultBool {
  bool ok;
  bool value;
  NprtSysError err;
} NprtResultBool;

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

// ---------------- std support helpers ----------------
typedef struct NprtResultPathList {
  bool ok;
  char** items;
  size_t len;
  NprtSysError err;
} NprtResultPathList;

// Returns directory entries (UTF-8 path strings). Caller frees via nprt_std_free_path_list.
NprtResultPathList nprt_std_list_dir_simple(const char* path_utf8);
void nprt_std_free_path_list(NprtResultPathList* list);

// ---------------- security / hardware identity ----------------
typedef struct NprtHardwareIdentity {
  char disk_serial[128];
  char mac_address[64];
  char board_uuid[128];
  char bios_uuid[128];
  char tpm_id[128];
  char device_fingerprint[128];
} NprtHardwareIdentity;

typedef struct NprtResultHardwareIdentity {
  bool ok;
  NprtHardwareIdentity value;
  NprtSysError err;
} NprtResultHardwareIdentity;

// Read-only collection from public OS APIs/files.
NprtResultHardwareIdentity nprt_sec_read_hardware_identity(void);

// For developer/research use, this only overlays process-visible values.
NprtResultHardwareIdentity nprt_sec_read_hardware_identity_overlay(void);

// License binding helper using a stable normalized fingerprint.
NprtResultBool nprt_sec_verify_license_binding(const char* expected_fingerprint_utf8);

typedef struct NprtVmSignals {
  bool cpuid_hypervisor;
  bool file_registry_traits;
  bool device_driver_traits;
  bool timing_anomaly;
} NprtVmSignals;

typedef struct NprtResultVmSignals {
  bool ok;
  NprtVmSignals value;
  NprtSysError err;
} NprtResultVmSignals;

NprtResultVmSignals nprt_sec_detect_vm_signals(void);

typedef struct NprtMemAccessRequest {
  uint32_t target_pid;
  uintptr_t address;
  size_t size;
  void* buffer;
  const char* authorization_token;
} NprtMemAccessRequest;

// WARNING: This API is for authorized security research and compatibility
// testing only. Do not use for attacking third-party systems or bypassing
// license validation.
NprtResultSize nprt_sec_read_process_memory(const NprtMemAccessRequest* req);

// WARNING: This API is for authorized security research and compatibility
// testing only. Do not use for attacking third-party systems or bypassing
// license validation.
NprtResultSize nprt_sec_write_process_memory(const NprtMemAccessRequest* req);
