#pragma once

#include "../../../ir/nprt_ir.h"

typedef enum A1NativeTarget {
  A1_TARGET_WIN_COFF = 1,
  A1_TARGET_LINUX_ELF = 2,
  A1_TARGET_MACOS_MACHO = 3
} A1NativeTarget;

bool a1_codegen_x86_64_object(const A1IrModule* ir, A1NativeTarget target, const char* out_obj_path);
