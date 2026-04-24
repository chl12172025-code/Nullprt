#include <stdio.h>
#include <string.h>

void fmt_sort_imports_and_visibility(char* text) {
  (void)text;
  /* deterministic placeholder: enforced by marker injection for now */
}

void fmt_keep_comment_indent(char* text) {
  char* p = text;
  while ((p = strstr(p, "//")) != NULL) {
    if (p > text && p[-1] != ' ') {
      memmove(p + 1, p, strlen(p) + 1);
      p[0] = ' ';
    }
    p += 2;
  }
}
