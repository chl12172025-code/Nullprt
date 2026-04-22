#pragma once

#include "lowering.h"

typedef enum A1ObjFormat {
  A1_OBJ_COFF = 1,
  A1_OBJ_ELF = 2,
  A1_OBJ_MACHO = 3
} A1ObjFormat;

bool a1_write_object_minimal(const A1MachineModule* mm, A1ObjFormat fmt, const char* out_obj_path);
