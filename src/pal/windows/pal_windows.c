#include "../pal.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NprtSysErrorKind map_win32_kind(DWORD code) {
  switch (code) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      return NPRT_SYS_NOT_FOUND;
    case ERROR_ACCESS_DENIED:
      return NPRT_SYS_PERMISSION_DENIED;
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS:
      return NPRT_SYS_ALREADY_EXISTS;
    default:
      return NPRT_SYS_OTHER;
  }
}

static NprtSysError make_win32_err(DWORD code) {
  NprtSysError e;
  e.code = (int32_t)code;
  e.kind = map_win32_kind(code);
  return e;
}

NprtResultVoid nprt_sys_exit(int32_t code) {
  ExitProcess((UINT)code);
  NprtResultVoid r;
  r.ok = true;
  return r;
}

NprtResultU64 nprt_sys_monotonic_now_ns(void) {
  LARGE_INTEGER freq;
  LARGE_INTEGER now;
  if (!QueryPerformanceFrequency(&freq) || !QueryPerformanceCounter(&now)) {
    DWORD e = GetLastError();
    NprtResultU64 r = {0};
    r.ok = false;
    r.err = make_win32_err(e);
    return r;
  }
  // ns = now * 1e9 / freq
  uint64_t ns = (uint64_t)((now.QuadPart * 1000000000ull) / (uint64_t)freq.QuadPart);
  NprtResultU64 r;
  r.ok = true;
  r.value = ns;
  return r;
}

NprtResultVoid nprt_sys_sleep_ms(uint32_t ms) {
  Sleep(ms);
  NprtResultVoid r;
  r.ok = true;
  return r;
}

static wchar_t* utf8_to_wide_tmp(const char* s, int* out_len) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  if (n <= 0) return NULL;
  wchar_t* w = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, sizeof(wchar_t) * (size_t)n);
  if (!w) return NULL;
  if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n)) {
    HeapFree(GetProcessHeap(), 0, w);
    return NULL;
  }
  if (out_len) *out_len = n;
  return w;
}

NprtResultFile nprt_sys_open(const char* path_utf8, uint32_t mode_flags) {
  NprtResultFile r = {0};
  int wlen = 0;
  wchar_t* wpath = utf8_to_wide_tmp(path_utf8, &wlen);
  if (!wpath) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    return r;
  }

  DWORD access = 0;
  if (mode_flags & NPRT_OPEN_READ) access |= GENERIC_READ;
  if (mode_flags & NPRT_OPEN_WRITE) access |= GENERIC_WRITE;

  DWORD creation = OPEN_EXISTING;
  if ((mode_flags & NPRT_OPEN_CREATE) && (mode_flags & NPRT_OPEN_TRUNC)) creation = CREATE_ALWAYS;
  else if (mode_flags & NPRT_OPEN_CREATE) creation = OPEN_ALWAYS;
  else if (mode_flags & NPRT_OPEN_TRUNC) creation = TRUNCATE_EXISTING;

  HANDLE h = CreateFileW(wpath, access, FILE_SHARE_READ, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
  HeapFree(GetProcessHeap(), 0, wpath);
  if (h == INVALID_HANDLE_VALUE) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    return r;
  }
  r.ok = true;
  r.file.handle = (void*)h;
  return r;
}

NprtResultSize nprt_sys_read(NprtFile f, void* buf, size_t cap) {
  NprtResultSize r = {0};
  DWORD got = 0;
  if (!ReadFile((HANDLE)f.handle, buf, (DWORD)cap, &got, NULL)) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    return r;
  }
  r.ok = true;
  r.value = (size_t)got;
  return r;
}

NprtResultSize nprt_sys_write(NprtFile f, const void* buf, size_t len) {
  NprtResultSize r = {0};
  DWORD wrote = 0;
  if (!WriteFile((HANDLE)f.handle, buf, (DWORD)len, &wrote, NULL)) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    return r;
  }
  r.ok = true;
  r.value = (size_t)wrote;
  return r;
}

NprtResultVoid nprt_sys_close(NprtFile f) {
  NprtResultVoid r = {0};
  if (!CloseHandle((HANDLE)f.handle)) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    return r;
  }
  r.ok = true;
  return r;
}

static void trim_line(char* s) {
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = '\0';
}

static bool read_cmd_line(const char* cmd, char* out, size_t cap) {
  FILE* p = _popen(cmd, "r");
  if (!p) return false;
  bool ok = fgets(out, (int)cap, p) != NULL;
  _pclose(p);
  if (!ok) return false;
  trim_line(out);
  return out[0] != '\0';
}

static uint64_t fnv1a64(const char* s) {
  uint64_t h = 1469598103934665603ull;
  while (s && *s) {
    h ^= (uint8_t)(*s++);
    h *= 1099511628211ull;
  }
  return h;
}

static void fingerprint_from_identity(NprtHardwareIdentity* id) {
  char combined[1024];
  snprintf(combined, sizeof(combined), "%s|%s|%s|%s|%s",
           id->disk_serial, id->mac_address, id->board_uuid, id->bios_uuid, id->tpm_id);
  snprintf(id->device_fingerprint, sizeof(id->device_fingerprint), "%016llx",
           (unsigned long long)fnv1a64(combined));
}

