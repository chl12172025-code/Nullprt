#include "../frontend/parser.h"
#include "../frontend/ast_validate.h"
#include "../semantic/typecheck.h"
#include "../semantic/borrowcheck.h"
#include "../semantic/monomorphize.h"
#include "../ir/nprt_ir.h"
#include "../ir/dump.h"
#include "../ir/protect_passes.h"
#include "../backend/c_emitter.h"
#include "../backend/native/x86_64/codegen.h"
#include "../runtime/dev_gate.h"
#include "../runtime/extension_registry.h"
#include "../runtime/concurrency_runtime.h"
#include "build_graph.h"
#include "distributed_protocol.h"
#include "pch_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool a1_arm64_sve_lower_report(const A1IrModule* ir, const char* out_path);
bool a1_riscv_rvv_lower_report(const A1IrModule* ir, const char* out_path);
bool a1_wasm_multivalue_lower_report(const A1IrModule* ir, const char* out_path);
bool a1_sparc_window_lower_report(const A1IrModule* ir, const char* out_path);
bool a1_powerpc_link_stack_lower_report(const A1IrModule* ir, const char* out_path);
bool a1_mips_delay_slot_lower_report(const A1IrModule* ir, const char* out_path);
unsigned int a1_register_alloc_estimate_spills(const A1IrModule* ir);
bool a1_link_emit_incremental_patch(const A1IrModule* ir, const char* out_path);
bool a1_link_emit_lazy_stub_table(const A1IrModule* ir, const char* out_path);
bool a1_link_emit_prelink_inline_report(const A1IrModule* ir, const char* out_path);
bool a1_link_emit_postlink_rewrite_report(const A1IrModule* ir, const char* out_path);
const char* a1_diag_translate(const char* locale, const char* code);
double a1_ml_hint_score(const char* diagnostic_code, const char* context);
bool a1_diag_emit_autofix_preview(const char* file, const char* before, const char* after, const char* out_path);
bool a1_vuln_db_update(const char* source_url, const char* cache_path);
bool a1_license_is_compatible(const char* root_license, const char* dep_license);

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

static const char* file_ext(const char* path) {
  const char* slash = strrchr(path, '/');
  const char* bslash = strrchr(path, '\\');
  const char* base = path;
  const char* dot;
  if (slash && slash + 1 > base) base = slash + 1;
  if (bslash && bslash + 1 > base) base = bslash + 1;
  dot = strrchr(base, '.');
  return dot ? dot : "";
}

static void hash_source_cheap(const char* src, size_t len, char out_hex[65]) {
  unsigned long long h = 1469598103934665603ull;
  size_t i;
  for (i = 0; i < len; i++) {
    h ^= (unsigned char)src[i];
    h *= 1099511628211ull;
  }
  snprintf(out_hex, 65, "%016llx%016llx%016llx%016llx",
           h, h ^ 0x9e3779b97f4a7c15ull, h ^ 0xa0761d6478bd642full, h ^ 0xe7037ed1a0b428dbull);
}

