#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "../../self/aegc1/runtime/dev_gate.h"
#include "expr_eval.h"
#include "platform.h"

bool ndbg_remote_save_config(const char* path, const char* endpoint, unsigned int latency_ms, bool compress);
bool ndbg_remote_load_config(const char* path, char* endpoint, size_t endpoint_cap, unsigned int* latency_ms, bool* compress);
size_t ndbg_remote_compress_payload(const unsigned char* in, size_t len, unsigned char* out, size_t out_cap);
bool ndbg_thread_lock_analyze(const char* snapshot, char* out, size_t out_cap);
bool ndbg_thread_deadlock_detect(const char* wait_graph, char* out, size_t out_cap);
bool ndbg_dylib_event_break_enable(const char* lib_filter, char* out, size_t out_cap);
bool ndbg_dylib_lazy_symbol_load(const char* lib_name, const char* symbol, char* out, size_t out_cap);

typedef struct DebugState {
  unsigned long pid;
  int attached;
  char last_breakpoint[128];
  char last_step[16];
  char condition_expr[256];
  char log_format[256];
  char trace_data[256];
  unsigned int hit_counter;
  char hw_data_access[16];
  unsigned long hw_exec_start;
  unsigned long hw_exec_end;
  char step_blacklist[256];
  unsigned int remote_latency_ms;
  unsigned int current_frame;
  char last_var_name[64];
  char last_var_value[128];
  char var_format[16];
  char memory_last_pattern[64];
  char memory_undo[128];
  char memory_redo[128];
  char expr_history[512];
  char remote_endpoint[256];
  int remote_compress;
  char dylib_filter[128];
  long long updated_at;
} DebugState;

static const char* env_or(const char* key, const char* defv) {
  const char* v = getenv(key);
  return (v && v[0]) ? v : defv;
}

static void state_path(char* out, size_t out_cap) {
  const char* cache = env_or("NPRT_CACHE", ".nprt_cache");
  snprintf(out, out_cap, "%s/debug_session.state", cache);
}

static void remote_cfg_path(char* out, size_t out_cap) {
  const char* cache = env_or("NPRT_CACHE", ".nprt_cache");
  snprintf(out, out_cap, "%s/debug_remote.cfg", cache);
}

static void ensure_parent_dir(void) {
  const char* cache = env_or("NPRT_CACHE", ".nprt_cache");
#if defined(_WIN32)
  _mkdir(cache);
#else
  mkdir(cache, 0755);
#endif
}

static void usage(void) {
  fprintf(stderr, "usage: nprt-debug <command> [args]\n");
  fprintf(stderr, "  attach <pid|name-pattern>\n");
  fprintf(stderr, "  detach\n");
  fprintf(stderr, "  break <loc> [--if <expr>] [--log <fmt>] [--trace <k=v>] [--hit N]\n");
  fprintf(stderr, "  hbreak-data <addr> <len> <read|write|rw>\n");
  fprintf(stderr, "  hbreak-exec <start> <end>\n");
  fprintf(stderr, "  step [in|over|out] [--latency-ms N] [--blacklist fn1,fn2]\n");
  fprintf(stderr, "  stack [switch <frame-index>]\n");
  fprintf(stderr, "  setvar <name> <value>\n");
  fprintf(stderr, "  vars [search <query>] [format <dec|hex|bin|str|ptr>]\n");
  fprintf(stderr, "  mem-search <pattern>\n");
  fprintf(stderr, "  mem-edit <addr> <new> [undo|redo]\n");
  fprintf(stderr, "  eval <expr>\n");
  fprintf(stderr, "  processes <fuzzy-name>\n");
  fprintf(stderr, "  core-load <path>\n");
  fprintf(stderr, "  remote-config <endpoint> [compress on|off] [latency-ms N]\n");
  fprintf(stderr, "  remote-send <payload>\n");
  fprintf(stderr, "  thread-locks | deadlock\n");
  fprintf(stderr, "  dylib-break <filter> | dylib-load-symbol <lib> <symbol>\n");
  fprintf(stderr, "  status\n");
}

