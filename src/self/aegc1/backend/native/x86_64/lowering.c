#include "lowering.h"

#include <stdlib.h>
#include <string.h>

A1MachineModule a1_x64_lower(const A1IrModule* ir, A1CallConv cc) {
  A1MachineModule mm = {0};
  for (size_t i = 0; i < ir->len; i++) {
    if (ir->fns[i].source_kind != A1_ITEM_FN) continue;
    A1MachineFunction* n = (A1MachineFunction*)realloc(mm.fns, sizeof(A1MachineFunction) * (mm.len + 1));
    if (!n) break;
    mm.fns = n;
    mm.fns[mm.len].name = ir->fns[i].name;
    mm.fns[mm.len].cc = cc;
    mm.fns[mm.len].stack_size = 32; // placeholder prologue requirement
    mm.fns[mm.len].insn_count = 6;
    mm.fns[mm.len].insns = (A1InsnKind*)calloc(mm.fns[mm.len].insn_count, sizeof(A1InsnKind));
    if (mm.fns[mm.len].insns) {
      A1InsnKind seq[6] = {
        A1_INSN_PROLOGUE,
        A1_INSN_MOV_IMM,
        A1_INSN_CMP_IMM,
        A1_INSN_JCC,
        A1_INSN_CALL,
        A1_INSN_RET
      };
      memcpy(mm.fns[mm.len].insns, seq, sizeof(seq));
    }
    mm.len++;
  }
  return mm;
}

void a1_x64_machine_free(A1MachineModule* mm) {
  for (size_t i = 0; i < mm->len; i++) {
    free(mm->fns[i].insns);
    mm->fns[i].insns = NULL;
  }
  free(mm->fns);
  mm->fns = NULL;
  mm->len = 0;
}
