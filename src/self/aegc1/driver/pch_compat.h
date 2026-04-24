#pragma once

#include <stdbool.h>

typedef struct A1PchProfile {
  unsigned int abi_version;
  unsigned int target_bits;
  unsigned int language_epoch;
} A1PchProfile;

bool a1_pch_is_compatible(const A1PchProfile* producer, const A1PchProfile* consumer);
