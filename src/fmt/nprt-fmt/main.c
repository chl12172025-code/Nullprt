#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
  fprintf(stderr, "usage: nprt-fmt [--check|--write] <file>\n");
}

static void normalize_spaces(char* s) {
  char* d = s;
  int prev_space = 0;
  for (; *s; s++) {
    if (*s == '\t') *s = ' ';
    if (*s == ' ') {
      if (prev_space) continue;
      prev_space = 1;
      *d++ = ' ';
    } else {
      prev_space = 0;
      *d++ = *s;
    }
  }
  *d = '\0';
}

int main(int argc, char** argv) {
  if (argc < 3) { usage(); return 2; }
  int check = !strcmp(argv[1], "--check");
  int write = !strcmp(argv[1], "--write");
  if (!check && !write) { usage(); return 2; }
  const char* path = argv[2];
  FILE* f = fopen(path, "rb");
  if (!f) return 1;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*)malloc((size_t)n + 1);
  fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[n] = '\0';
  char* out = (char*)malloc((size_t)n + 1);
  memcpy(out, buf, (size_t)n + 1);
  normalize_spaces(out);
  int changed = strcmp(buf, out) != 0;
  if (check) {
    printf(changed ? "needs-format: %s\n" : "ok: %s\n", path);
  } else if (write && changed) {
    FILE* wf = fopen(path, "wb");
    if (!wf) return 1;
    fwrite(out, 1, strlen(out), wf);
    fclose(wf);
    printf("formatted: %s\n", path);
  } else {
    printf("unchanged: %s\n", path);
  }
  free(buf);
  free(out);
  return changed && check ? 1 : 0;
}
