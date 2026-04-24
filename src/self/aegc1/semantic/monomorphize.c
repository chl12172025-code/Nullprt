#include "monomorphize.h"

A1MonomorphizeResult a1_monomorphize_prep(const A1AstModule* m) {
  A1MonomorphizeResult r;
  r.ok = true;
  r.instantiated_items = 0;
  // Production baseline: count and validate generic-driven instantiations.
  for (size_t i = 0; i < m->len; i++) {
    const A1AstItem* it = &m->items[i];
    if (it->kind == A1_ITEM_FN || it->kind == A1_ITEM_STRUCT || it->kind == A1_ITEM_ENUM) {
      if (it->is_generic) r.instantiated_items += 2;
      else r.instantiated_items++;
      if ((m->feature_bits & A1_FEAT_HIGHER_KINDED) && !it->is_generic) r.ok = false;
      if ((m->feature_bits & A1_FEAT_EXISTENTIAL) && !it->has_where) r.ok = false;
      if ((m->feature_bits & A1_FEAT_ROW_POLYMORPHISM) && !it->is_generic) r.ok = false;
    }
  }
  return r;
}
