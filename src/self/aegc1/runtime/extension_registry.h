#ifndef NPRT_EXTENSION_REGISTRY_H
#define NPRT_EXTENSION_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct NprtExtensionInfo {
  const char* short_ext;
  const char* long_ext;
  const char* kind;
} NprtExtensionInfo;

bool nprt_extension_is_known(const char* ext);
const NprtExtensionInfo* nprt_extension_lookup(const char* ext);
const char* nprt_extension_normalize(const char* ext);
size_t nprt_extension_count(void);

#endif
