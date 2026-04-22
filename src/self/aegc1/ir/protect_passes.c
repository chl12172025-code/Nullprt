#include "protect_passes.h"

#include <stddef.h>

void a1_protect_options_default(A1ProtectOptions* opt) {
  if (!opt) return;
  opt->enable = true;
  opt->flatten_cfg = true;
  opt->opaque_predicate = true;
  opt->string_obfuscation = true;
  opt->integrity_hash = true;
  opt->antidebug = true;
  opt->optimization_profile = "O2";
}

bool a1_ir_apply_protection_passes(A1IrModule* ir, const A1ProtectOptions* opt) {
  if (!ir || !opt || !opt->enable) return true;
  for (size_t i = 0; i < ir->len; i++) {
    if (!ir->fns[i].cfg_enabled) continue;
    if (opt->flatten_cfg) {
      ir->fns[i].flattened_cfg = true;
      ir->fns[i].basic_blocks += 2;
    }
    if (opt->opaque_predicate) {
      ir->fns[i].opaque_predicate = true;
      ir->fns[i].basic_blocks += 1;
    }
    if (opt->string_obfuscation) ir->fns[i].string_obfuscated = true;
    if (opt->antidebug) ir->fns[i].antidebug_guard = true;
    if (opt->integrity_hash) ir->fns[i].integrity_tag ^= 0xA5A5A5A5A5A5A5A5ull;
  }
  return true;
}
