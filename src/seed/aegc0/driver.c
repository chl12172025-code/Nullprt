#include "emit_c.h"
#include "parser.h"
#include "sema.h"

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

static int run_cmd(const char* cmd) {
  fprintf(stderr, "aegc0: run: %s\n", cmd);
  return system(cmd);
}

static void usage(void) {
  fprintf(stderr, "usage: aegc0 -i <input.nprt> -o <out.exe> [--emit-c <out.c>]\n");
}

int main(int argc, char** argv) {
  const char* in_path = NULL;
  const char* out_path = NULL;
  const char* emit_c_path = "aegc0_out.c";

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-i") && i + 1 < argc) { in_path = argv[++i]; continue; }
    if (!strcmp(argv[i], "-o") && i + 1 < argc) { out_path = argv[++i]; continue; }
    if (!strcmp(argv[i], "--emit-c") && i + 1 < argc) { emit_c_path = argv[++i]; continue; }
    usage();
    return 2;
  }
  if (!in_path || !out_path) {
    usage();
    return 2;
  }

  size_t src_len = 0;
  char* src = read_file(in_path, &src_len);
  if (!src) {
    fprintf(stderr, "aegc0: failed to read input: %s\n", in_path);
    return 1;
  }

  Arena arena;
  arena_init(&arena);

  Parser p;
  parser_init(&p, &arena, in_path, src, src_len);
  Program* prog = parse_program(&p);
  if (p.had_error) {
    free(src);
    arena_free(&arena);
    return 1;
  }

  Sema s;
  sema_init(&s, &arena);
  if (!sema_check_program(&s, prog)) {
    free(src);
    arena_free(&arena);
    return 1;
  }

  CEmitOptions opt = {emit_c_path};
  if (!emit_c_program(prog, &opt)) {
    fprintf(stderr, "aegc0: failed to write C output: %s\n", emit_c_path);
    free(src);
    arena_free(&arena);
    return 1;
  }

  // compile
  // Prefer clang if present, else fallback.
  // Windows: use cl if available (Developer Prompt). Otherwise clang.
  char cmd[4096];
#if defined(_WIN32)
  // try cl
  snprintf(cmd, sizeof(cmd), "cl /nologo /W3 /O2 \"%s\" /Fe:\"%s\" >NUL", emit_c_path, out_path);
  int rc = run_cmd(cmd);
  if (rc != 0) {
    snprintf(cmd, sizeof(cmd), "clang -O2 \"%s\" -o \"%s\"", emit_c_path, out_path);
    rc = run_cmd(cmd);
  }
  if (rc != 0) {
    fprintf(stderr, "aegc0: no usable C compiler found. Install one of: MSVC Build Tools (cl) or LLVM (clang).\n");
    fprintf(stderr, "aegc0: hint: on Windows, run from a Developer Command Prompt so cl.exe is on PATH.\n");
  }
#else
  snprintf(cmd, sizeof(cmd), "cc -O2 \"%s\" -o \"%s\"", emit_c_path, out_path);
  int rc = run_cmd(cmd);
  if (rc != 0) {
    fprintf(stderr, "aegc0: no usable C compiler found. Install clang/gcc (cc) and ensure it's on PATH.\n");
  }
#endif

  free(src);
  arena_free(&arena);
  return rc == 0 ? 0 : 1;
}
