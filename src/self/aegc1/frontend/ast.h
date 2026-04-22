#pragma once

#include "../common.h"

typedef enum A1AstItemKind {
  A1_ITEM_FN = 1,
  A1_ITEM_STRUCT,
  A1_ITEM_ENUM,
  A1_ITEM_IMPORT,
  A1_ITEM_MACRO,
} A1AstItemKind;

typedef struct A1AstItem {
  A1AstItemKind kind;
  A1StringView name;
  bool is_generic;
  bool is_extern_c;
  bool has_where;
  bool has_cfg;
  bool has_macro_attrs;
  A1Span span;
} A1AstItem;

typedef struct A1AstModule {
  A1AstItem* items;
  size_t len;
} A1AstModule;
