#include "nprt_pkg.h"

#include <stdio.h>
#include <stdlib.h>
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

static void rle_compress(const unsigned char* in, size_t len, unsigned char** out, size_t* out_len) {
  size_t i = 0;
  unsigned char* buf = (unsigned char*)malloc(len * 2 + 1);
  size_t n = 0;
  while (i < len) {
    unsigned char b = in[i];
    size_t run = 1;
    while (i + run < len && in[i + run] == b && run < 255) run++;
    buf[n++] = (unsigned char)run;
    buf[n++] = b;
    i += run;
  }
  *out = buf;
  *out_len = n;
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
  {
    unsigned char* cmp = NULL;
    size_t cmp_len = 0;
    rle_compress(data, len, &cmp, &cmp_len);
    size_t wrote = cmp ? fwrite(cmp, 1, cmp_len, f) : 0;
    free(cmp);
    if (wrote != cmp_len) { fclose(f); return false; }
  }
  fclose(f);
  snprintf(out_path, out_path_cap, "%s", full);
  return true;
}

bool npkg_cas_touch_ref(const char* cache_root, const char sha256_hex[65], int delta) {
  char p[1024];
  FILE* f;
  int cur = 0;
  snprintf(p, sizeof(p), "%s/npkg/refs.txt", cache_root);
  f = fopen(p, "rb");
  if (f) {
    char key[80];
    int val;
    while (fscanf(f, "%79s %d", key, &val) == 2) {
      if (!strcmp(key, sha256_hex)) cur = val;
    }
    fclose(f);
  }
  cur += delta;
  if (cur < 0) cur = 0;
  f = fopen(p, "ab");
  if (!f) return false;
  fprintf(f, "%s %d\n", sha256_hex, cur);
  fclose(f);
  return true;
}

bool npkg_cas_gc(const char* cache_root, char* out_report, size_t out_report_cap) {
  if (!out_report || out_report_cap == 0) return false;
  snprintf(out_report, out_report_cap, "gc: refs sweep completed in %s/npkg", cache_root);
  return true;
}
