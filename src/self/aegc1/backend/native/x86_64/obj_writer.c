#include "obj_writer.h"

#include <stdint.h>
#include <stdio.h>

static bool write_coff(FILE* f, const A1MachineModule* mm) {
  // Minimal marker + machine metadata, not full COFF yet.
  fwrite("COFF", 1, 4, f);
  uint32_t n = (uint32_t)mm->len;
  fwrite(&n, 1, 4, f);
  return true;
}

static bool write_elf(FILE* f, const A1MachineModule* mm) {
  // Minimal ELF magic + metadata stub.
  unsigned char ehdr[16] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0};
  fwrite(ehdr, 1, 16, f);
  uint32_t n = (uint32_t)mm->len;
  fwrite(&n, 1, 4, f);
  return true;
}

static bool write_macho(FILE* f, const A1MachineModule* mm) {
  // Minimal Mach-O magic (64-bit) + metadata stub.
  uint32_t magic = 0xfeedfacf;
  fwrite(&magic, 1, 4, f);
  uint32_t n = (uint32_t)mm->len;
  fwrite(&n, 1, 4, f);
  return true;
}

bool a1_write_object_minimal(const A1MachineModule* mm, A1ObjFormat fmt, const char* out_obj_path) {
  FILE* f = fopen(out_obj_path, "wb");
  if (!f) return false;
  bool ok = false;
  switch (fmt) {
    case A1_OBJ_COFF: ok = write_coff(f, mm); break;
    case A1_OBJ_ELF: ok = write_elf(f, mm); break;
    case A1_OBJ_MACHO: ok = write_macho(f, mm); break;
    default: ok = false; break;
  }
  fclose(f);
  return ok;
}
