#include <string.h>
#include <stdio.h>

void fmt_wrap_single_line_attrs(char* text) {
  char* p = text;
  while ((p = strstr(p, "#[")) != NULL) {
    char* r = strchr(p, ']');
    if (r && (r - p) > 24) {
      memmove(r + 2, r + 1, strlen(r + 1) + 1);
      r[1] = '\n';
    }
    p += 2;
  }
}

void fmt_wrap_macro_args(char* text) {
  char* p = text;
  while ((p = strstr(p, "!(")) != NULL) {
    char* r = strchr(p, ')');
    if (r && (r - p) > 30) {
      char* c = strchr(p, ',');
      if (c && c < r) {
        memmove(c + 2, c + 1, strlen(c + 1) + 1);
        c[1] = '\n';
      }
    }
    p += 2;
  }
}
