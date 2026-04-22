#include "nprt_pkg.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static wchar_t* utf8_to_wide_alloc(const char* s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  if (n <= 0) return NULL;
  wchar_t* w = (wchar_t*)malloc(sizeof(wchar_t) * (size_t)n);
  if (!w) return NULL;
  if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n)) { free(w); return NULL; }
  return w;
}

static bool append_buf(NpkgBuf* b, const unsigned char* data, size_t n) {
  unsigned char* p = (unsigned char*)realloc(b->data, b->len + n);
  if (!p) return false;
  memcpy(p + b->len, data, n);
  b->data = p;
  b->len += n;
  return true;
}

bool npkg_http_get(const char* url_utf8, NpkgHttpResponse* out) {
  memset(out, 0, sizeof(*out));

  wchar_t* wurl = utf8_to_wide_alloc(url_utf8);
  if (!wurl) return false;

  URL_COMPONENTS uc;
  memset(&uc, 0, sizeof(uc));
  uc.dwStructSize = sizeof(uc);
  uc.dwSchemeLength = (DWORD)-1;
  uc.dwHostNameLength = (DWORD)-1;
  uc.dwUrlPathLength = (DWORD)-1;
  uc.dwExtraInfoLength = (DWORD)-1;

  if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) {
    free(wurl);
    return false;
  }

  HINTERNET hSession = WinHttpOpen(L"nprt-pkg/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession) { free(wurl); return false; }

  wchar_t host[256];
  wcsncpy(host, uc.lpszHostName, uc.dwHostNameLength);
  host[uc.dwHostNameLength] = 0;

  HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
  if (!hConnect) { WinHttpCloseHandle(hSession); free(wurl); return false; }

  wchar_t path[2048];
  size_t path_len = 0;
  wcsncpy(path, uc.lpszUrlPath, uc.dwUrlPathLength);
  path_len = uc.dwUrlPathLength;
  if (uc.dwExtraInfoLength) {
    wcsncpy(path + path_len, uc.lpszExtraInfo, uc.dwExtraInfoLength);
    path_len += uc.dwExtraInfoLength;
  }
  path[path_len] = 0;

  DWORD flags = 0;
  if (uc.nScheme == INTERNET_SCHEME_HTTPS) flags |= WINHTTP_FLAG_SECURE;

  HINTERNET hReq = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); free(wurl); return false; }

  BOOL ok = WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  if (!ok) goto fail;
  ok = WinHttpReceiveResponse(hReq, NULL);
  if (!ok) goto fail;

  DWORD status = 0;
  DWORD status_sz = sizeof(status);
  if (!WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_sz, WINHTTP_NO_HEADER_INDEX)) goto fail;
  out->status = (int)status;

  for (;;) {
    DWORD avail = 0;
    if (!WinHttpQueryDataAvailable(hReq, &avail)) goto fail;
    if (avail == 0) break;
    unsigned char* tmp = (unsigned char*)malloc(avail);
    if (!tmp) goto fail;
    DWORD read = 0;
    if (!WinHttpReadData(hReq, tmp, avail, &read)) { free(tmp); goto fail; }
    if (!append_buf(&out->body, tmp, read)) { free(tmp); goto fail; }
    free(tmp);
  }

  WinHttpCloseHandle(hReq);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);
  free(wurl);
  return true;

fail:
  WinHttpCloseHandle(hReq);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);
  free(wurl);
  npkg_http_free(out);
  return false;
}

void npkg_http_free(NpkgHttpResponse* r) {
  free(r->body.data);
  r->body.data = NULL;
  r->body.len = 0;
}

#else
// Non-windows stub for now.
bool npkg_http_get(const char* url_utf8, NpkgHttpResponse* out) {
  (void)url_utf8;
  (void)out;
  return false;
}
void npkg_http_free(NpkgHttpResponse* r) { (void)r; }
#endif
