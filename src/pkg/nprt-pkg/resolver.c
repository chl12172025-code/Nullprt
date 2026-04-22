#include "resolver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* dup_s(const char* s) {
  size_t n = strlen(s);
  char* p = (char*)malloc(n + 1);
  if (!p) return NULL;
  memcpy(p, s, n + 1);
  return p;
}

typedef struct SemVer {
  int major;
  int minor;
  int patch;
} SemVer;

static bool parse_semver(const char* s, SemVer* out) {
  return sscanf(s, "%d.%d.%d", &out->major, &out->minor, &out->patch) == 3;
}

static int cmp_semver(const SemVer* a, const SemVer* b) {
  if (a->major != b->major) return (a->major < b->major) ? -1 : 1;
  if (a->minor != b->minor) return (a->minor < b->minor) ? -1 : 1;
  if (a->patch != b->patch) return (a->patch < b->patch) ? -1 : 1;
  return 0;
}

static bool semver_satisfies(const char* req, const char* actual) {
  SemVer r, a;
  if (!req || !actual) return false;
  if (req[0] == '^') {
    if (!parse_semver(req + 1, &r) || !parse_semver(actual, &a)) return false;
    if (a.major != r.major) return false;
    return cmp_semver(&a, &r) >= 0;
  }
  if (req[0] == '~') {
    if (!parse_semver(req + 1, &r) || !parse_semver(actual, &a)) return false;
    if (a.major != r.major || a.minor != r.minor) return false;
    return cmp_semver(&a, &r) >= 0;
  }
  if (!parse_semver(req, &r) || !parse_semver(actual, &a)) return false;
  return cmp_semver(&a, &r) == 0;
}

static void trim(char* s) {
  char* p = s;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  if (p != s) memmove(s, p, strlen(p) + 1);
  size_t n = strlen(s);
  while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) s[--n] = 0;
}

bool npkg_parse_deps_file(const char* path, NpkgDepGraph* out) {
  memset(out, 0, sizeof(*out));
  FILE* f = fopen(path, "rb");
  if (!f) return false;
  char line[1024];
  while (fgets(line, sizeof(line), f)) {
    trim(line);
    if (!line[0] || line[0] == '#') continue;
    char* eq = strchr(line, '=');
    if (!eq) continue;
    *eq = 0;
    char* name = line;
    char* ver = eq + 1;
    trim(name);
    trim(ver);
    if (!name[0] || !ver[0]) continue;

    for (size_t i = 0; i < out->len; i++) {
      if (!strcmp(out->deps[i].name, name) &&
          !semver_satisfies(out->deps[i].version_req, ver) &&
          !semver_satisfies(ver, out->deps[i].version_req)) {
        out->has_conflict = true;
        snprintf(out->conflict_message, sizeof(out->conflict_message),
                 "conflict: %s requires both %s and %s",
                 name, out->deps[i].version_req, ver);
      }
    }

    NpkgDep* n = (NpkgDep*)realloc(out->deps, sizeof(NpkgDep) * (out->len + 1));
    if (!n) break;
    out->deps = n;
    out->deps[out->len].name = dup_s(name);
    out->deps[out->len].version_req = dup_s(ver);
    out->deps[out->len].resolved_version = dup_s(ver[0] == '^' || ver[0] == '~' ? ver + 1 : ver);
    out->deps[out->len].source = dup_s("registry:nullprt");
    out->deps[out->len].download_url = dup_s("https://registry.nullprt.local/v0/download");
    out->deps[out->len].integrity = dup_s("sha256:pending");
    out->len++;
  }
  fclose(f);
  return true;
}

bool npkg_write_lockfile(const char* path, const NpkgDepGraph* graph) {
  FILE* f = fopen(path, "wb");
  if (!f) return false;
  fprintf(f, "nprtcfg 0.1.0;\n\n");
  fprintf(f, "lock.format_version = \"1\";\n");
  fprintf(f, "packages {\n");
  for (size_t i = 0; i < graph->len; i++) {
    fprintf(
      f,
      "  %s = { version_req: \"%s\", version: \"%s\", source: \"%s\", download_url: \"%s\", integrity: \"%s\" };\n",
      graph->deps[i].name,
      graph->deps[i].version_req,
      graph->deps[i].resolved_version,
      graph->deps[i].source,
      graph->deps[i].download_url,
      graph->deps[i].integrity);
  }
  fprintf(f, "}\n");
  fclose(f);
  return true;
}

void npkg_free_dep_graph(NpkgDepGraph* g) {
  if (!g) return;
  for (size_t i = 0; i < g->len; i++) {
    free(g->deps[i].name);
    free(g->deps[i].version_req);
    free(g->deps[i].resolved_version);
    free(g->deps[i].source);
    free(g->deps[i].download_url);
    free(g->deps[i].integrity);
  }
  free(g->deps);
  g->deps = NULL;
  g->len = 0;
}