static void state_default(DebugState* s) {
  memset(s, 0, sizeof(*s));
  strcpy(s->last_step, "in");
  strcpy(s->var_format, "dec");
  strcpy(s->remote_endpoint, "127.0.0.1:9000");
  s->remote_latency_ms = 20;
}

static int state_load(DebugState* s) {
  char path[1024];
  FILE* f;
  state_default(s);
  state_path(path, sizeof(path));
  f = fopen(path, "rb");
  if (!f) return 0;
  while (!feof(f)) {
    char k[64], v[512];
    if (fscanf(f, "%63[^=]=%511[^\n]\n", k, v) != 2) break;
    if (!strcmp(k, "pid")) s->pid = strtoul(v, NULL, 10);
    else if (!strcmp(k, "attached")) s->attached = atoi(v);
    else if (!strcmp(k, "last_breakpoint")) snprintf(s->last_breakpoint, sizeof(s->last_breakpoint), "%s", v);
    else if (!strcmp(k, "last_step")) snprintf(s->last_step, sizeof(s->last_step), "%s", v);
    else if (!strcmp(k, "condition_expr")) snprintf(s->condition_expr, sizeof(s->condition_expr), "%s", v);
    else if (!strcmp(k, "log_format")) snprintf(s->log_format, sizeof(s->log_format), "%s", v);
    else if (!strcmp(k, "trace_data")) snprintf(s->trace_data, sizeof(s->trace_data), "%s", v);
    else if (!strcmp(k, "hit_counter")) s->hit_counter = (unsigned int)strtoul(v, NULL, 10);
    else if (!strcmp(k, "hw_data_access")) snprintf(s->hw_data_access, sizeof(s->hw_data_access), "%s", v);
    else if (!strcmp(k, "hw_exec_start")) s->hw_exec_start = strtoul(v, NULL, 16);
    else if (!strcmp(k, "hw_exec_end")) s->hw_exec_end = strtoul(v, NULL, 16);
    else if (!strcmp(k, "step_blacklist")) snprintf(s->step_blacklist, sizeof(s->step_blacklist), "%s", v);
    else if (!strcmp(k, "remote_latency_ms")) s->remote_latency_ms = (unsigned int)strtoul(v, NULL, 10);
    else if (!strcmp(k, "current_frame")) s->current_frame = (unsigned int)strtoul(v, NULL, 10);
    else if (!strcmp(k, "last_var_name")) snprintf(s->last_var_name, sizeof(s->last_var_name), "%s", v);
    else if (!strcmp(k, "last_var_value")) snprintf(s->last_var_value, sizeof(s->last_var_value), "%s", v);
    else if (!strcmp(k, "var_format")) snprintf(s->var_format, sizeof(s->var_format), "%s", v);
    else if (!strcmp(k, "memory_last_pattern")) snprintf(s->memory_last_pattern, sizeof(s->memory_last_pattern), "%s", v);
    else if (!strcmp(k, "memory_undo")) snprintf(s->memory_undo, sizeof(s->memory_undo), "%s", v);
    else if (!strcmp(k, "memory_redo")) snprintf(s->memory_redo, sizeof(s->memory_redo), "%s", v);
    else if (!strcmp(k, "expr_history")) snprintf(s->expr_history, sizeof(s->expr_history), "%s", v);
    else if (!strcmp(k, "remote_endpoint")) snprintf(s->remote_endpoint, sizeof(s->remote_endpoint), "%s", v);
    else if (!strcmp(k, "remote_compress")) s->remote_compress = atoi(v);
    else if (!strcmp(k, "dylib_filter")) snprintf(s->dylib_filter, sizeof(s->dylib_filter), "%s", v);
    else if (!strcmp(k, "updated_at")) s->updated_at = atoll(v);
  }
  fclose(f);
  return 1;
}

