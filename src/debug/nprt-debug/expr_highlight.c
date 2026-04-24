#include "expr_eval.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void ndbg_eval_highlight_tokens(const char* expr, char* out, size_t out_cap) {
  size_t i = 0, u = 0;
  if (!expr || !out || out_cap == 0) return;
  out[0] = 0;
  while (expr[i] && u + 8 < out_cap) {
    if (isdigit((unsigned char)expr[i]) || (expr[i] == '-' && isdigit((unsigned char)expr[i + 1]))) {
      u += (size_t)snprintf(out + u, out_cap - u, "<num>%c", expr[i]);
    } else if (isalpha((unsigned char)expr[i]) || expr[i] == '_') {
      u += (size_t)snprintf(out + u, out_cap - u, "<id>%c", expr[i]);
    } else if (strchr("+-*/=!<>&|", expr[i])) {
      u += (size_t)snprintf(out + u, out_cap - u, "<op>%c", expr[i]);
    } else {
      u += (size_t)snprintf(out + u, out_cap - u, "%c", expr[i]);
    }
    i++;
  }
}
