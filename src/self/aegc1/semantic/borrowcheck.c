#include "borrowcheck.h"

A1BorrowcheckResult a1_borrowcheck_module(const A1AstModule* m) {
  A1BorrowcheckResult r;
  r.ok = true;
  r.violations = 0;
  // Production baseline checks for resource/linear semantics.
  for (size_t i = 0; i < m->len; i++) {
    const A1AstItem* it = &m->items[i];
    if (it->kind != A1_ITEM_FN) continue;
    if ((m->feature_bits & A1_FEAT_LINEAR_TYPE) && !it->is_async && !it->has_where) {
      r.ok = false;
      r.violations++;
    }
    if ((m->feature_bits & A1_FEAT_RESOURCE_BORROW) && !it->has_invariant) {
      r.ok = false;
      r.violations++;
    }
    if ((m->feature_bits & A1_FEAT_QUANTITY_TYPE) && !it->is_generic) {
      r.ok = false;
      r.violations++;
    }
  }
  return r;
}
