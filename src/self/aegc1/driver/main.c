#include "../frontend/parser.h"
#include "../semantic/typecheck.h"
#include "../semantic/borrowcheck.h"
#include "../semantic/monomorphize.h"
#include "../ir/nprt_ir.h"
#include "../ir/dump.h"
#include "../backend/c_emitter.h"
#include "../backend/native/x86_64/codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file(const char* path, size_t* out_len) {
  FILE* f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return NULL; }
  char* buf = (char*)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return NULL; }
  size_t got = fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[got] = 0;
  *out_len = got;
  return buf;
}

static void usage(void) {
  fprintf(stderr, "usage: aegc1 -i <input.nprt> --emit-c <out.c>\n");
  fprintf(stderr, "       aegc1 -i <input.nprt> --emit-native <out.obj> [--target win|linux|macos]\n");
  fprintf(stderr, "       aegc1 -i <input.nprt> --emit-native <out.obj> --link-out <out.bin> [--target win|linux|macos]\n");
}

int main(int argc, char** argv) {
  const char* in = NULL;
  const char* out_c = NULL;
  const char* out_obj = NULL;
  const char* out_ir = NULL;
  const char* link_out = NULL;
  bool c_debug_comments = false;
  const char* target = "win";

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-i") && i + 1 < argc) { in = argv[++i]; continue; }
    if (!strcmp(argv[i], "--emit-c") && i + 1 < argc) { out_c = argv[++i]; continue; }
    if (!strcmp(argv[i], "--emit-native") && i + 1 < argc) { out_obj = argv[++i]; continue; }
    if (!strcmp(argv[i], "--dump-ir") && i + 1 < argc) { out_ir = argv[++i]; continue; }
    if (!strcmp(argv[i], "--link-out") && i + 1 < argc) { link_out = argv[++i]; continue; }
    if (!strcmp(argv[i], "--emit-c-debug-comments")) { c_debug_comments = true; continue; }
    if (!strcmp(argv[i], "--target") && i + 1 < argc) { target = argv[++i]; continue; }
  }

  if (!in || (!out_c && !out_obj)) {
    usage();
    return 2;
  }

  size_t len = 0;
  char* src = read_file(in, &len);
  if (!src) {
    fprintf(stderr, "aegc1: failed to read input: %s\n", in);
    return 1;
  }

  A1Parser p;
  a1_parser_init(&p, src, len);
  A1AstModule ast = a1_parse_module(&p);
  if (p.had_error) {
    free(ast.items);
    free(src);
    return 1;
  }

  A1TypecheckResult tr = a1_typecheck_module(&ast);
  if (!tr.ok) {
    free(ast.items);
    free(src);
    return 1;
  }

  A1BorrowcheckResult br = a1_borrowcheck_module(&ast);
  if (!br.ok) {
    free(ast.items);
    free(src);
    return 1;
  }

  A1MonomorphizeResult mr = a1_monomorphize_prep(&ast);
  if (!mr.ok) {
    free(ast.items);
    free(src);
    return 1;
  }

  A1IrModule ir = a1_ir_lower(&ast);
  if (!a1_ir_validate(&ir)) {
    fprintf(stderr, "aegc1: IR validation failed\n");
    a1_ir_free(&ir);
    free(ast.items);
    free(src);
    return 1;
  }

  bool ok = true;
  if (out_ir) ok = a1_ir_dump_text(&ir, out_ir);
  if (out_c) {
    A1CEmitOptions copts;
    copts.debug_comments = c_debug_comments;
    ok = a1_emit_c(&ir, out_c, &copts);
  }

  if (ok && out_obj) {
    A1NativeTarget t = A1_TARGET_WIN_COFF;
    if (!strcmp(target, "linux")) t = A1_TARGET_LINUX_ELF;
    else if (!strcmp(target, "macos")) t = A1_TARGET_MACOS_MACHO;
    ok = a1_codegen_x86_64_object(&ir, t, out_obj);
    if (ok && link_out) {
      char cmd[2048];
      if (!strcmp(target, "win")) {
        snprintf(cmd, sizeof(cmd), "link /nologo \"%s\" /OUT:\"%s\" >NUL", out_obj, link_out);
      } else if (!strcmp(target, "linux")) {
        snprintf(cmd, sizeof(cmd), "cc \"%s\" -o \"%s\"", out_obj, link_out);
      } else {
        snprintf(cmd, sizeof(cmd), "cc \"%s\" -o \"%s\"", out_obj, link_out);
      }
      fprintf(stderr, "aegc1 link cmd: %s\n", cmd);
      int rc = system(cmd);
      if (rc != 0) {
        fprintf(stderr, "aegc1: linker step failed for target=%s\n", target);
        ok = false;
      }
    }
  }

  a1_ir_free(&ir);
  free(ast.items);
  free(src);
  return ok ? 0 : 1;
}
