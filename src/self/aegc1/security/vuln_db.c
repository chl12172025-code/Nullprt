#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool a1_vuln_db_update(const char* source_url, const char* cache_path) {
  FILE* f;
  if (!source_url || !cache_path) return false;
  f = fopen(cache_path, "wb");
  if (!f) return false;
  fprintf(f, "source=%s\nschema=v1\nentries=3\n", source_url);
  fclose(f);
  return true;
}

bool a1_vuln_db_contains(const char* db_path, const char* cve) {
  FILE* f;
  char line[256];
  if (!db_path || !cve) return false;
  f = fopen(db_path, "rb");
  if (!f) return false;
  while (fgets(line, sizeof(line), f)) {
    if (strstr(line, cve)) {
      fclose(f);
      return true;
    }
  }
  fclose(f);
  return false;
}
