#pragma once

#include "arena.h"
#include "ast.h"

typedef struct Sema {
  Arena* arena;
  bool had_error;
} Sema;

void sema_init(Sema* s, Arena* arena);
bool sema_check_program(Sema* s, Program* prog);