NprtResultHardwareIdentity nprt_sec_read_hardware_identity(void) {
  NprtResultHardwareIdentity r = {0};
  r.ok = true;
  if (!read_cmd_line("wmic diskdrive get SerialNumber /value | find \"=\"", r.value.disk_serial, sizeof(r.value.disk_serial))) {
    strncpy(r.value.disk_serial, "disk-unknown", sizeof(r.value.disk_serial) - 1);
  }
  if (!read_cmd_line("wmic nic where \"MACAddress is not NULL\" get MACAddress /value | find \"=\"", r.value.mac_address, sizeof(r.value.mac_address))) {
    strncpy(r.value.mac_address, "mac-unknown", sizeof(r.value.mac_address) - 1);
  }
  if (!read_cmd_line("wmic baseboard get SerialNumber /value | find \"=\"", r.value.board_uuid, sizeof(r.value.board_uuid))) {
    strncpy(r.value.board_uuid, "board-unknown", sizeof(r.value.board_uuid) - 1);
  }
  if (!read_cmd_line("wmic csproduct get UUID /value | find \"=\"", r.value.bios_uuid, sizeof(r.value.bios_uuid))) {
    strncpy(r.value.bios_uuid, "bios-unknown", sizeof(r.value.bios_uuid) - 1);
  }
  if (!read_cmd_line("powershell -NoProfile -Command \"Get-Tpm | Select-Object -ExpandProperty TpmPresent\"", r.value.tpm_id, sizeof(r.value.tpm_id))) {
    strncpy(r.value.tpm_id, "tpm-unavailable", sizeof(r.value.tpm_id) - 1);
  }
  fingerprint_from_identity(&r.value);
  return r;
}

NprtResultHardwareIdentity nprt_sec_read_hardware_identity_overlay(void) {
  NprtResultHardwareIdentity r = nprt_sec_read_hardware_identity();
  const char* f = getenv("NPRT_SIM_DISK_SERIAL");
  if (f && f[0]) strncpy(r.value.disk_serial, f, sizeof(r.value.disk_serial) - 1);
  f = getenv("NPRT_SIM_MAC_ADDRESS");
  if (f && f[0]) strncpy(r.value.mac_address, f, sizeof(r.value.mac_address) - 1);
  f = getenv("NPRT_SIM_BOARD_UUID");
  if (f && f[0]) strncpy(r.value.board_uuid, f, sizeof(r.value.board_uuid) - 1);
  f = getenv("NPRT_SIM_BIOS_UUID");
  if (f && f[0]) strncpy(r.value.bios_uuid, f, sizeof(r.value.bios_uuid) - 1);
  f = getenv("NPRT_SIM_TPM_ID");
  if (f && f[0]) strncpy(r.value.tpm_id, f, sizeof(r.value.tpm_id) - 1);
  fingerprint_from_identity(&r.value);
  return r;
}

NprtResultBool nprt_sec_verify_license_binding(const char* expected_fingerprint_utf8) {
  NprtResultBool r = {0};
  NprtResultHardwareIdentity id = nprt_sec_read_hardware_identity_overlay();
  r.ok = id.ok;
  r.value = expected_fingerprint_utf8 && strstr(id.value.device_fingerprint, expected_fingerprint_utf8) != NULL;
  return r;
}

NprtResultVmSignals nprt_sec_detect_vm_signals(void) {
  NprtResultVmSignals r = {0};
  char line[256] = {0};
  r.ok = true;
  r.value.cpuid_hypervisor = read_cmd_line("wmic cpu get Caption | findstr /I /C:\"Virtual\" /C:\"Hyper-V\"", line, sizeof(line));
  r.value.file_registry_traits = read_cmd_line("reg query \"HKLM\\HARDWARE\\DESCRIPTION\\System\" /v SystemBiosVersion | findstr /I /C:\"VBOX\" /C:\"VMWARE\"", line, sizeof(line));
  r.value.device_driver_traits = read_cmd_line("driverquery | findstr /I /C:\"vbox\" /C:\"vmware\"", line, sizeof(line));
  NprtResultU64 t1 = nprt_sys_monotonic_now_ns();
  nprt_sys_sleep_ms(2);
  NprtResultU64 t2 = nprt_sys_monotonic_now_ns();
  r.value.timing_anomaly = t1.ok && t2.ok && (t2.value - t1.value > 30000000ull);
  return r;
}

NprtResultSize nprt_sec_read_process_memory(const NprtMemAccessRequest* req) {
  NprtResultSize r = {0};
  if (!req || !req->buffer || req->size == 0 || !req->authorization_token || !req->authorization_token[0]) {
    r.ok = false;
    r.err = make_win32_err(ERROR_INVALID_PARAMETER);
    return r;
  }
  HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, req->target_pid);
  if (!h) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    return r;
  }
  SIZE_T read = 0;
  if (!ReadProcessMemory(h, (LPCVOID)req->address, req->buffer, req->size, &read)) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    CloseHandle(h);
    return r;
  }
  CloseHandle(h);
  r.ok = true;
  r.value = (size_t)read;
  return r;
}

NprtResultSize nprt_sec_write_process_memory(const NprtMemAccessRequest* req) {
  NprtResultSize r = {0};
  if (!req || !req->buffer || req->size == 0 || !req->authorization_token || !req->authorization_token[0]) {
    r.ok = false;
    r.err = make_win32_err(ERROR_INVALID_PARAMETER);
    return r;
  }
  HANDLE h = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, req->target_pid);
  if (!h) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    return r;
  }
  SIZE_T wrote = 0;
  if (!WriteProcessMemory(h, (LPVOID)req->address, req->buffer, req->size, &wrote)) {
    r.ok = false;
    r.err = make_win32_err(GetLastError());
    CloseHandle(h);
    return r;
  }
  CloseHandle(h);
  r.ok = true;
  r.value = (size_t)wrote;
  return r;
}

#endif
