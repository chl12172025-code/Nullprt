#include "pch_compat.h"

bool a1_pch_is_compatible(const A1PchProfile* producer, const A1PchProfile* consumer) {
  if (!producer || !consumer) return false;
  if (producer->abi_version != consumer->abi_version) return false;
  if (producer->target_bits != consumer->target_bits) return false;
  return producer->language_epoch <= consumer->language_epoch;
}
