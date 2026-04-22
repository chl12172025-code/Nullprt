#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct NpkgDep {
  char* name;
  char* version;
} NpkgDep;

typedef struct NpkgDepGraph {
  NpkgDep* deps;
  size_t len;
  bool has_conflict;
} NpkgDepGraph;

bool npkg_parse_deps_file(const char* path, NpkgDepGraph* out);
bool npkg_write_lockfile(const char* path, const NpkgDepGraph* graph);
void npkg_free_dep_graph(NpkgDepGraph* g);
