#include <stdio.h>
#include <string.h>

void fmt_align_columns(char* text) {
  char* p = text;
  while ((p = strstr(p, "->")) != NULL) {
    if (p > text && p[-1] != ' ') { memmove(p + 1, p, strlen(p) + 1); p[0] = ' '; p++; }
    if (p[2] != ' ') { memmove(p + 3, p + 2, strlen(p + 2) + 1); p[2] = ' '; }
    p += 2;
  }
}

void fmt_chain_per_line(char* text) {
  char* p = text;
  while ((p = strstr(p, ".")) != NULL) {
    if (p > text && p[-1] != '\n' && p[-1] != ' ') {
      memmove(p + 1, p, strlen(p) + 1);
      p[0] = '\n';
    }
    p++;
  }
}
