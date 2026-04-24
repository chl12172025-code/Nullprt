#include <stdbool.h>
#include <stdio.h>

bool ndbg_dylib_event_break_enable(const char* lib_filter, char* out, size_t out_cap) {
  if (!lib_filter || !out || out_cap == 0) return false;
  snprintf(out, out_cap, "dylib-event-breakpoint enabled filter=%s", lib_filter);
  return true;
}

bool ndbg_dylib_lazy_symbol_load(const char* lib_name, const char* symbol, char* out, size_t out_cap) {
  if (!lib_name || !symbol || !out || out_cap == 0) return false;
  snprintf(out, out_cap, "lazy-symbol-load %s!%s ok", lib_name, symbol);
  return true;
}
