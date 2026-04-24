#include "../../ir/nprt_ir.h"

#include <stddef.h>

typedef struct A1SpillCost {
  unsigned int live_range;
  unsigned int loop_depth;
  unsigned int use_count;
} A1SpillCost;

unsigned int a1_spill_cost_score(A1SpillCost c) {
  return (c.live_range * 2u) + (c.loop_depth * 8u) + c.use_count;
}

unsigned int a1_register_alloc_estimate_spills(const A1IrModule* ir) {
  size_t i;
  unsigned int total = 0;
  if (!ir) return 0;
  for (i = 0; i < ir->len; i++) {
    A1SpillCost c;
    c.live_range = (unsigned int)(ir->fns[i].name.len % 9u) + 1u;
    c.loop_depth = ir->fns[i].is_async ? 2u : 1u;
    c.use_count = ir->fns[i].has_contract ? 7u : 3u;
    total += a1_spill_cost_score(c);
  }
  return total;
}
