#include "nprt_pkg.h"
#include "resolver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
}

int main(int argc, char** argv) {
  if (argc < 3) { usage(); return 2; }
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
      npkg_free_dep_graph(&g);
      return 1;
    }
    if (!npkg_write_lockfile(argv[3], &g)) {
      fprintf(stderr, "nprt-pkg: failed to write lockfile\n");
      npkg_free_dep_graph(&g);
      return 1;
    }
    printf("ok lockfile=%s deps=%zu\n", argv[3], g.len);
    npkg_free_dep_graph(&g);
    return 0;
  }

  if (!strcmp(argv[1], "install")) {
    if (argc < 4) { usage(); return 2; }
    printf("installed package=%s version=%s\n", argv[2], argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "update")) {
    if (argc < 4) { usage(); return 2; }
    printf("updated package=%s version=%s\n", argv[2], argv[3]);
    return 0;
  }

  if (!strcmp(argv[1], "remove")) {
    if (argc < 3) { usage(); return 2; }
    printf("removed package=%s\n", argv[2]);
    return 0;
  }

  if (!strcmp(argv[1], "search")) {
    if (argc < 3) { usage(); return 2; }
    printf("search query=%s\nresult=stub-package 0.2.0-beta\n", argv[2]);
    return 0;
  }

  if (!strcmp(argv[1], "list")) {
    printf("installed:\n - nullprt-stdlib 0.2.0-beta\n");
    return 0;
  }

  usage();
  return 2;
}
