#pragma once

#include <stddef.h>

typedef struct Arena {
  unsigned char* data;
  size_t len;
  size_t cap;
} Arena;

void arena_init(Arena* a);
void arena_free(Arena* a);
void* arena_alloc(Arena* a, size_t size, size_t align);
