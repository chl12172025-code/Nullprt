#include "json_min.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static size_t align_up(size_t x, size_t a) { size_t m = a - 1; return (x + m) & ~m; }

void jarena_init(JArena* a) { a->data = NULL; a->len = 0; a->cap = 0; }
void jarena_free(JArena* a) { free(a->data); a->data = NULL; a->len = 0; a->cap = 0; }
void* jarena_alloc(JArena* a, size_t n, size_t align) {
  if (align < 1) align = 1;
  if ((align & (align - 1)) != 0) align = sizeof(void*);
  size_t start = align_up(a->len, align);
  size_t need = start + n;
  if (need > a->cap) {
    size_t nc = a->cap ? a->cap : 4096;
    while (nc < need) nc *= 2;
    unsigned char* p = (unsigned char*)realloc(a->data, nc);
    if (!p) return NULL;
    a->data = p;
    a->cap = nc;
  }
  void* out = a->data + start;
  memset(out, 0, n);
  a->len = need;
  return out;
}

typedef struct P {
  const char* s;
  size_t n;
  size_t i;
} P;

static void ws(P* p) { while (p->i < p->n && (p->s[p->i]==' '||p->s[p->i]=='\n'||p->s[p->i]=='\r'||p->s[p->i]=='\t')) p->i++; }
static bool match(P* p, char c) { ws(p); if (p->i < p->n && p->s[p->i]==c) { p->i++; return true; } return false; }
static bool starts(P* p, const char* lit) {
  size_t k = strlen(lit);
  if (p->i + k > p->n) return false;
  return memcmp(p->s + p->i, lit, k) == 0;
}

static JVal* parse_val(JArena* a, P* p);

static JVal* newv(JArena* a) { return (JVal*)jarena_alloc(a, sizeof(JVal), _Alignof(JVal)); }

static JVal* parse_str(JArena* a, P* p) {
  ws(p);
  if (p->i >= p->n || p->s[p->i] != '"') return NULL;
  p->i++;
  size_t start = p->i;
  while (p->i < p->n && p->s[p->i] != '"') {
    if (p->s[p->i] == '\\' && p->i + 1 < p->n) { p->i += 2; continue; }
    p->i++;
  }
  if (p->i >= p->n) return NULL;
  size_t end = p->i;
  p->i++;
  JVal* v = newv(a);
  if (!v) return NULL;
  v->kind = J_STR;
  v->str.ptr = p->s + start;
  v->str.len = end - start;
  return v;
}

static JVal* parse_num(JArena* a, P* p) {
  ws(p);
  size_t start = p->i;
  if (p->i < p->n && (p->s[p->i] == '-' )) p->i++;
  while (p->i < p->n && isdigit((unsigned char)p->s[p->i])) p->i++;
  if (p->i < p->n && p->s[p->i] == '.') { p->i++; while (p->i < p->n && isdigit((unsigned char)p->s[p->i])) p->i++; }
  size_t end = p->i;
  if (end == start) return NULL;
  char tmp[64];
  size_t m = end - start;
  if (m >= sizeof(tmp)) return NULL;
  memcpy(tmp, p->s + start, m);
  tmp[m] = 0;
  JVal* v = newv(a);
  if (!v) return NULL;
  v->kind = J_NUM;
  v->num = strtod(tmp, NULL);
  return v;
}

static JVal* parse_arr(JArena* a, P* p) {
  if (!match(p, '[')) return NULL;
  JVal* v = newv(a);
  if (!v) return NULL;
  v->kind = J_ARR;
  v->arr.items = NULL;
  v->arr.len = 0;
  ws(p);
  if (match(p, ']')) return v;
  for (;;) {
    JVal* it = parse_val(a, p);
    if (!it) return NULL;
    v->arr.len++;
    JVal** new_items = (JVal**)jarena_alloc(a, sizeof(JVal*) * v->arr.len, _Alignof(JVal*));
    if (v->arr.items) memcpy(new_items, v->arr.items, sizeof(JVal*) * (v->arr.len - 1));
    new_items[v->arr.len - 1] = it;
    v->arr.items = new_items;
    ws(p);
    if (match(p, ',')) continue;
    if (match(p, ']')) break;
    return NULL;
  }
  return v;
}

static JVal* parse_obj(JArena* a, P* p) {
  if (!match(p, '{')) return NULL;
  JVal* v = newv(a);
  if (!v) return NULL;
  v->kind = J_OBJ;
  v->obj.items = NULL;
  v->obj.len = 0;
  ws(p);
  if (match(p, '}')) return v;
  for (;;) {
    JVal* k = parse_str(a, p);
    if (!k) return NULL;
    if (!match(p, ':')) return NULL;
    JVal* val = parse_val(a, p);
    if (!val) return NULL;
    v->obj.len++;
    JObjEntry* ents = (JObjEntry*)jarena_alloc(a, sizeof(JObjEntry) * v->obj.len, _Alignof(JObjEntry));
    if (v->obj.items) memcpy(ents, v->obj.items, sizeof(JObjEntry) * (v->obj.len - 1));
    ents[v->obj.len - 1].key = k->str;
    ents[v->obj.len - 1].val = val;
    v->obj.items = ents;
    ws(p);
    if (match(p, ',')) continue;
    if (match(p, '}')) break;
    return NULL;
  }
  return v;
}

static JVal* parse_val(JArena* a, P* p) {
  ws(p);
  if (p->i >= p->n) return NULL;
  char c = p->s[p->i];
  if (c == '"') return parse_str(a, p);
  if (c == '{') return parse_obj(a, p);
  if (c == '[') return parse_arr(a, p);
  if (c == '-' || isdigit((unsigned char)c)) return parse_num(a, p);
  if (starts(p, "true")) { p->i += 4; JVal* v=newv(a); v->kind=J_BOOL; v->b=true; return v; }
  if (starts(p, "false")) { p->i += 5; JVal* v=newv(a); v->kind=J_BOOL; v->b=false; return v; }
  if (starts(p, "null")) { p->i += 4; JVal* v=newv(a); v->kind=J_NULL; return v; }
  return NULL;
}

bool json_parse(JArena* a, const char* src, size_t len, JVal** out) {
  P p = {src, len, 0};
  JVal* v = parse_val(a, &p);
  if (!v) return false;
  ws(&p);
  if (p.i != p.n) return false;
  *out = v;
  return true;
}

JVal* json_obj_get(const JVal* obj, const char* key) {
  if (!obj || obj->kind != J_OBJ) return NULL;
  size_t klen = strlen(key);
  for (size_t i = 0; i < obj->obj.len; i++) {
    if (obj->obj.items[i].key.len == klen && memcmp(obj->obj.items[i].key.ptr, key, klen) == 0) return obj->obj.items[i].val;
  }
  return NULL;
}