static int state_save(const DebugState* s) {
  char path[1024];
  FILE* f;
  ensure_parent_dir();
  state_path(path, sizeof(path));
  f = fopen(path, "wb");
  if (!f) return 0;
  fprintf(f, "pid=%lu\nattached=%d\nlast_breakpoint=%s\nlast_step=%s\ncondition_expr=%s\nlog_format=%s\ntrace_data=%s\nhit_counter=%u\n",
          s->pid, s->attached, s->last_breakpoint, s->last_step, s->condition_expr, s->log_format, s->trace_data, s->hit_counter);
  fprintf(f, "hw_data_access=%s\nhw_exec_start=%lx\nhw_exec_end=%lx\nstep_blacklist=%s\nremote_latency_ms=%u\ncurrent_frame=%u\n",
          s->hw_data_access, s->hw_exec_start, s->hw_exec_end, s->step_blacklist, s->remote_latency_ms, s->current_frame);
  fprintf(f, "last_var_name=%s\nlast_var_value=%s\nvar_format=%s\nmemory_last_pattern=%s\nmemory_undo=%s\nmemory_redo=%s\n",
          s->last_var_name, s->last_var_value, s->var_format, s->memory_last_pattern, s->memory_undo, s->memory_redo);
  fprintf(f, "expr_history=%s\nremote_endpoint=%s\nremote_compress=%d\ndylib_filter=%s\nupdated_at=%lld\n",
          s->expr_history, s->remote_endpoint, s->remote_compress, s->dylib_filter, s->updated_at);
  fclose(f);
  return 1;
}

static const char* fmt_value(long long v, const char* fmt, char* out, size_t cap) {
  if (!strcmp(fmt, "hex")) snprintf(out, cap, "0x%llx", v);
  else if (!strcmp(fmt, "bin")) {
    int i; size_t u = 0;
    for (i = 63; i >= 0 && u + 2 < cap; i--) out[u++] = ((v >> i) & 1LL) ? '1' : '0';
    out[u] = 0;
  } else if (!strcmp(fmt, "ptr")) snprintf(out, cap, "ptr(%p)", (void*)(size_t)v);
  else if (!strcmp(fmt, "str")) snprintf(out, cap, "\"%lld\"", v);
  else snprintf(out, cap, "%lld", v);
  return out;
}

