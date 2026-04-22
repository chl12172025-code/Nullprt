#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
  #define ABI_EXPORT __declspec(dllexport)
#else
  #define ABI_EXPORT __attribute__((visibility("default")))
#endif

typedef struct ABI_PairI32 {
  int32_t a;
  int32_t b;
} ABI_PairI32;

typedef struct ABI_Mixed {
  uint8_t  tag;
  uint32_t value;
  uint16_t small;
} ABI_Mixed;

typedef int32_t (*ABI_I32UnaryFn)(int32_t x);

ABI_EXPORT int32_t abi_add_i32(int32_t a, int32_t b);
ABI_EXPORT uint64_t abi_add_u64(uint64_t a, uint64_t b);

ABI_EXPORT ABI_PairI32 abi_swap_pair(ABI_PairI32 p);
ABI_EXPORT uint32_t abi_sizeof_mixed(void);

ABI_EXPORT uint32_t abi_sum_bytes(const uint8_t* data, size_t len);

ABI_EXPORT int32_t abi_call_cb_i32(ABI_I32UnaryFn cb, int32_t x);
