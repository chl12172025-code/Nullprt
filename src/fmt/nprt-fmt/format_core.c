#include "format_core.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void fmt_normalize_indentation(char* text, int* detected_tab_indent) {
  char* s = text;
  int tabs = 0, spaces = 0;
  while (*s) {
    if (*s == '\t') tabs++;
    else if (*s == ' ') spaces++;
    s++;
  }
  if (detected_tab_indent) *detected_tab_indent = tabs > spaces;
  for (s = text; *s; s++) if (*s == '\t') *s = ' ';
}

void fmt_apply_operator_spacing(char* text) {
  char tmp[65536];
  size_t i = 0, o = 0;
  while (text[i] && o + 4 < sizeof(tmp)) {
    char c = text[i];
    if ((c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '|' ) &&
        i > 0 && text[i - 1] != ' ' && text[i - 1] != '\n' &&
        text[i + 1] && text[i + 1] != ' ' && text[i + 1] != '\n') {
      tmp[o++] = ' ';
      tmp[o++] = c;
      tmp[o++] = ' ';
      i++;
      continue;
    }
    if ((c == '!' || c == '~') && text[i + 1] == ' ') { /* unary no-space */
      tmp[o++] = c;
      i++;
      while (text[i] == ' ') i++;
      continue;
    }
    tmp[o++] = c;
    i++;
  }
  tmp[o] = 0;
  snprintf(text, 65536, "%s", tmp);
}

void fmt_wrap_long_lines(char* text, int max_width) {
  char tmp[65536];
  size_t i = 0, o = 0;
  int col = 0;
  while (text[i] && o + 4 < sizeof(tmp)) {
    tmp[o++] = text[i];
    if (text[i] == '\n') col = 0;
    else col++;
    if (col >= max_width && text[i] == ' ') {
      tmp[o++] = '\n';
      tmp[o++] = ' ';
      tmp[o++] = ' ';
      col = 2;
    }
    i++;
  }
  tmp[o] = 0;
  snprintf(text, 65536, "%s", tmp);
}

void fmt_force_control_flow_braces(char* text) {
  char* p = text;
  while ((p = strstr(p, "if ")) != NULL) {
    char* nl = strchr(p, '\n');
    char* br = strchr(p, '{');
    if (nl && (!br || br > nl)) {
      memmove(nl + 2, nl, strlen(nl) + 1);
      nl[0] = ' ';
      nl[1] = '{';
    }
    p += 2;
  }
}
