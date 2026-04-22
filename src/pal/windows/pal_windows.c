#include "../pal.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

#endif
