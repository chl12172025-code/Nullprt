#include "abi_test.h"

ABI_EXPORT int32_t abi_add_i32(int32_t a, int32_t b) { return a + b; }
ABI_EXPORT uint64_t abi_add_u64(uint64_t a, uint64_t b) { return a + b; }

ABI_EXPORT ABI_PairI32 abi_swap_pair(ABI_PairI32 p) {
  ABI_PairI32 out;
  out.a = p.b;
  out.b = p.a;
  return out;
}

ABI_EXPORT uint32_t abi_sizeof_mixed(void) { return (uint32_t)sizeof(ABI_Mixed); }

ABI_EXPORT uint32_t abi_sum_bytes(const uint8_t* data, size_t len) {
  uint32_t s = 0;
  for (size_t i = 0; i < len; i++) s += (uint32_t)data[i];
  return s;
}

ABI_EXPORT int32_t abi_call_cb_i32(ABI_I32UnaryFn cb, int32_t x) {
  if (!cb) return -1;
  return cb(x);
}
