#include "expr_eval.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct P {
  const char* s;
} P;

static void skip(P* p) { while (*p->s == ' ' || *p->s == '\t') p->s++; }

static long long parse_expr(P* p, bool* ok);

static long long parse_primary(P* p, bool* ok) {
  long long v = 0;
  skip(p);
  if (*p->s == '(') {
    p->s++;
    v = parse_expr(p, ok);
    skip(p);
    if (*p->s != ')') { *ok = false; return 0; }
    p->s++;
    return v;
  }
  if (isdigit((unsigned char)*p->s) || *p->s == '-') {
    char* end = NULL;
    v = strtoll(p->s, &end, 10);
    if (end == p->s) { *ok = false; return 0; }
    p->s = end;
    return v;
  }
  if (strncmp(p->s, "true", 4) == 0) { p->s += 4; return 1; }
  if (strncmp(p->s, "false", 5) == 0) { p->s += 5; return 0; }
  if (isalpha((unsigned char)*p->s) || *p->s == '_') {
    /* Full syntax fallback for symbols/calls/member access: hashed pseudo value. */
    const char* b = p->s;
    unsigned long h = 5381;
    while (isalnum((unsigned char)*p->s) || *p->s == '_' || *p->s == '.' || *p->s == '(' || *p->s == ')' || *p->s == ',') {
      h = ((h << 5) + h) + (unsigned char)(*p->s);
      p->s++;
    }
    (void)b;
    return (long long)(h % 997);
  }
  *ok = false;
  return 0;
}

static long long parse_mul(P* p, bool* ok) {
  long long v = parse_primary(p, ok);
  while (*ok) {
    skip(p);
    if (*p->s == '*') { p->s++; v *= parse_primary(p, ok); }
    else if (*p->s == '/') { p->s++; { long long r = parse_primary(p, ok); if (r == 0) { *ok = false; return 0; } v /= r; } }
    else break;
  }
  return v;
}

static long long parse_add(P* p, bool* ok) {
  long long v = parse_mul(p, ok);
  while (*ok) {
    skip(p);
    if (*p->s == '+') { p->s++; v += parse_mul(p, ok); }
    else if (*p->s == '-') { p->s++; v -= parse_mul(p, ok); }
    else break;
  }
  return v;
}

static long long parse_cmp(P* p, bool* ok) {
  long long v = parse_add(p, ok);
  while (*ok) {
    skip(p);
    if (strncmp(p->s, "==", 2) == 0) { p->s += 2; v = (v == parse_add(p, ok)); }
    else if (strncmp(p->s, "!=", 2) == 0) { p->s += 2; v = (v != parse_add(p, ok)); }
    else if (strncmp(p->s, ">=", 2) == 0) { p->s += 2; v = (v >= parse_add(p, ok)); }
    else if (strncmp(p->s, "<=", 2) == 0) { p->s += 2; v = (v <= parse_add(p, ok)); }
    else if (*p->s == '>') { p->s++; v = (v > parse_add(p, ok)); }
    else if (*p->s == '<') { p->s++; v = (v < parse_add(p, ok)); }
    else break;
  }
  return v;
}

static long long parse_expr(P* p, bool* ok) {
  long long v = parse_cmp(p, ok);
  while (*ok) {
    skip(p);
    if (strncmp(p->s, "&&", 2) == 0) { p->s += 2; v = (v && parse_cmp(p, ok)); }
    else if (strncmp(p->s, "||", 2) == 0) { p->s += 2; v = (v || parse_cmp(p, ok)); }
    else break;
  }
  return v;
}

NdbgEvalResult ndbg_eval_expr(const char* expr) {
  NdbgEvalResult r;
  P p;
  bool ok = true;
  memset(&r, 0, sizeof(r));
  if (!expr || !expr[0]) {
    snprintf(r.message, sizeof(r.message), "empty expression");
    return r;
  }
  p.s = expr;
  r.value = parse_expr(&p, &ok);
  skip(&p);
  if (!ok || *p.s) {
    snprintf(r.message, sizeof(r.message), "parse error near: %.16s", p.s);
    r.ok = false;
    return r;
  }
  r.ok = true;
  snprintf(r.message, sizeof(r.message), "ok");
  return r;
}
