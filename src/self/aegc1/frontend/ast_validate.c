#include "ast_validate.h"

#include <stddef.h>

bool a1_ast_validate(const A1AstModule* ast) {
  if (!ast) return false;
  if (ast->len == 0) return false;
  if (ast->nested_pattern_depth_limit == 0 || ast->nested_pattern_depth_limit > 256) return false;
  for (size_t i = 0; i < ast->len; i++) {
    const A1AstItem* it = &ast->items[i];
    if (it->name.len == 0) return false;
    if (it->is_coroutine && !it->is_async) return false;
    if (it->has_proc_macro_bridge && it->kind != A1_ITEM_MACRO) return false;
  }
  return true;
}
