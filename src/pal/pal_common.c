#include "pal.h"

static NprtSysErrorKind map_err_kind_default(int32_t code) {
  (void)code;
  return NPRT_SYS_OTHER;
}

static NprtSysError make_err_default(int32_t code) {
  NprtSysError e;
  e.code = code;
  e.kind = map_err_kind_default(code);
  return e;
}

NprtResultVoid nprt_err_void(int32_t code) {
  NprtResultVoid r;
  r.ok = false;
  r.err = make_err_default(code);
  return r;
}
