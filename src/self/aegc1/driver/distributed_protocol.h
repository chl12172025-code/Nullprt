#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct A1DistPacket {
  char kind[32];
  char digest[65];
  unsigned long payload_size;
  unsigned long seq;
} A1DistPacket;

bool a1_dist_encode_packet(const A1DistPacket* p, char* out, size_t out_cap);
bool a1_dist_decode_packet(const char* in, A1DistPacket* out);
