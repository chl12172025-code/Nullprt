#include "arena.h"

#include <stdlib.h>
#include <string.h>

static size_t align_up(size_t x, size_t a) {
  size_t mask = a - 1;
  return (x + mask) & ~mask;
}

void arena_init(Arena* a) {
  a->data = NULL;
  a->len = 0;
  a->cap = 0;
}

void arena_free(Arena* a) {
  free(a->data);
  a->data = NULL;
  a->len = 0;
  a->cap = 0;
}

void* arena_alloc(Arena* a, size_t size, size_t align) {
  if (align < 1) align = 1;
  if ((align & (align - 1)) != 0) align = sizeof(void*);

  size_t start = align_up(a->len, align);
  size_t needed = start + size;
  if (needed > a->cap) {
    size_t new_cap = a->cap ? a->cap : 4096;
    while (new_cap < needed) new_cap *= 2;
    unsigned char* new_data = (unsigned char*)realloc(a->data, new_cap);
    if (!new_data) return NULL;
    a->data = new_data;
    a->cap = new_cap;
  }
  void* out = a->data + start;
  memset(out, 0, size);
  a->len = needed;
  return out;
}
