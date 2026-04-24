#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct NpkgStr {
  const char* ptr;
  size_t len;
} NpkgStr;

typedef struct NpkgBuf {
  unsigned char* data;
  size_t len;
} NpkgBuf;

typedef struct NpkgHttpResponse {
  int status;
  NpkgBuf body;
} NpkgHttpResponse;

bool npkg_http_get(const char* url_utf8, NpkgHttpResponse* out);
void npkg_http_free(NpkgHttpResponse* r);

bool npkg_sha256_hex(const unsigned char* data, size_t len, char out_hex[65]);

// CAS path: <cache_root>/npkg/dist/sha256/<first2>/<fullhex>
bool npkg_cas_put(const char* cache_root, const unsigned char* data, size_t len, const char sha256_hex[65], char* out_path, size_t out_path_cap);
bool npkg_cas_touch_ref(const char* cache_root, const char sha256_hex[65], int delta);
bool npkg_cas_gc(const char* cache_root, char* out_report, size_t out_report_cap);
