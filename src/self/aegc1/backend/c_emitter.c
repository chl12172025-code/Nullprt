#include "c_emitter.h"

#include <stdio.h>

bool a1_emit_c(const A1IrModule* ir, const char* out_c_path, const A1CEmitOptions* opt) {
  FILE* f = fopen(out_c_path, "wb");
  if (!f) return false;
  fprintf(f, "#include <stdint.h>\n");
  fprintf(f, "#include <stdbool.h>\n\n");
  if (opt && opt->debug_comments) {
    fprintf(f, "/* aegc1 C backend (feature coverage skeleton) */\n");
  }
  fprintf(f, "typedef struct A1_Result_i32 { int32_t ok; int32_t value; int32_t err; } A1_Result_i32;\n");
  fprintf(f, "#define A1_RESULT_OK(v) ((A1_Result_i32){1,(v),0})\n");
  fprintf(f, "#define A1_RESULT_ERR(e) ((A1_Result_i32){0,0,(e)})\n\n");
  fprintf(f, "typedef struct A1_EnumTagPayload { int32_t tag; union { int32_t i32v; uint64_t u64v; } payload; } A1_EnumTagPayload;\n");
  fprintf(f, "static int32_t a1_match_enum_i32(A1_EnumTagPayload e) { switch (e.tag) { case 0: return e.payload.i32v; case 1: return (int32_t)e.payload.u64v; default: return -1; } }\n\n");
  fprintf(f, "static const unsigned char A1_OBF_KEY = 0x5Au;\n");
  fprintf(f, "static const char* a1_decode_stub(const char* in) { (void)in; return \"decoded\"; }\n");
  fprintf(f, "static int a1_antidebug_probe(void) { return 0; }\n");
  fprintf(f, "static uint64_t a1_integrity_hash(uint64_t v) { return (v ^ 0xA5A5A5A5A5A5A5A5ull) * 1099511628211ull; }\n\n");

  for (size_t i = 0; i < ir->len; i++) {
    const A1IrFunction* fn = &ir->fns[i];
    if (!fn->cfg_enabled) continue;

    if (fn->source_kind == A1_ITEM_STRUCT) {
      fprintf(f, "typedef struct %.*s { int32_t _stub; } %.*s;\n",
        (int)fn->name.len, fn->name.ptr, (int)fn->name.len, fn->name.ptr);
      continue;
    }
    if (fn->source_kind == A1_ITEM_ENUM) {
      fprintf(f, "typedef struct %.*s { int32_t tag; int32_t payload_i32; } %.*s;\n",
        (int)fn->name.len, fn->name.ptr, (int)fn->name.len, fn->name.ptr);
      continue;
    }
    if (fn->is_result_like) {
      fprintf(f, "A1_Result_i32 %.*s(void) { return A1_RESULT_OK(0); }\n", (int)fn->name.len, fn->name.ptr);
      continue;
    }

    if (fn->name.len == 4 && fn->name.ptr[0] == 'm' && fn->name.ptr[1] == 'a' && fn->name.ptr[2] == 'i' && fn->name.ptr[3] == 'n') {
      fprintf(f, "int32_t main(void) {\n");
      if (fn->antidebug_guard) fprintf(f, "  if (a1_antidebug_probe()) { return -1001; }\n");
      if (fn->string_obfuscated) fprintf(f, "  (void)a1_decode_stub(\"encoded\");\n");
      fprintf(f, "  (void)a1_integrity_hash(%lluull);\n", (unsigned long long)fn->integrity_tag);
      fprintf(f, "  A1_EnumTagPayload e; e.tag = 0; e.payload.i32v = 0;\n");
      fprintf(f, "  return a1_match_enum_i32(e);\n");
      fprintf(f, "}\n");
    } else {
      if (fn->is_generic_instance && opt && opt->debug_comments) fprintf(f, "/* generic-instance */\n");
      fprintf(f, "int32_t %.*s(void) { return 0; }\n", (int)fn->name.len, fn->name.ptr);
    }
  }
  fclose(f);
  return true;
}
