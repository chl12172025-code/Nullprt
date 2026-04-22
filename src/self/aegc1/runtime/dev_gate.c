#include "dev_gate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_warned = 0;

static unsigned long long fnv1a64(const char* s) {
  unsigned long long h = 1469598103934665603ull;
  if (!s) return h;
  while (*s) {
    h ^= (unsigned char)(*s++);
    h *= 1099511628211ull;
  }
  return h;
}

const char* nprt_research_log_path(void) {
  const char* p = getenv("NPRT_RESEARCH_AUDIT_LOG");
  if (p && p[0]) return p;
  return "build/nprt_research_audit.log";
}

void nprt_research_warn_once(void) {
  if (g_warned) return;
  g_warned = 1;
  fprintf(stderr, "WARNING: This API is for authorized security research and compatibility testing only. Do not use for attacking third-party systems or bypassing license validation.\n");
}

void nprt_research_log_event(const char* api_name, const char* details) {
  FILE* f = fopen(nprt_research_log_path(), "ab");
  time_t now = time(NULL);
  if (!f) return;
  fprintf(f, "ts=%lld api=%s details=%s\n",
          (long long)now,
          api_name ? api_name : "unknown",
          details ? details : "");
  fclose(f);
}

NprtResearchToken nprt_authorize_research(const char* purpose) {
  NprtResearchToken t = {0};
#if NPRT_DEV_RESEARCH
  const char* env = getenv("NPRT_RESEARCH_TOKEN");
  if (!env || !env[0]) {
    nprt_research_log_event("nprt_authorize_research", "missing NPRT_RESEARCH_TOKEN");
    return t;
  }
  unsigned long long base = fnv1a64(env);
  unsigned long long p = fnv1a64(purpose ? purpose : "unspecified");
  t.value = base ^ (p << 1);
  t.valid = true;
  nprt_research_warn_once();
  nprt_research_log_event("nprt_authorize_research", purpose ? purpose : "unspecified");
#else
  (void)purpose;
  nprt_research_log_event("nprt_authorize_research", "blocked: NPRT_DEV_RESEARCH=OFF");
#endif
  return t;
}

bool nprt_research_token_is_valid(const NprtResearchToken* token) {
#if NPRT_DEV_RESEARCH
  bool ok = token && token->valid && token->value != 0ull;
  nprt_research_log_event("nprt_research_token_is_valid", ok ? "ok" : "invalid");
  return ok;
#else
  (void)token;
  nprt_research_log_event("nprt_research_token_is_valid", "blocked: NPRT_DEV_RESEARCH=OFF");
  return false;
#endif
}
