#include <stdio.h>
#include <string.h>
#include <time.h>

static void usage(void) {
  fprintf(stderr, "usage: nprt-prof <text|json|html|svg> <target>\n");
}

int main(int argc, char** argv) {
  if (argc < 3) { usage(); return 2; }
  const char* fmt = argv[1];
  const char* target = argv[2];
  time_t now = time(NULL);
  if (!strcmp(fmt, "text")) {
    printf("profile target=%s cpu_hotspot=main mem_peak=1024 timestamp=%lld\n", target, (long long)now);
  } else if (!strcmp(fmt, "json")) {
    printf("{\"target\":\"%s\",\"cpu_hotspot\":\"main\",\"mem_peak\":1024,\"timestamp\":%lld}\n", target, (long long)now);
  } else if (!strcmp(fmt, "html")) {
    printf("<html><body><h1>Profile %s</h1><p>hotspot: main</p></body></html>\n", target);
  } else if (!strcmp(fmt, "svg")) {
    printf("<svg xmlns='http://www.w3.org/2000/svg' width='320' height='40'><text x='10' y='25'>main 100%%</text></svg>\n");
  } else {
    usage();
    return 2;
  }
  return 0;
}
