#include "codegen.h"

#include "lowering.h"
#include "obj_writer.h"

bool a1_codegen_x86_64_object(const A1IrModule* ir, A1NativeTarget target, const char* out_obj_path) {
  A1CallConv cc = (target == A1_TARGET_WIN_COFF) ? A1_CC_WIN64 : A1_CC_SYSV;
  A1ObjFormat fmt = A1_OBJ_COFF;
  if (target == A1_TARGET_LINUX_ELF) fmt = A1_OBJ_ELF;
  if (target == A1_TARGET_MACOS_MACHO) fmt = A1_OBJ_MACHO;

  A1MachineModule mm = a1_x64_lower(ir, cc);
  bool ok = a1_write_object_minimal(&mm, fmt, out_obj_path);
  a1_x64_machine_free(&mm);
  return ok;
}
