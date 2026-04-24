#include "distributed_protocol.h"

#include <stdio.h>
#include <string.h>

bool a1_dist_encode_packet(const A1DistPacket* p, char* out, size_t out_cap) {
  int n;
  if (!p || !out || out_cap == 0) return false;
  n = snprintf(out, out_cap, "kind=%s;digest=%s;size=%lu;seq=%lu",
               p->kind, p->digest, p->payload_size, p->seq);
  return n > 0 && (size_t)n < out_cap;
}

bool a1_dist_decode_packet(const char* in, A1DistPacket* out) {
  if (!in || !out) return false;
  memset(out, 0, sizeof(*out));
  return sscanf(in, "kind=%31[^;];digest=%64[^;];size=%lu;seq=%lu",
                out->kind, out->digest, &out->payload_size, &out->seq) == 4;
}
