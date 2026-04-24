#pragma once

#include "../common.h"

typedef enum A1AstItemKind {
  A1_ITEM_FN = 1,
  A1_ITEM_STRUCT,
  A1_ITEM_ENUM,
  A1_ITEM_IMPORT,
  A1_ITEM_MACRO,
} A1AstItemKind;

typedef enum A1AstFeatureBits {
  A1_FEAT_DEPENDENT_TYPE       = 1ull << 0,
  A1_FEAT_REFINEMENT_TYPE      = 1ull << 1,
  A1_FEAT_LINEAR_TYPE          = 1ull << 2,
  A1_FEAT_EFFECT_TYPE          = 1ull << 3,
  A1_FEAT_ALGEBRAIC_EFFECT     = 1ull << 4,
  A1_FEAT_HIGHER_KINDED        = 1ull << 5,
  A1_FEAT_EXISTENTIAL          = 1ull << 6,
  A1_FEAT_SESSION_TYPE         = 1ull << 7,
  A1_FEAT_PROOF_TYPE           = 1ull << 8,
  A1_FEAT_CONTRACT_TYPE        = 1ull << 9,
  A1_FEAT_INVARIANT_TYPE       = 1ull << 10,
  A1_FEAT_RESOURCE_BORROW      = 1ull << 11,
  A1_FEAT_QUANTITY_TYPE        = 1ull << 12,
  A1_FEAT_GRADED_TYPE          = 1ull << 13,
  A1_FEAT_ROW_POLYMORPHISM     = 1ull << 14,
  A1_FEAT_RECURSIVE_TYPE       = 1ull << 15,
  A1_FEAT_ASSOC_TYPE_DEP       = 1ull << 16,
  A1_FEAT_SIZE_TYPE            = 1ull << 17,
  A1_FEAT_VALUE_DEPENDENT      = 1ull << 18,
  A1_FEAT_PATTERN_EXHAUSTIVE   = 1ull << 19,
  A1_FEAT_PATTERN_RANGE        = 1ull << 20,
  A1_FEAT_PATTERN_NESTED       = 1ull << 21,
  A1_FEAT_ASYNC_CANCEL         = 1ull << 22,
  A1_FEAT_GENERATOR_RESUME     = 1ull << 23,
  A1_FEAT_COROUTINE_SCHED      = 1ull << 24,
  A1_FEAT_DECL_MACRO_RECURSION = 1ull << 25,
  A1_FEAT_PROC_MACRO_STABLE    = 1ull << 26,
  A1_FEAT_HYGIENE_ISOLATION    = 1ull << 27,
  A1_FEAT_COMPTIME_REFLECTION  = 1ull << 28,
  A1_FEAT_AST_VALIDATION       = 1ull << 29,
} A1AstFeatureBits;

typedef struct A1TypeExpr {
  A1StringView raw;
  bool has_where;
  bool has_effect;
  bool has_session;
  bool has_exists;
  bool has_linear;
  bool has_dependent;
  bool has_refinement;
} A1TypeExpr;

typedef struct A1AstItem {
  A1AstItemKind kind;
  A1StringView name;
  bool is_generic;
  bool is_extern_c;
  bool is_async;
  bool is_generator;
  bool is_coroutine;
  bool has_contract;
  bool has_invariant;
  bool has_macro_recursion_limit;
  bool has_proc_macro_bridge;
  bool has_hygiene_attr;
  bool has_where;
  bool has_cfg;
  bool has_macro_attrs;
  uint64_t feature_bits;
  A1TypeExpr signature_type;
  A1Span span;
} A1AstItem;

typedef struct A1AstModule {
  A1AstItem* items;
  size_t len;
  uint64_t feature_bits;
  uint32_t nested_pattern_depth_limit;
} A1AstModule;
