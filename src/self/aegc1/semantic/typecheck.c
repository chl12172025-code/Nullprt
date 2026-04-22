#include "typecheck.h"

#include <stdio.h>

A1TypecheckResult a1_typecheck_module(const A1AstModule* m) {
  A1TypecheckResult r;
  r.ok = true;
  r.errors = 0;
  // Placeholder for full type system, generic constraints and Result propagation rules.
  // Current check: require at least one function item.
  bool has_fn = false;
  for (size_t i = 0; i < m->len; i++) {
    if (m->items[i].kind == A1_ITEM_FN) {
      has_fn = true;
      break;
    }
  }
  if (!has_fn) {
    fprintf(stderr, "aegc1 typecheck: no function definitions found\n");
    r.ok = false;
    r.errors++;
  }
  return r;
}
