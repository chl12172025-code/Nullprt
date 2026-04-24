#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct A1BuildNode {
  char path[512];
  char hash[65];
  unsigned long version;
} A1BuildNode;

typedef struct A1BuildGraph {
  A1BuildNode nodes[1024];
  size_t len;
} A1BuildGraph;

bool a1_build_graph_load(const char* cache_path, A1BuildGraph* out);
bool a1_build_graph_record(A1BuildGraph* graph, const char* input_path, const char* hash_hex);
bool a1_build_graph_save(const char* cache_path, const A1BuildGraph* graph);
