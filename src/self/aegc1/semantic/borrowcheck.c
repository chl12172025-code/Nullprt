#include "borrowcheck.h"

A1BorrowcheckResult a1_borrowcheck_module(const A1AstModule* m) {
  (void)m;
  A1BorrowcheckResult r;
  r.ok = true;
  r.violations = 0;
  // Placeholder: full ownership/borrowing constraints to be enforced on typed IR graph.
  return r;
}
