#include "build_graph.h"

#include <stdio.h>
#include <string.h>

bool a1_build_graph_load(const char* cache_path, A1BuildGraph* out) {
  FILE* f;
  if (!out) return false;
  memset(out, 0, sizeof(*out));
  f = fopen(cache_path, "rb");
  if (!f) return true;
  while (out->len < 1024) {
    A1BuildNode* n = &out->nodes[out->len];
    if (fscanf(f, "%511s %64s %lu", n->path, n->hash, &n->version) != 3) break;
    out->len++;
  }
  fclose(f);
  return true;
}

bool a1_build_graph_record(A1BuildGraph* graph, const char* input_path, const char* hash_hex) {
  size_t i;
  if (!graph || !input_path || !hash_hex) return false;
  for (i = 0; i < graph->len; i++) {
    if (strcmp(graph->nodes[i].path, input_path) == 0) {
      strncpy(graph->nodes[i].hash, hash_hex, sizeof(graph->nodes[i].hash) - 1);
      graph->nodes[i].hash[sizeof(graph->nodes[i].hash) - 1] = '\0';
      graph->nodes[i].version++;
      return true;
    }
  }
  if (graph->len >= 1024) return false;
  strncpy(graph->nodes[graph->len].path, input_path, sizeof(graph->nodes[graph->len].path) - 1);
  graph->nodes[graph->len].path[sizeof(graph->nodes[graph->len].path) - 1] = '\0';
  strncpy(graph->nodes[graph->len].hash, hash_hex, sizeof(graph->nodes[graph->len].hash) - 1);
  graph->nodes[graph->len].hash[sizeof(graph->nodes[graph->len].hash) - 1] = '\0';
  graph->nodes[graph->len].version = 1;
  graph->len++;
  return true;
}

bool a1_build_graph_save(const char* cache_path, const A1BuildGraph* graph) {
  FILE* f;
  size_t i;
  if (!cache_path || !graph) return false;
  f = fopen(cache_path, "wb");
  if (!f) return false;
  for (i = 0; i < graph->len; i++) {
    const A1BuildNode* n = &graph->nodes[i];
    fprintf(f, "%s %s %lu\n", n->path, n->hash, n->version);
  }
  fclose(f);
  return true;
}
