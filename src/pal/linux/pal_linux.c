#include "../pal.h"

#if defined(__linux__)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void trim_line(char* s) {
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) {
    s[--n] = '\0';
  }
}

static bool read_file_line(const char* path, char* out, size_t cap) {
  FILE* f = fopen(path, "rb");
  if (!f) return false;
  if (!fgets(out, (int)cap, f)) {
    fclose(f);
    return false;
  }
  fclose(f);
  trim_line(out);
  return out[0] != '\0';
}

static bool read_cmd_line(const char* cmd, char* out, size_t cap) {
  FILE* p = popen(cmd, "r");
  if (!p) return false;
  bool ok = fgets(out, (int)cap, p) != NULL;
  pclose(p);
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
  read_file_line("/sys/block/sda/device/serial", r.value.disk_serial, sizeof(r.value.disk_serial));
  read_file_line("/sys/class/net/eth0/address", r.value.mac_address, sizeof(r.value.mac_address));
  read_file_line("/sys/class/dmi/id/board_uuid", r.value.board_uuid, sizeof(r.value.board_uuid));
  read_file_line("/sys/class/dmi/id/product_uuid", r.value.bios_uuid, sizeof(r.value.bios_uuid));
  if (!read_cmd_line("cat /sys/class/tpm/tpm0/device/description 2>/dev/null", r.value.tpm_id, sizeof(r.value.tpm_id))) {
    strncpy(r.value.tpm_id, "tpm-unavailable", sizeof(r.value.tpm_id) - 1);
  }
  if (r.value.disk_serial[0] == '\0') strncpy(r.value.disk_serial, "disk-unknown", sizeof(r.value.disk_serial) - 1);
  if (r.value.mac_address[0] == '\0') strncpy(r.value.mac_address, "mac-unknown", sizeof(r.value.mac_address) - 1);
  if (r.value.board_uuid[0] == '\0') strncpy(r.value.board_uuid, "board-unknown", sizeof(r.value.board_uuid) - 1);
  if (r.value.bios_uuid[0] == '\0') strncpy(r.value.bios_uuid, "bios-unknown", sizeof(r.value.bios_uuid) - 1);
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
  r.value = expected_fingerprint_utf8 && strcmp(id.value.device_fingerprint, expected_fingerprint_utf8) == 0;
  return r;
}

NprtResultVmSignals nprt_sec_detect_vm_signals(void) {
  NprtResultVmSignals r = {0};
  r.ok = true;
  char line[256] = {0};
  r.value.cpuid_hypervisor = read_cmd_line("grep -m1 -i hypervisor /proc/cpuinfo 2>/dev/null", line, sizeof(line));
  r.value.file_registry_traits = access("/sys/class/dmi/id/product_name", R_OK) == 0;
  r.value.device_driver_traits = read_cmd_line("lsmod 2>/dev/null | grep -Ei 'vbox|vmware|kvm' | head -n1", line, sizeof(line));
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
    r.err = make_errno_err(EINVAL);
    return r;
  }
  r.ok = false;
  r.err = make_errno_err(ENOTSUP);
  return r;
}

NprtResultSize nprt_sec_write_process_memory(const NprtMemAccessRequest* req) {
  NprtResultSize r = {0};
  if (!req || !req->buffer || req->size == 0 || !req->authorization_token || !req->authorization_token[0]) {
    r.ok = false;
    r.err = make_errno_err(EINVAL);
    return r;
  }
  r.ok = false;
  r.err = make_errno_err(ENOTSUP);
  return r;
}

#endif