int main(int argc, char** argv) {
  const char* in = NULL;
  const char* out_c = NULL;
  const char* out_obj = NULL;
  const char* out_ir = NULL;
  const char* link_out = NULL;
  bool c_debug_comments = false;
  bool protect_off = false;
  const char* opt_level = "O2";
  bool emit_diag_json = false;
  const char* target = "win";
  const char* research_purpose = NULL;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-i") && i + 1 < argc) { in = argv[++i]; continue; }
    if (!strcmp(argv[i], "--emit-c") && i + 1 < argc) { out_c = argv[++i]; continue; }
    if (!strcmp(argv[i], "--emit-native") && i + 1 < argc) { out_obj = argv[++i]; continue; }
    if (!strcmp(argv[i], "--dump-ir") && i + 1 < argc) { out_ir = argv[++i]; continue; }
    if (!strcmp(argv[i], "--link-out") && i + 1 < argc) { link_out = argv[++i]; continue; }
    if (!strcmp(argv[i], "--emit-c-debug-comments")) { c_debug_comments = true; continue; }
    if (!strcmp(argv[i], "--no-protect")) { protect_off = true; continue; }
    if (!strcmp(argv[i], "--opt") && i + 1 < argc) { opt_level = argv[++i]; continue; }
    if (!strcmp(argv[i], "--diag-json")) { emit_diag_json = true; continue; }
    if (!strcmp(argv[i], "--target") && i + 1 < argc) { target = argv[++i]; continue; }
    if (!strcmp(argv[i], "--research-purpose") && i + 1 < argc) { research_purpose = argv[++i]; continue; }
  }

  if (!in || (!out_c && !out_obj)) {
    usage();
    return 2;
  }
  {
    const char* ext = file_ext(in);
    const NprtExtensionInfo* info = nprt_extension_lookup(ext);
    if (!info) {
      fprintf(stderr, "aegc1: unsupported input extension: %s\n", ext[0] ? ext : "<none>");
      fprintf(stderr, "aegc1: expected Nullprt source alias (.nprt/.nullprt/.nprti/.nprtm)\n");
      return 2;
    }
    fprintf(stderr, "aegc1: input-kind=%s normalized-ext=%s\n", info->kind, info->short_ext);
  }

  if (research_purpose) {
    NprtResearchToken tok = nprt_authorize_research(research_purpose);
    if (!nprt_research_token_is_valid(&tok)) {
      fprintf(stderr, "aegc1: research gate rejected purpose=%s\n", research_purpose);
      return 1;
    }
    nprt_research_log_event("aegc1.main", research_purpose);
  }
  if (emit_diag_json) {
    fprintf(stderr, "{\"tool\":\"aegc1\",\"target\":\"%s\",\"opt\":\"%s\",\"phase\":\"pre-parse\"}\n", target, opt_level);
  }

  size_t len = 0;
  char* src = read_file(in, &len);
  A1BuildGraph build_graph;
  char src_hash[65];
  if (!src) {
    fprintf(stderr, "aegc1: failed to read input: %s\n", in);
    return 1;
  }
  hash_source_cheap(src, len, src_hash);
  (void)a1_build_graph_load("build/aegc1.buildgraph", &build_graph);
  (void)a1_build_graph_record(&build_graph, in, src_hash);
  (void)a1_build_graph_save("build/aegc1.buildgraph", &build_graph);
  {
    A1PchProfile producer = {1u, 64u, 100u};
    A1PchProfile consumer = {1u, 64u, 100u};
    if (!a1_pch_is_compatible(&producer, &consumer)) {
      fprintf(stderr, "aegc1: pch compatibility mismatch\n");
      free(src);
      return 1;
    }
  }
  if (emit_diag_json) {
    A1DistPacket pkt;
    char wire[256];
    memset(&pkt, 0, sizeof(pkt));
    strncpy(pkt.kind, "capability", sizeof(pkt.kind) - 1);
    strncpy(pkt.digest, src_hash, sizeof(pkt.digest) - 1);
    pkt.payload_size = (unsigned long)len;
    pkt.seq = 1;
    if (a1_dist_encode_packet(&pkt, wire, sizeof(wire))) {
      fprintf(stderr, "{\"tool\":\"aegc1\",\"phase\":\"dist-protocol\",\"packet\":\"%s\"}\n", wire);
    }
  }

  A1Parser p;
  a1_parser_init(&p, src, len);
  A1AstModule ast = a1_parse_module(&p);
  if (!a1_ast_validate(&ast)) {
    fprintf(stderr, "aegc1: AST validation failed\n");
    free(ast.items);
    free(src);
    return 1;
  }
  if (p.had_error) {
    free(ast.items);
    free(src);
    return 1;
  }

  if (ast.feature_bits & A1_FEAT_COROUTINE_SCHED) {
    NprtScheduler sched = {"aegc1-default", true, true};
    (void)nprt_register_scheduler(&sched);
  }
  if (ast.feature_bits & A1_FEAT_ASYNC_CANCEL) {
    (void)nprt_propagate_cancellation("compile_async_scope");
  }
  if (ast.feature_bits & A1_FEAT_GENERATOR_RESUME) {
    (void)nprt_generator_resume("compile_generator_scope");
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
  if (emit_diag_json) {
    fprintf(stderr,
            "{\"tool\":\"aegc1\",\"phase\":\"semantic\",\"feature_bits\":%llu,\"items\":%zu,\"type_ok\":%s,\"borrow_ok\":%s,\"mono_ok\":%s}\n",
            (unsigned long long)ast.feature_bits, ast.len, tr.ok ? "true" : "false", br.ok ? "true" : "false", mr.ok ? "true" : "false");
  }
  A1ProtectOptions popt;
  a1_protect_options_default(&popt);
  if (protect_off) popt.enable = false;
  (void)a1_ir_apply_protection_passes(&ir, &popt);
  if (!a1_ir_validate(&ir)) {
    fprintf(stderr, "aegc1: IR validation failed\n");
    a1_ir_free(&ir);
    free(ast.items);
    free(src);
    return 1;
  }
  (void)a1_arm64_sve_lower_report(&ir, "build/backend_arm64_sve.txt");
  (void)a1_riscv_rvv_lower_report(&ir, "build/backend_riscv_rvv.txt");
  (void)a1_wasm_multivalue_lower_report(&ir, "build/backend_wasm_multivalue.txt");
  (void)a1_sparc_window_lower_report(&ir, "build/backend_sparc_window.txt");
  (void)a1_powerpc_link_stack_lower_report(&ir, "build/backend_powerpc_link_stack.txt");
  (void)a1_mips_delay_slot_lower_report(&ir, "build/backend_mips_delay_slot.txt");
  (void)a1_link_emit_incremental_patch(&ir, "build/link_incremental.patch");
  (void)a1_link_emit_lazy_stub_table(&ir, "build/link_lazy_stubs.txt");
  (void)a1_link_emit_prelink_inline_report(&ir, "build/link_prelink_report.txt");
  (void)a1_link_emit_postlink_rewrite_report(&ir, "build/link_postlink_report.txt");
  (void)a1_diag_emit_autofix_preview(in, "let  x=1", "let x = 1", "build/diag_autofix_preview.txt");
  (void)a1_vuln_db_update("https://example.invalid/nprt-vuln-db", "build/vuln_db.txt");

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
  if (emit_diag_json) {
    fprintf(stderr,
            "{\"tool\":\"aegc1\",\"phase\":\"compiler-full\",\"spill_cost\":%u,\"i18n_e1001\":\"%s\",\"ml_hint_score\":%.2f,\"license_ok\":%s}\n",
            a1_register_alloc_estimate_spills(&ir),
            a1_diag_translate("zh-CN", "E1001"),
            a1_ml_hint_score("E1001", "missing generic where bound"),
            a1_license_is_compatible("Apache-2.0", "MIT") ? "true" : "false");
  }

  a1_ir_free(&ir);
  free(ast.items);
  free(src);
  return ok ? 0 : 1;
}
