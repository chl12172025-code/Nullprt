#pragma once

#include "../../../ir/nprt_ir.h"

typedef enum A1CallConv {
  A1_CC_WIN64 = 1,
  A1_CC_SYSV = 2
} A1CallConv;

typedef enum A1InsnKind {
  A1_INSN_PROLOGUE = 1,
  A1_INSN_MOV_IMM,
  A1_INSN_CMP_IMM,
  A1_INSN_JCC,
  A1_INSN_CALL,
  A1_INSN_AVX_BLEND,
  A1_INSN_RET
} A1InsnKind;

typedef struct A1MachineFunction {
  A1StringView name;
  A1CallConv cc;
  uint32_t stack_size;
  uint32_t insn_count;
  A1InsnKind* insns;
} A1MachineFunction;

typedef struct A1MachineModule {
  A1MachineFunction* fns;
  size_t len;
} A1MachineModule;

A1MachineModule a1_x64_lower(const A1IrModule* ir, A1CallConv cc);
void a1_x64_machine_free(A1MachineModule* mm);
