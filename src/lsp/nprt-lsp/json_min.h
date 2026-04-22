#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum JKind {
  J_NULL = 0,
  J_BOOL,
  J_NUM,
  J_STR,
  J_ARR,
  J_OBJ,
} JKind;

typedef struct JStr {
  const char* ptr;
  size_t len;
} JStr;

typedef struct JVal JVal;

typedef struct JArr {
  JVal** items;
  size_t len;
} JArr;

typedef struct JObjEntry {
  JStr key;
  JVal* val;
} JObjEntry;

typedef struct JObj {
  JObjEntry* items;
  size_t len;
} JObj;

struct JVal {
  JKind kind;
  union {
    bool b;
    double num;
    JStr str;
    JArr arr;
    JObj obj;
  };
};

typedef struct JArena {
  unsigned char* data;
  size_t len;
  size_t cap;
} JArena;

void jarena_init(JArena* a);
void jarena_free(JArena* a);
void* jarena_alloc(JArena* a, size_t n, size_t align);

bool json_parse(JArena* a, const char* src, size_t len, JVal** out);
JVal* json_obj_get(const JVal* obj, const char* key);
