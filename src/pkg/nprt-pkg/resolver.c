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
      if (!strcmp(out->deps[i].name, name) && strcmp(out->deps[i].version, ver)) {
        out->has_conflict = true;
      }
    }

    NpkgDep* n = (NpkgDep*)realloc(out->deps, sizeof(NpkgDep) * (out->len + 1));
    if (!n) break;
    out->deps = n;
    out->deps[out->len].name = dup_s(name);
    out->deps[out->len].version = dup_s(ver);
    out->len++;
  }
  fclose(f);
  return true;
}

bool npkg_write_lockfile(const char* path, const NpkgDepGraph* graph) {
  FILE* f = fopen(path, "wb");
  if (!f) return false;
  fprintf(f, "nprtcfg 0.1.0;\n\n");
  fprintf(f, "lock.format_version = \"0\";\n");
  fprintf(f, "packages {\n");
  for (size_t i = 0; i < graph->len; i++) {
    fprintf(f, "  %s = { version: \"%s\" };\n", graph->deps[i].name, graph->deps[i].version);
  }
  fprintf(f, "}\n");
  fclose(f);
  return true;
}

void npkg_free_dep_graph(NpkgDepGraph* g) {
  if (!g) return;
  for (size_t i = 0; i < g->len; i++) {
    free(g->deps[i].name);
    free(g->deps[i].version);
  }
  free(g->deps);
  g->deps = NULL;
  g->len = 0;
}
