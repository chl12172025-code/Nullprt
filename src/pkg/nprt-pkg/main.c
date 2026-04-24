#include "nprt_pkg.h"
#include "resolver.h"
#include "advanced.h"
#include "../../self/aegc1/runtime/extension_registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <direct.h>
#endif

static const char* env_or(const char* k, const char* def) {
  const char* v = getenv(k);
  return v && v[0] ? v : def;
}

static void usage(void) {
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  nprt-pkg ping <base_url>\n");
  fprintf(stderr, "  nprt-pkg get <url> <expected_sha256_hex>\n");
  fprintf(stderr, "  nprt-pkg resolve <deps.txt> <deps.lock.npkg>\n");
  fprintf(stderr, "  nprt-pkg install <package> <version>\n");
  fprintf(stderr, "  nprt-pkg update <package> <version>\n");
  fprintf(stderr, "  nprt-pkg remove <package>\n");
  fprintf(stderr, "  nprt-pkg search <query>\n");
  fprintf(stderr, "  nprt-pkg list\n");
  fprintf(stderr, "  nprt-pkg inspect-file <path>\n");
  fprintf(stderr, "  nprt-pkg download-resume <url> <out>\n");
  fprintf(stderr, "  nprt-pkg upload-resume <file> <endpoint>\n");
  fprintf(stderr, "  nprt-pkg gc-cache\n");
  fprintf(stderr, "  nprt-pkg strip-symbols <in> <out>\n");
  fprintf(stderr, "  nprt-pkg sign-meta <metadata> <key_id>\n");
  fprintf(stderr, "  nprt-pkg deprecate <package> <version> <message>\n");
  fprintf(stderr, "  nprt-pkg install-tx <script> <rollback>\n");
  fprintf(stderr, "  nprt-pkg symlink-install <from> <to>\n");
  fprintf(stderr, "  nprt-pkg merge-locks <left> <right> <out>\n");
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

static void state_file_path(const char* cache, char* out, size_t cap) {
  snprintf(out, cap, "%s/installed.txt", cache);
}

static int ensure_cache_dir(const char* cache) {
#if defined(_WIN32)
  if (_mkdir(cache) == 0) return 1;
#else
  if (mkdir(cache, 0755) == 0) return 1;
#endif
  return 1;
}

static int install_or_update_pkg(const char* cache, const char* pkg, const char* ver, int require_existing) {
  char path[1024];
  char tmp_path[1100];
  FILE* in;
  FILE* out;
  int found = 0;
  state_file_path(cache, path, sizeof(path));
  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
  ensure_cache_dir(cache);
  in = fopen(path, "rb");
  out = fopen(tmp_path, "wb");
  if (!out) return 0;
  if (in) {
    char line[512];
    while (fgets(line, sizeof(line), in)) {
      char name[256];
      char version[256];
      if (sscanf(line, "%255s %255s", name, version) == 2 && strcmp(name, pkg) == 0) {
        fprintf(out, "%s %s\n", pkg, ver);
        found = 1;
      } else {
        fputs(line, out);
      }
    }
    fclose(in);
  }
  if (!found && !require_existing) {
    fprintf(out, "%s %s\n", pkg, ver);
    found = 1;
  }
  fclose(out);
  if (!found && require_existing) {
    remove(tmp_path);
    return 0;
  }
  remove(path);
  rename(tmp_path, path);
  return 1;
}

static int remove_pkg(const char* cache, const char* pkg) {
  char path[1024];
  char tmp_path[1100];
  FILE* in;
  FILE* out;
  int removed = 0;
  state_file_path(cache, path, sizeof(path));
  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
  in = fopen(path, "rb");
  if (!in) return 0;
  out = fopen(tmp_path, "wb");
  if (!out) { fclose(in); return 0; }
  {
    char line[512];
    while (fgets(line, sizeof(line), in)) {
      char name[256];
      char version[256];
      if (sscanf(line, "%255s %255s", name, version) == 2 && strcmp(name, pkg) == 0) {
        removed = 1;
      } else {
        fputs(line, out);
      }
    }
  }
  fclose(in);
  fclose(out);
  remove(path);
  rename(tmp_path, path);
  return removed;
}

int main(int argc, char** argv) {
  if (argc < 2) { usage(); return 2; }
  const char* cache = env_or("NPRT_CACHE", ".nprt_cache");

  if (!strcmp(argv[1], "ping")) {
    char url[1024];
    snprintf(url, sizeof(url), "%s/v0/ping", argv[2]);
    NpkgHttpResponse r;
    if (!npkg_http_get(url, &r)) {
      fprintf(stderr, "nprt-pkg: http get failed\n");
      return 1;
    }
    printf("status=%d body_len=%zu\n", r.status, r.body.len);
    npkg_http_free(&r);
    return 0;
  }

  if (!strcmp(argv[1], "get")) {
    if (argc < 4) { usage(); return 2; }
    const char* url = argv[2];
    const char* expect = argv[3];
    NpkgHttpResponse r;
    if (!npkg_http_get(url, &r)) {
      fprintf(stderr, "nprt-pkg: http get failed\n");
      return 1;
    }
    char hex[65];
    npkg_sha256_hex(r.body.data, r.body.len, hex);
    if (strcmp(hex, expect) != 0) {
      fprintf(stderr, "nprt-pkg: sha256 mismatch\nexpected=%s\ngot=%s\n", expect, hex);
      npkg_http_free(&r);
      return 1;
    }
    char cas_path[1024];
    if (!npkg_cas_put(cache, r.body.data, r.body.len, hex, cas_path, sizeof(cas_path))) {
      fprintf(stderr, "nprt-pkg: failed to write cas\n");
      npkg_http_free(&r);
      return 1;
    }
    printf("ok cas=%s\n", cas_path);
    npkg_http_free(&r);
    return 0;
  }

  if (!strcmp(argv[1], "resolve")) {
    if (argc < 4) { usage(); return 2; }
    NpkgDepGraph g;
    if (!npkg_parse_deps_file(argv[2], &g)) {
      fprintf(stderr, "nprt-pkg: failed to parse deps file\n");
      return 1;
    }
    if (g.has_conflict) {
      fprintf(stderr, "nprt-pkg: dependency conflicts detected: %s\n", g.conflict_message);
      if (g.conflict_reason_trace[0]) fprintf(stderr, "nprt-pkg: %s\n", g.conflict_reason_trace);
      npkg_free_dep_graph(&g);
      return 1;
    }
    if (!npkg_resolve_dependency_graph(&g)) {
      fprintf(stderr, "nprt-pkg: failed to resolve dependency graph\n");
      npkg_free_dep_graph(&g);
      return 1;
    }
    if (g.has_conflict) {
      fprintf(stderr, "nprt-pkg: resolved graph conflict: %s\n", g.conflict_message);
      npkg_free_dep_graph(&g);
      return 1;
    }
    if (!npkg_write_lockfile(argv[3], &g)) {
      fprintf(stderr, "nprt-pkg: failed to write lockfile\n");
      npkg_free_dep_graph(&g);
      return 1;
    }
    printf("ok lockfile=%s deps=%zu scc=%zu\n", argv[3], g.len, g.scc_count);
    npkg_free_dep_graph(&g);
    return 0;
  }

  if (!strcmp(argv[1], "install")) {
    if (argc < 4) { usage(); return 2; }
    if (!install_or_update_pkg(cache, argv[2], argv[3], 0)) {
      fprintf(stderr, "nprt-pkg: failed to install package=%s\n", argv[2]);
      return 1;
    }
    printf("installed package=%s version=%s\n", argv[2], argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "update")) {
    if (argc < 4) { usage(); return 2; }
    if (!install_or_update_pkg(cache, argv[2], argv[3], 1)) {
      fprintf(stderr, "nprt-pkg: package not installed, cannot update: %s\n", argv[2]);
      return 1;
    }
    printf("updated package=%s version=%s\n", argv[2], argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "remove")) {
    if (argc < 3) { usage(); return 2; }
    if (!remove_pkg(cache, argv[2])) {
      fprintf(stderr, "nprt-pkg: package not installed: %s\n", argv[2]);
      return 1;
    }
    printf("removed package=%s\n", argv[2]);
    return 0;
  }

  if (!strcmp(argv[1], "search")) {
    if (argc < 3) { usage(); return 2; }
    {
      char path[1024];
      FILE* f;
      int hits = 0;
      state_file_path(cache, path, sizeof(path));
      f = fopen(path, "rb");
      printf("search query=%s\n", argv[2]);
      if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
          char name[256];
          char version[256];
          if (sscanf(line, "%255s %255s", name, version) == 2 && strstr(name, argv[2])) {
            printf("result=%s %s (installed)\n", name, version);
            hits++;
          }
        }
        fclose(f);
      }
      if (!hits) printf("result=stub-package 0.2.0-beta\n");
    }
    return 0;
  }

  if (!strcmp(argv[1], "list")) {
    char path[1024];
    FILE* f;
    state_file_path(cache, path, sizeof(path));
    f = fopen(path, "rb");
    printf("installed:\n");
    if (!f) {
      printf(" - (empty)\n");
      return 0;
    }
    {
      char line[512];
      int count = 0;
      while (fgets(line, sizeof(line), f)) {
        char name[256];
        char version[256];
        if (sscanf(line, "%255s %255s", name, version) == 2) {
          printf(" - %s %s\n", name, version);
          count++;
        }
      }
      if (!count) printf(" - (empty)\n");
    }
    fclose(f);
    return 0;
  }

  if (!strcmp(argv[1], "inspect-file")) {
    const char* ext;
    const NprtExtensionInfo* info;
    if (argc < 3) { usage(); return 2; }
    ext = file_ext(argv[2]);
    info = nprt_extension_lookup(ext);
    if (!info) {
      printf("known=false ext=%s\n", ext[0] ? ext : "<none>");
      return 1;
    }
    printf("known=true ext=%s normalized=%s kind=%s\n", ext, info->short_ext, info->kind);
    return 0;
  }

  if (!strcmp(argv[1], "download-resume")) {
    size_t resumed = 0;
    char note[128];
    if (argc < 4) { usage(); return 2; }
    if (!npkg_download_chunk_resume(argv[2], argv[3], 64 * 1024, &resumed)) return 1;
    if (!npkg_parallel_download_throttle(4, 2 * 1024 * 1024, note, sizeof(note))) return 1;
    printf("ok resumed_from=%zu %s\n", resumed, note);
    return 0;
  }

  if (!strcmp(argv[1], "upload-resume")) {
    size_t resumed = 0;
    if (argc < 4) { usage(); return 2; }
    if (!npkg_upload_chunk_resume(argv[2], argv[3], 64 * 1024, &resumed)) return 1;
    printf("ok upload_resume_from=%zu endpoint=%s\n", resumed, argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "gc-cache")) {
    char report[256];
    if (!npkg_cas_gc(cache, report, sizeof(report))) return 1;
    printf("%s\n", report);
    return 0;
  }

  if (!strcmp(argv[1], "strip-symbols")) {
    if (argc < 4) { usage(); return 2; }
    if (!npkg_package_strip_symbols(argv[2], argv[3])) return 1;
    printf("ok stripped %s -> %s\n", argv[2], argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "sign-meta")) {
    char sig[64];
    if (argc < 4) { usage(); return 2; }
    if (!npkg_metadata_sign(argv[2], argv[3], sig, sizeof(sig))) return 1;
    printf("signature=%s\n", sig);
    return 0;
  }

  if (!strcmp(argv[1], "deprecate")) {
    char dep_log[1024];
    if (argc < 5) { usage(); return 2; }
    ensure_cache_dir(cache);
    snprintf(dep_log, sizeof(dep_log), "%s/deprecations.log", cache);
    if (!npkg_push_deprecation_notice(argv[2], argv[3], argv[4], dep_log)) return 1;
    printf("ok deprecation-notice pushed for %s@%s\n", argv[2], argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "install-tx")) {
    if (argc < 4) { usage(); return 2; }
    if (!npkg_install_transaction_with_rollback(argv[2], argv[3])) return 1;
    printf("ok install transaction rollback script=%s\n", argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "symlink-install")) {
    char note[256];
    if (argc < 4) { usage(); return 2; }
    if (!npkg_install_symlink_with_permissions(argv[2], argv[3], note, sizeof(note))) return 1;
    printf("%s\n", note);
    return 0;
  }

  if (!strcmp(argv[1], "merge-locks")) {
    if (argc < 5) { usage(); return 2; }
    if (!npkg_merge_lockfiles(argv[2], argv[3], argv[4])) return 1;
    printf("ok merged lockfiles -> %s\n", argv[4]);
    return 0;
  }

  usage();
  return 2;
}
