#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "../../self/aegc1/runtime/extension_registry.h"
#include "format_core.h"

void fmt_align_columns(char* text);
void fmt_chain_per_line(char* text);
void fmt_sort_imports_and_visibility(char* text);
void fmt_keep_comment_indent(char* text);
void fmt_wrap_single_line_attrs(char* text);
void fmt_wrap_macro_args(char* text);

static void usage(void) {
  fprintf(stderr, "usage: nprt-fmt [--check|--write] <file>\n");
  fprintf(stderr, "       nprt-fmt [--check-all|--write-all] <directory>\n");
  fprintf(stderr, "       nprt-fmt [--diff|--diff-all] <file-or-directory>\n");
}

static const char* file_ext(const char* path) {
  const char* slash = strrchr(path, '/');
  const char* bslash = strrchr(path, '\\');
  const char* base = path;
  const char* dot;
  if (slash && slash + 1 > base) base = slash + 1;
  if (bslash && bslash + 1 > base) base = bslash + 1;
  dot = strrchr(base, '.');
  return dot ? dot : "";
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

static void apply_full_rules(char* out) {
  int detected_tab = 0;
  fmt_normalize_indentation(out, &detected_tab); /* 1,2 */
  fmt_wrap_long_lines(out, 100);                 /* 3,4 */
  fmt_force_control_flow_braces(out);            /* 5,6 */
  fmt_apply_operator_spacing(out);               /* 7,8 */
  fmt_chain_per_line(out);                       /* 9,10,21,22 */
  fmt_align_columns(out);                        /* 11,12,13,14 */
  fmt_sort_imports_and_visibility(out);          /* 15,16,17,18 */
  fmt_keep_comment_indent(out);                  /* 19,20 */
  fmt_wrap_single_line_attrs(out);               /* 23 */
  fmt_wrap_macro_args(out);                      /* 24 */
  normalize_spaces(out);
  (void)detected_tab;
}

static void print_diff_preview(const char* path, const char* before, const char* after) {
  int line = 1;
  const char* b = before;
  const char* a = after;
  printf("diff: %s\n", path);
  while (*b || *a) {
    char bl[256] = {0};
    char al[256] = {0};
    int bi = 0;
    int ai = 0;
    while (*b && *b != '\n' && bi < 255) bl[bi++] = *b++;
    while (*a && *a != '\n' && ai < 255) al[ai++] = *a++;
    if (*b == '\n') b++;
    if (*a == '\n') a++;
    if (strcmp(bl, al) != 0) {
      printf("L%d - %s\n", line, bl);
      printf("L%d + %s\n", line, al);
    }
    line++;
  }
}

static int process_file(const char* path, bool check, bool write, bool diff) {
  const NprtExtensionInfo* info = nprt_extension_lookup(file_ext(path));
  if (!info || strcmp(info->kind, "source") != 0) {
    return 0;
  }
  FILE* f = fopen(path, "rb");
  if (!f) return -1;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return -1; }
  fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[n] = '\0';
  char* out = (char*)malloc((size_t)n + 1);
  if (!out) { free(buf); return -1; }
  memcpy(out, buf, (size_t)n + 1);
  apply_full_rules(out);
  int changed = strcmp(buf, out) != 0;
  if (diff && changed) {
    print_diff_preview(path, buf, out);
  } else if (check) {
    printf(changed ? "needs-format: %s\n" : "ok: %s\n", path);
  } else if (write && changed) {
    FILE* wf = fopen(path, "wb");
    if (!wf) { free(buf); free(out); return -1; }
    fwrite(out, 1, strlen(out), wf);
    fclose(wf);
    printf("formatted: %s\n", path);
  } else {
    printf("unchanged: %s\n", path);
  }
  free(buf);
  free(out);
  return changed ? 1 : 0;
}

typedef struct FmtStats {
  int scanned;
  int changed;
  int errors;
} FmtStats;

#if defined(_WIN32)
static void walk_dir(const char* dir, bool check, bool write, bool diff, FmtStats* stats) {
  char pattern[2048];
  WIN32_FIND_DATAA data;
  HANDLE h;
  snprintf(pattern, sizeof(pattern), "%s\\*", dir);
  h = FindFirstFileA(pattern, &data);
  if (h == INVALID_HANDLE_VALUE) return;
  do {
    char path[2048];
    if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, "..")) continue;
    snprintf(path, sizeof(path), "%s\\%s", dir, data.cFileName);
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      walk_dir(path, check, write, diff, stats);
    } else {
      int rc = process_file(path, check, write, diff);
      if (rc >= 0) stats->scanned++;
      if (rc > 0) stats->changed++;
      if (rc < 0) stats->errors++;
    }
  } while (FindNextFileA(h, &data));
  FindClose(h);
}
#else
static void walk_dir(const char* dir, bool check, bool write, bool diff, FmtStats* stats) {
  DIR* d = opendir(dir);
  struct dirent* ent;
  if (!d) return;
  while ((ent = readdir(d)) != NULL) {
    char path[2048];
    struct stat st;
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
    snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
    if (stat(path, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)) {
      walk_dir(path, check, write, diff, stats);
    } else {
      int rc = process_file(path, check, write, diff);
      if (rc >= 0) stats->scanned++;
      if (rc > 0) stats->changed++;
      if (rc < 0) stats->errors++;
    }
  }
  closedir(d);
}
#endif

int main(int argc, char** argv) {
  if (argc < 3) { usage(); return 2; }
  if (!strcmp(argv[1], "--check") || !strcmp(argv[1], "--write")) {
    bool check = !strcmp(argv[1], "--check");
    bool write = !strcmp(argv[1], "--write");
    int rc = process_file(argv[2], check, write, false);
    if (rc < 0) return 1;
    return (check && rc > 0) ? 1 : 0;
  }
  if (!strcmp(argv[1], "--diff")) {
    int rc = process_file(argv[2], false, false, true);
    if (rc < 0) return 1;
    return rc > 0 ? 1 : 0;
  }
  if (!strcmp(argv[1], "--check-all") || !strcmp(argv[1], "--write-all")) {
    bool check = !strcmp(argv[1], "--check-all");
    bool write = !strcmp(argv[1], "--write-all");
    FmtStats stats = {0, 0, 0};
    walk_dir(argv[2], check, write, false, &stats);
    printf("summary: scanned=%d changed=%d errors=%d\n", stats.scanned, stats.changed, stats.errors);
    if (stats.errors > 0) return 1;
    return (check && stats.changed > 0) ? 1 : 0;
  }
  if (!strcmp(argv[1], "--diff-all")) {
    FmtStats stats = {0, 0, 0};
    walk_dir(argv[2], false, false, true, &stats);
    printf("summary: scanned=%d changed=%d errors=%d\n", stats.scanned, stats.changed, stats.errors);
    if (stats.errors > 0) return 1;
    return stats.changed > 0 ? 1 : 0;
  }
  usage();
  return 2;
}
