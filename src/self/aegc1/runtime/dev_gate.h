#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifndef NPRT_DEV_RESEARCH
#define NPRT_DEV_RESEARCH 0
#endif

typedef struct NprtResearchToken {
  unsigned long long value;
  bool valid;
} NprtResearchToken;

// WARNING: This API is for authorized security research and compatibility
// testing only. Do not use for attacking third-party systems or bypassing
// license validation.
NprtResearchToken nprt_authorize_research(const char* purpose);

// WARNING: This API is for authorized security research and compatibility
// testing only. Do not use for attacking third-party systems or bypassing
// license validation.
bool nprt_research_token_is_valid(const NprtResearchToken* token);

void nprt_research_log_event(const char* api_name, const char* details);
void nprt_research_warn_once(void);
const char* nprt_research_log_path(void);
