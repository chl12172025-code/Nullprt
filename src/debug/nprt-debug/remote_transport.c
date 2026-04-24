#include <stdio.h>
#include <stdbool.h>
#include <string.h>

bool ndbg_remote_save_config(const char* path, const char* endpoint, unsigned int latency_ms, bool compress) {
  FILE* f;
  if (!path || !endpoint) return false;
  f = fopen(path, "wb");
  if (!f) return false;
  fprintf(f, "endpoint=%s\nlatency_ms=%u\ncompress=%s\n", endpoint, latency_ms, compress ? "true" : "false");
  fclose(f);
  return true;
}

bool ndbg_remote_load_config(const char* path, char* endpoint, size_t endpoint_cap, unsigned int* latency_ms, bool* compress) {
  FILE* f;
  char cmp[16] = {0};
  if (!path || !endpoint || !latency_ms || !compress) return false;
  f = fopen(path, "rb");
  if (!f) return false;
  if (fscanf(f, "endpoint=%255s\nlatency_ms=%u\ncompress=%15s\n", endpoint, latency_ms, cmp) != 3) {
    fclose(f);
    return false;
  }
  *compress = strcmp(cmp, "true") == 0;
  fclose(f);
  return true;
}

size_t ndbg_remote_compress_payload(const unsigned char* in, size_t len, unsigned char* out, size_t out_cap) {
  size_t i = 0, o = 0;
  while (i < len && o + 2 < out_cap) {
    unsigned char b = in[i];
    size_t run = 1;
    while (i + run < len && in[i + run] == b && run < 255) run++;
    out[o++] = (unsigned char)run;
    out[o++] = b;
    i += run;
  }
  return o;
}
