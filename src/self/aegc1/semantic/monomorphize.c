#include "monomorphize.h"

A1MonomorphizeResult a1_monomorphize_prep(const A1AstModule* m) {
  A1MonomorphizeResult r;
  r.ok = true;
  r.instantiated_items = 0;
  // Placeholder: scan generic declarations and collect specialization requests.
  for (size_t i = 0; i < m->len; i++) {
    if (m->items[i].kind == A1_ITEM_FN || m->items[i].kind == A1_ITEM_STRUCT || m->items[i].kind == A1_ITEM_ENUM) {
      r.instantiated_items++;
    }
  }
  return r;
}
