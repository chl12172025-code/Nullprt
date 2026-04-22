#include "../pal.h"

#if defined(__linux__)

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

static NprtSysErrorKind map_errno_kind(int e) {
  switch (e) {
    case ENOENT: return NPRT_SYS_NOT_FOUND;
    case EACCES: return NPRT_SYS_PERMISSION_DENIED;
    case EEXIST: return NPRT_SYS_ALREADY_EXISTS;
    case EINVAL: return NPRT_SYS_INVALID_INPUT;
    case EINTR: return NPRT_SYS_INTERRUPTED;
    case ETIMEDOUT: return NPRT_SYS_TIMED_OUT;
    default: return NPRT_SYS_OTHER;
  }
}

static NprtSysError make_errno_err(int e) {
  NprtSysError er;
  er.code = (int32_t)e;
  er.kind = map_errno_kind(e);
  return er;
}

NprtResultVoid nprt_sys_exit(int32_t code) {
  _exit(code);
  NprtResultVoid r = {0};
  r.ok = true;
  return r;
}

NprtResultU64 nprt_sys_monotonic_now_ns(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    NprtResultU64 r = {0};
    r.ok = false;
    r.err = make_errno_err(errno);
    return r;
  }
  uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
  NprtResultU64 r;
  r.ok = true;
  r.value = ns;
  return r;
}

NprtResultVoid nprt_sys_sleep_ms(uint32_t ms) {
  struct timespec ts;
  ts.tv_sec = (time_t)(ms / 1000u);
  ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
  while (nanosleep(&ts, &ts) != 0) {
    if (errno == EINTR) continue;
    NprtResultVoid r = {0};
    r.ok = false;
    r.err = make_errno_err(errno);
    return r;
  }
  NprtResultVoid r;
  r.ok = true;
  return r;
}

NprtResultFile nprt_sys_open(const char* path_utf8, uint32_t mode_flags) {
  (void)path_utf8;
  int flags = 0;
  if ((mode_flags & NPRT_OPEN_READ) && (mode_flags & NPRT_OPEN_WRITE)) flags |= O_RDWR;
  else if (mode_flags & NPRT_OPEN_WRITE) flags |= O_WRONLY;
  else flags |= O_RDONLY;

  if (mode_flags & NPRT_OPEN_CREATE) flags |= O_CREAT;
  if (mode_flags & NPRT_OPEN_TRUNC) flags |= O_TRUNC;

  int fd = open(path_utf8, flags, 0644);
  NprtResultFile r = {0};
  if (fd < 0) {
    r.ok = false;
    r.err = make_errno_err(errno);
    return r;
  }
  r.ok = true;
  r.file.handle = (void*)(intptr_t)fd;
  return r;
}

NprtResultSize nprt_sys_read(NprtFile f, void* buf, size_t cap) {
  ssize_t n = read((int)(intptr_t)f.handle, buf, cap);
  NprtResultSize r = {0};
  if (n < 0) {
    r.ok = false;
    r.err = make_errno_err(errno);
    return r;
  }
  r.ok = true;
  r.value = (size_t)n;
  return r;
}

NprtResultSize nprt_sys_write(NprtFile f, const void* buf, size_t len) {
  ssize_t n = write((int)(intptr_t)f.handle, buf, len);
  NprtResultSize r = {0};
  if (n < 0) {
    r.ok = false;
    r.err = make_errno_err(errno);
    return r;
  }
  r.ok = true;
  r.value = (size_t)n;
  return r;
}

NprtResultVoid nprt_sys_close(NprtFile f) {
  NprtResultVoid r = {0};
  if (close((int)(intptr_t)f.handle) != 0) {
    r.ok = false;
    r.err = make_errno_err(errno);
    return r;
  }
  r.ok = true;
  return r;
}

#endif
