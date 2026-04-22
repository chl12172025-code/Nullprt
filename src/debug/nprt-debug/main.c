#include <stdio.h>
#include <string.h>

#include "../../self/aegc1/runtime/dev_gate.h"

static void usage(void) {
  fprintf(stderr, "usage: nprt-debug <attach|detach|break|step|stack> [args]\n");
}

int main(int argc, char** argv) {
  if (argc < 2) { usage(); return 2; }
  NprtResearchToken tok = nprt_authorize_research("nprt-debug");
  if (!nprt_research_token_is_valid(&tok)) {
    fprintf(stderr, "nprt-debug: blocked by developer research gate\n");
    return 1;
  }
  nprt_research_log_event("nprt-debug", argv[1]);
  if (!strcmp(argv[1], "attach")) printf("attach ok (platform backend selected)\n");
  else if (!strcmp(argv[1], "detach")) printf("detach ok\n");
  else if (!strcmp(argv[1], "break")) printf("breakpoint set\n");
  else if (!strcmp(argv[1], "step")) printf("step ok\n");
  else if (!strcmp(argv[1], "stack")) printf("stack frame[0]=main\n");
  else { usage(); return 2; }
  return 0;
}
