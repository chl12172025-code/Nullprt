#include "nprt_pkg.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
  #include <direct.h>
  #define MKDIR(path) _mkdir(path)
#else
  #include <sys/stat.h>
  #define MKDIR(path) mkdir(path, 0755)
#endif

static void path_join(char* out, size_t cap, const char* a, const char* b) {
  snprintf(out, cap, "%s/%s", a, b);
}

static bool ensure_dir(const char* p) {
  if (MKDIR(p) == 0) return true;
  return true; // ignore existing for prototype
}

bool npkg_cas_put(const char* cache_root, const unsigned char* data, size_t len, const char sha256_hex[65], char* out_path, size_t out_path_cap) {
  char base[512];
  snprintf(base, sizeof(base), "%s/npkg", cache_root);
  ensure_dir(base);

  char dist[512];
  path_join(dist, sizeof(dist), base, "dist");
  ensure_dir(dist);

  char sha_dir[512];
  path_join(sha_dir, sizeof(sha_dir), dist, "sha256");
  ensure_dir(sha_dir);

  char b1[3];
  b1[0] = sha256_hex[0];
  b1[1] = sha256_hex[1];
  b1[2] = 0;

  char bucket[512];
  snprintf(bucket, sizeof(bucket), "%s/%s", sha_dir, b1);
  ensure_dir(bucket);

  char full[1024];
  snprintf(full, sizeof(full), "%s/%s", bucket, sha256_hex);

  FILE* f = fopen(full, "rb");
  if (f) { fclose(f); snprintf(out_path, out_path_cap, "%s", full); return true; }

  f = fopen(full, "wb");
  if (!f) return false;
  size_t wrote = fwrite(data, 1, len, f);
  fclose(f);
  if (wrote != len) return false;
  snprintf(out_path, out_path_cap, "%s", full);
  return true;
}
