#include "pal.h"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

NprtResultPathList nprt_std_list_dir_simple(const char* path_utf8) {
  (void)path_utf8;
  NprtResultPathList r = {0};
  // Placeholder cross-platform surface: fill in detailed listing later.
  r.ok = true;
  return r;
}

#else
#include <dirent.h>

NprtResultPathList nprt_std_list_dir_simple(const char* path_utf8) {
  NprtResultPathList r = {0};
  DIR* d = opendir(path_utf8);
  if (!d) {
    r.ok = false;
    r.err.kind = NPRT_SYS_NOT_FOUND;
    return r;
  }
  struct dirent* ent;
  while ((ent = readdir(d)) != NULL) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
    char** n = (char**)realloc(r.items, sizeof(char*) * (r.len + 1));
    if (!n) break;
    r.items = n;
    size_t l = strlen(ent->d_name);
    r.items[r.len] = (char*)malloc(l + 1);
    if (r.items[r.len]) memcpy(r.items[r.len], ent->d_name, l + 1);
    r.len++;
  }
  closedir(d);
  r.ok = true;
  return r;
}
#endif

void nprt_std_free_path_list(NprtResultPathList* list) {
  if (!list) return;
  for (size_t i = 0; i < list->len; i++) free(list->items[i]);
  free(list->items);
  list->items = NULL;
  list->len = 0;
}
