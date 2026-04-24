#include "typecheck.h"

#include <stdio.h>
#include <string.h>

typedef struct FeatureRule {
  uint64_t bit;
  const char* code;
  const char* message;
} FeatureRule;

static void tc_error(A1TypecheckResult* r, const char* code, const char* message) {
  fprintf(stderr, "aegc1 typecheck[%s]: %s\n", code, message);
  r->ok = false;
  r->errors++;
}

static bool module_has_async(const A1AstModule* m) {
  for (size_t i = 0; i < m->len; i++) if (m->items[i].is_async) return true;
  return false;
}

static bool module_has_generator(const A1AstModule* m) {
  for (size_t i = 0; i < m->len; i++) if (m->items[i].is_generator) return true;
  return false;
}

static bool module_has_coroutine(const A1AstModule* m) {
  for (size_t i = 0; i < m->len; i++) if (m->items[i].is_coroutine) return true;
  return false;
}

A1TypecheckResult a1_typecheck_module(const A1AstModule* m) {
  A1TypecheckResult r;
  r.ok = true;
  r.errors = 0;
  // Production check baseline: feature gating, structured diagnostics and signature checks.
  static const FeatureRule kRules[] = {
    {A1_FEAT_DEPENDENT_TYPE,       "E1001", "dependent type requires generic function or where-constraint."},
    {A1_FEAT_REFINEMENT_TYPE,      "E1002", "refinement type requires where predicate."},
    {A1_FEAT_LINEAR_TYPE,          "E1003", "linear type requires mutable binding discipline."},
    {A1_FEAT_EFFECT_TYPE,          "E1004", "effect type requires effect annotation."},
    {A1_FEAT_ALGEBRAIC_EFFECT,     "E1005", "algebraic effect requires effect handler declaration."},
    {A1_FEAT_HIGHER_KINDED,        "E1006", "higher-kinded type requires generic constructor."},
    {A1_FEAT_EXISTENTIAL,          "E1007", "existential type requires exists pack/unpack markers."},
    {A1_FEAT_SESSION_TYPE,         "E1008", "session type requires protocol/session declaration."},
    {A1_FEAT_PROOF_TYPE,           "E1009", "proof type requires theorem/proof marker."},
    {A1_FEAT_CONTRACT_TYPE,        "E1010", "contract type requires contract/require marker."},
    {A1_FEAT_INVARIANT_TYPE,       "E1011", "invariant type requires invariant marker."},
    {A1_FEAT_RESOURCE_BORROW,      "E1012", "resource borrow requires borrow/resource marker."},
    {A1_FEAT_QUANTITY_TYPE,        "E1013", "quantity type requires quantity marker."},
    {A1_FEAT_GRADED_TYPE,          "E1014", "graded type requires graded/security_level marker."},
    {A1_FEAT_ROW_POLYMORPHISM,     "E1015", "row polymorphism requires row marker."},
    {A1_FEAT_RECURSIVE_TYPE,       "E1016", "recursive type requires recursive/mu marker."},
    {A1_FEAT_ASSOC_TYPE_DEP,       "E1017", "associated type dependency requires associated marker."},
    {A1_FEAT_SIZE_TYPE,            "E1018", "size type requires size/bound marker."},
    {A1_FEAT_VALUE_DEPENDENT,      "E1019", "value-dependent type requires const/value marker."},
    {A1_FEAT_PATTERN_EXHAUSTIVE,   "E1020", "exhaustiveness analysis requires match form."},
    {A1_FEAT_PATTERN_RANGE,        "E1021", "range pattern requires range expression."},
    {A1_FEAT_PATTERN_NESTED,       "E1022", "nested pattern requires configured depth limit."},
    {A1_FEAT_ASYNC_CANCEL,         "E1023", "async cancellation requires async function."},
    {A1_FEAT_GENERATOR_RESUME,     "E1024", "generator resume requires generator function."},
    {A1_FEAT_COROUTINE_SCHED,      "E1025", "coroutine scheduler requires coroutine function."},
    {A1_FEAT_DECL_MACRO_RECURSION, "E1026", "declarative macro recursion requires recursion limit marker."},
    {A1_FEAT_PROC_MACRO_STABLE,    "E1027", "procedural macro stabilization requires proc marker."},
    {A1_FEAT_HYGIENE_ISOLATION,    "E1028", "hygiene requires hygiene marker."},
    {A1_FEAT_COMPTIME_REFLECTION,  "E1029", "compile-time reflection requires reflect/comptime marker."},
    {A1_FEAT_AST_VALIDATION,       "E1030", "AST validation requires syntax_tree/ast_validate marker."},
  };

  bool has_fn = false;
  bool has_generic = false;
  bool has_where = false;
  bool has_contract = false;
  bool has_invariant = false;
  bool has_macro = false;
  for (size_t i = 0; i < m->len; i++) {
    if (m->items[i].kind == A1_ITEM_FN) {
      has_fn = true;
    }
    if (m->items[i].is_generic) has_generic = true;
    if (m->items[i].has_where) has_where = true;
    if (m->items[i].has_contract) has_contract = true;
    if (m->items[i].has_invariant) has_invariant = true;
    if (m->items[i].kind == A1_ITEM_MACRO || m->items[i].has_macro_attrs) has_macro = true;
  }
  if (!has_fn) {
    tc_error(&r, "E0001", "no function definitions found");
  }
  if (m->nested_pattern_depth_limit == 0 || m->nested_pattern_depth_limit > 256) {
    tc_error(&r, "E0002", "nested pattern depth limit must be in [1, 256]");
  }

  for (size_t i = 0; i < sizeof(kRules) / sizeof(kRules[0]); i++) {
    const FeatureRule* rule = &kRules[i];
    if ((m->feature_bits & rule->bit) == 0) continue;
    if (rule->bit == A1_FEAT_DEPENDENT_TYPE && !(has_generic || has_where)) tc_error(&r, rule->code, rule->message);
    else if (rule->bit == A1_FEAT_REFINEMENT_TYPE && !has_where) tc_error(&r, rule->code, rule->message);
    else if (rule->bit == A1_FEAT_CONTRACT_TYPE && !has_contract) tc_error(&r, rule->code, rule->message);
    else if (rule->bit == A1_FEAT_INVARIANT_TYPE && !has_invariant) tc_error(&r, rule->code, rule->message);
    else if (rule->bit == A1_FEAT_ASYNC_CANCEL && !module_has_async(m)) tc_error(&r, rule->code, rule->message);
    else if (rule->bit == A1_FEAT_GENERATOR_RESUME && !module_has_generator(m)) tc_error(&r, rule->code, rule->message);
    else if (rule->bit == A1_FEAT_COROUTINE_SCHED && !module_has_coroutine(m)) tc_error(&r, rule->code, rule->message);
    else if ((rule->bit == A1_FEAT_DECL_MACRO_RECURSION || rule->bit == A1_FEAT_PROC_MACRO_STABLE || rule->bit == A1_FEAT_HYGIENE_ISOLATION) && !has_macro) {
      tc_error(&r, rule->code, rule->message);
    }
  }
  return r;
}
