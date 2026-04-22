#include "nprt_pkg.h"

#include <string.h>

// Minimal SHA-256 implementation (public domain style) for prototype use.
// NOTE: For production, move into NPRT std::crypto and harden/test thoroughly.

typedef struct {
  uint32_t h[8];
  uint64_t len_bits;
  unsigned char buf[64];
  size_t buf_len;
} Sha256;

static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
static uint32_t bsig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
static uint32_t bsig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
static uint32_t ssig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
static uint32_t ssig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

static const uint32_t K[64] = {
  0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
  0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
  0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
  0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
  0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
  0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
  0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
  0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

static void sha256_init(Sha256* s) {
  s->h[0]=0x6a09e667u; s->h[1]=0xbb67ae85u; s->h[2]=0x3c6ef372u; s->h[3]=0xa54ff53au;
  s->h[4]=0x510e527fu; s->h[5]=0x9b05688cu; s->h[6]=0x1f83d9abu; s->h[7]=0x5be0cd19u;
  s->len_bits = 0;
  s->buf_len = 0;
}

static void sha256_block(Sha256* s, const unsigned char block[64]) {
  uint32_t w[64];
  for (int i = 0; i < 16; i++) {
    w[i] = ((uint32_t)block[i*4] << 24) | ((uint32_t)block[i*4+1] << 16) | ((uint32_t)block[i*4+2] << 8) | (uint32_t)block[i*4+3];
  }
  for (int i = 16; i < 64; i++) w[i] = ssig1(w[i-2]) + w[i-7] + ssig0(w[i-15]) + w[i-16];
  uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7];
  for (int i = 0; i < 64; i++) {
    uint32_t t1 = h + bsig1(e) + ch(e,f,g) + K[i] + w[i];
    uint32_t t2 = bsig0(a) + maj(a,b,c);
    h=g; g=f; f=e; e=d + t1; d=c; c=b; b=a; a=t1 + t2;
  }
  s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d; s->h[4]+=e; s->h[5]+=f; s->h[6]+=g; s->h[7]+=h;
}

static void sha256_update(Sha256* s, const unsigned char* data, size_t len) {
  s->len_bits += (uint64_t)len * 8u;
  while (len) {
    size_t take = 64 - s->buf_len;
    if (take > len) take = len;
    memcpy(s->buf + s->buf_len, data, take);
    s->buf_len += take;
    data += take;
    len -= take;
    if (s->buf_len == 64) {
      sha256_block(s, s->buf);
      s->buf_len = 0;
    }
  }
}

static void sha256_final(Sha256* s, unsigned char out[32]) {
  s->buf[s->buf_len++] = 0x80;
  if (s->buf_len > 56) {
    while (s->buf_len < 64) s->buf[s->buf_len++] = 0;
    sha256_block(s, s->buf);
    s->buf_len = 0;
  }
  while (s->buf_len < 56) s->buf[s->buf_len++] = 0;
  uint64_t L = s->len_bits;
  for (int i = 7; i >= 0; i--) s->buf[s->buf_len++] = (unsigned char)((L >> (i*8)) & 0xff);
  sha256_block(s, s->buf);
  for (int i = 0; i < 8; i++) {
    out[i*4] = (unsigned char)(s->h[i] >> 24);
    out[i*4+1] = (unsigned char)(s->h[i] >> 16);
    out[i*4+2] = (unsigned char)(s->h[i] >> 8);
    out[i*4+3] = (unsigned char)(s->h[i]);
  }
}

static char hex_digit(unsigned v) { return (char)(v < 10 ? ('0' + v) : ('a' + (v - 10))); }

bool npkg_sha256_hex(const unsigned char* data, size_t len, char out_hex[65]) {
  Sha256 s;
  sha256_init(&s);
  sha256_update(&s, data, len);
  unsigned char digest[32];
  sha256_final(&s, digest);
  for (int i = 0; i < 32; i++) {
    out_hex[i*2] = hex_digit((digest[i] >> 4) & 0xf);
    out_hex[i*2+1] = hex_digit(digest[i] & 0xf);
  }
  out_hex[64] = 0;
  return true;
}