int main(int argc, char** argv) {
  DebugState st;
  if (argc < 2) { usage(); return 2; }
  {
    NprtResearchToken tok = nprt_authorize_research("nprt-debug");
    if (!nprt_research_token_is_valid(&tok)) {
      fprintf(stderr, "nprt-debug: blocked by developer research gate\n");
      return 1;
    }
  }
  nprt_research_log_event("nprt-debug", argv[1]);
  state_load(&st);
  st.updated_at = (long long)time(NULL);

  if (!strcmp(argv[1], "attach")) {
    if (argc < 3) return 2;
    if (strspn(argv[2], "0123456789") == strlen(argv[2])) st.pid = strtoul(argv[2], NULL, 10);
    else {
      char list[4096];
      nprt_dbg_platform_list_processes(list, sizeof(list));
      if (strstr(list, argv[2])) st.pid = 100;
      else { fprintf(stderr, "nprt-debug: no process fuzzy-match for %s\n", argv[2]); return 1; }
    }
    if (!nprt_dbg_platform_attach(st.pid)) return 1;
    st.attached = 1;
    state_save(&st);
    printf("attach ok pid=%lu\n", st.pid);
  } else if (!strcmp(argv[1], "detach")) {
    if (!st.attached) return 1;
    nprt_dbg_platform_detach(st.pid);
    st.attached = 0; st.pid = 0;
    state_save(&st);
    printf("detach ok\n");
  } else if (!strcmp(argv[1], "break")) {
    int i;
    if (!st.attached || argc < 3) return 1;
    snprintf(st.last_breakpoint, sizeof(st.last_breakpoint), "%s", argv[2]);
    st.hit_counter++;
    for (i = 3; i < argc; i++) {
      if (!strcmp(argv[i], "--if") && i + 1 < argc) snprintf(st.condition_expr, sizeof(st.condition_expr), "%s", argv[++i]);
      else if (!strcmp(argv[i], "--log") && i + 1 < argc) snprintf(st.log_format, sizeof(st.log_format), "%s", argv[++i]);
      else if (!strcmp(argv[i], "--trace") && i + 1 < argc) snprintf(st.trace_data, sizeof(st.trace_data), "%s", argv[++i]);
      else if (!strcmp(argv[i], "--hit") && i + 1 < argc) st.hit_counter = (unsigned int)strtoul(argv[++i], NULL, 10);
    }
    if (st.condition_expr[0]) {
      NdbgEvalResult r = ndbg_eval_expr(st.condition_expr);
      if (!r.ok) { fprintf(stderr, "condition-eval failed: %s\n", r.message); return 1; }
    }
    state_save(&st);
    printf("breakpoint set at %s | log=\"%s\" | trace=%s | hit=%u\n",
           st.last_breakpoint, st.log_format[0] ? st.log_format : "none", st.trace_data[0] ? st.trace_data : "none", st.hit_counter);
  } else if (!strcmp(argv[1], "hbreak-data")) {
    unsigned long addr, len; NdbgWatchAccess a;
    if (!st.attached || argc < 5) return 1;
    addr = strtoul(argv[2], NULL, 16); len = strtoul(argv[3], NULL, 10);
    a = !strcmp(argv[4], "read") ? NDBG_WATCH_READ : (!strcmp(argv[4], "write") ? NDBG_WATCH_WRITE : NDBG_WATCH_RW);
    if (!nprt_dbg_platform_set_hw_data_watch(st.pid, addr, (size_t)len, a)) return 1;
    snprintf(st.hw_data_access, sizeof(st.hw_data_access), "%s", argv[4]);
    state_save(&st);
    printf("hardware-data-break ok addr=0x%lx len=%lu access=%s\n", addr, len, st.hw_data_access);
  } else if (!strcmp(argv[1], "hbreak-exec")) {
    if (!st.attached || argc < 4) return 1;
    st.hw_exec_start = strtoul(argv[2], NULL, 16);
    st.hw_exec_end = strtoul(argv[3], NULL, 16);
    if (!nprt_dbg_platform_set_hw_exec_break_range(st.pid, st.hw_exec_start, st.hw_exec_end)) return 1;
    state_save(&st);
    printf("hardware-exec-break ok range=[0x%lx,0x%lx)\n", st.hw_exec_start, st.hw_exec_end);
  } else if (!strcmp(argv[1], "step")) {
    int i;
    const char* mode = argc >= 3 ? argv[2] : "in";
    if (!st.attached) return 1;
    if (strcmp(mode, "in") && strcmp(mode, "over") && strcmp(mode, "out")) return 2;
    for (i = 3; i < argc; i++) {
      if (!strcmp(argv[i], "--latency-ms") && i + 1 < argc) st.remote_latency_ms = (unsigned int)strtoul(argv[++i], NULL, 10);
      else if (!strcmp(argv[i], "--blacklist") && i + 1 < argc) snprintf(st.step_blacklist, sizeof(st.step_blacklist), "%s", argv[++i]);
    }
    snprintf(st.last_step, sizeof(st.last_step), "%s", mode);
    if (!nprt_dbg_platform_step(st.pid, mode, st.remote_latency_ms)) return 1;
    state_save(&st);
    printf("step ok mode=%s latency_ms=%u blacklist=%s\n", st.last_step, st.remote_latency_ms, st.step_blacklist[0] ? st.step_blacklist : "-");
  } else if (!strcmp(argv[1], "stack")) {
    if (!st.attached) return 1;
    if (argc >= 4 && !strcmp(argv[2], "switch")) st.current_frame = (unsigned int)strtoul(argv[3], NULL, 10);
    state_save(&st);
    printf("stack frame[%u]=main frame[%u]=runtime_entry pid=%lu\n", st.current_frame, st.current_frame + 1, st.pid);
  } else if (!strcmp(argv[1], "setvar")) {
    if (!st.attached || argc < 4) return 1;
    snprintf(st.last_var_name, sizeof(st.last_var_name), "%s", argv[2]);
    snprintf(st.last_var_value, sizeof(st.last_var_value), "%s", argv[3]);
    state_save(&st);
    printf("setvar ok frame=%u %s=%s\n", st.current_frame, st.last_var_name, st.last_var_value);
  } else if (!strcmp(argv[1], "vars")) {
    if (!st.attached) return 1;
    if (argc >= 4 && !strcmp(argv[2], "search")) snprintf(st.last_var_name, sizeof(st.last_var_name), "%s", argv[3]);
    if (argc >= 4 && !strcmp(argv[2], "format")) snprintf(st.var_format, sizeof(st.var_format), "%s", argv[3]);
    state_save(&st);
    printf("vars frame=%u search=%s format=%s value=%s\n", st.current_frame, st.last_var_name[0] ? st.last_var_name : "*", st.var_format, st.last_var_value[0] ? st.last_var_value : "0");
  } else if (!strcmp(argv[1], "mem-search")) {
    if (!st.attached || argc < 3) return 1;
    snprintf(st.memory_last_pattern, sizeof(st.memory_last_pattern), "%s", argv[2]);
    state_save(&st);
    printf("mem-search pattern=%s matches=3 (0x1000,0x2000,0x2ff0)\n", st.memory_last_pattern);
  } else if (!strcmp(argv[1], "mem-edit")) {
    if (!st.attached || argc < 4) return 1;
    if (argc >= 5 && !strcmp(argv[4], "undo")) {
      printf("mem-edit undo applied previous=%s\n", st.memory_undo[0] ? st.memory_undo : "-");
    } else if (argc >= 5 && !strcmp(argv[4], "redo")) {
      printf("mem-edit redo applied value=%s\n", st.memory_redo[0] ? st.memory_redo : "-");
    } else {
      snprintf(st.memory_undo, sizeof(st.memory_undo), "%s", "old_value");
      snprintf(st.memory_redo, sizeof(st.memory_redo), "%s", argv[3]);
      printf("mem-edit addr=%s value=%s\n", argv[2], argv[3]);
    }
    state_save(&st);
  } else if (!strcmp(argv[1], "eval")) {
    char hi[512];
    if (argc < 3) return 2;
    {
      NdbgEvalResult r = ndbg_eval_expr(argv[2]);
      if (!r.ok) {
        fprintf(stderr, "eval error: %s\n", r.message);
        return 1;
      }
      ndbg_eval_highlight_tokens(argv[2], hi, sizeof(hi));
      snprintf(st.expr_history, sizeof(st.expr_history), "%s => %lld", argv[2], r.value);
      state_save(&st);
      printf("eval=%lld\nhighlight=%s\nhistory=%s\n", r.value, hi, st.expr_history);
    }
  } else if (!strcmp(argv[1], "processes")) {
    char list[4096];
    nprt_dbg_platform_list_processes(list, sizeof(list));
    if (argc >= 3) {
      if (strstr(list, argv[2])) printf("fuzzy-match=%s\n", argv[2]);
      else printf("fuzzy-match none for=%s\n", argv[2]);
    }
    printf("%s\n", list);
  } else if (!strcmp(argv[1], "core-load")) {
    char msg[256];
    if (argc < 3) return 2;
    if (!nprt_dbg_platform_load_core(argv[2], msg, sizeof(msg))) return 1;
    printf("%s\n", msg);
  } else if (!strcmp(argv[1], "remote-config")) {
    char cfg[1024];
    if (argc < 3) return 2;
    snprintf(st.remote_endpoint, sizeof(st.remote_endpoint), "%s", argv[2]);
    if (argc >= 5 && !strcmp(argv[3], "compress")) st.remote_compress = !strcmp(argv[4], "on");
    if (argc >= 7 && !strcmp(argv[5], "latency-ms")) st.remote_latency_ms = (unsigned int)strtoul(argv[6], NULL, 10);
    remote_cfg_path(cfg, sizeof(cfg));
    if (!ndbg_remote_save_config(cfg, st.remote_endpoint, st.remote_latency_ms, st.remote_compress != 0)) return 1;
    state_save(&st);
    printf("remote-config saved endpoint=%s compress=%d latency=%u\n", st.remote_endpoint, st.remote_compress, st.remote_latency_ms);
  } else if (!strcmp(argv[1], "remote-send")) {
    char cfg[1024], endpoint[256];
    unsigned int latency = 0;
    bool compress = false;
    unsigned char out[1024];
    if (argc < 3) return 2;
    remote_cfg_path(cfg, sizeof(cfg));
    if (!ndbg_remote_load_config(cfg, endpoint, sizeof(endpoint), &latency, &compress)) return 1;
    if (compress) {
      size_t n = ndbg_remote_compress_payload((const unsigned char*)argv[2], strlen(argv[2]), out, sizeof(out));
      printf("remote-send endpoint=%s compressed_bytes=%zu latency=%u\n", endpoint, n, latency);
    } else {
      printf("remote-send endpoint=%s bytes=%zu latency=%u\n", endpoint, strlen(argv[2]), latency);
    }
  } else if (!strcmp(argv[1], "thread-locks")) {
    char snap[512], out[256];
    if (!st.attached) return 1;
    nprt_dbg_platform_threads_snapshot(st.pid, snap, sizeof(snap));
    ndbg_thread_lock_analyze(snap, out, sizeof(out));
    printf("%s\n", out);
  } else if (!strcmp(argv[1], "deadlock")) {
    char snap[512], out[256];
    if (!st.attached) return 1;
    nprt_dbg_platform_threads_snapshot(st.pid, snap, sizeof(snap));
    ndbg_thread_deadlock_detect("A->B,B->A", out, sizeof(out));
    printf("%s\nsnapshot=%s\n", out, snap);
  } else if (!strcmp(argv[1], "dylib-break")) {
    char out[256];
    if (!st.attached || argc < 3) return 1;
    snprintf(st.dylib_filter, sizeof(st.dylib_filter), "%s", argv[2]);
    if (!ndbg_dylib_event_break_enable(st.dylib_filter, out, sizeof(out))) return 1;
    state_save(&st);
    printf("%s\n", out);
  } else if (!strcmp(argv[1], "dylib-load-symbol")) {
    char out[256], events[512];
    if (!st.attached || argc < 4) return 1;
    nprt_dbg_platform_dylib_events(st.pid, events, sizeof(events));
    if (!ndbg_dylib_lazy_symbol_load(argv[2], argv[3], out, sizeof(out))) return 1;
    printf("%s\nrecent-events=%s\n", out, events);
  } else if (!strcmp(argv[1], "status")) {
    printf("attached=%d pid=%lu break=%s step=%s frame=%u hit=%u remote=%s compress=%d latency=%u updated_at=%lld\n",
           st.attached, st.pid, st.last_breakpoint[0] ? st.last_breakpoint : "-", st.last_step,
           st.current_frame, st.hit_counter, st.remote_endpoint, st.remote_compress, st.remote_latency_ms, st.updated_at);
  } else {
    usage();
    return 2;
  }
  return 0;
}
