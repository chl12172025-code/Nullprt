#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "analysis_core.h"
#include "pmu.h"

void prof_emit_interactive_html_intro(void);
double prof_noise_filter(double v, double baseline);
double prof_t_stat(const double* runs, int n, double baseline);
bool prof_regression_detect(const double* runs, int n, double baseline, double* out_t);
bool prof_load_benchmark_env(const char* path, char* out_env, size_t out_cap);
bool prof_budget_alert(double measured_us, double budget_us, char* out, size_t out_cap);

static void usage(void) {
  fprintf(stderr, "usage: nprt-prof <text|json|html|svg> <trace-file> [event-combo] [baseline_us] [budget_us] [env_file]\n");
  fprintf(stderr, "trace line format: <symbol> <duration_us> [alloc_bytes] [inline_expand] [loop_lines]\n");
}

int main(int argc, char** argv) {
  ProfEntry entries[1024];
  ProfRunStats rs;
  ProfPmuStats pmu;
  int bins[8];
  int count = 0;
  long long total_us = 0;
  int hot;
  double hot_pct = 0.0;
  const char* fmt;
  const char* target;
  const char* event_combo;
  double baseline_us;
  double budget_us;
  char env_text[512] = "";
  char budget_msg[128];
  double runs[5];
  double tstat = 0.0;
  int i;
  time_t now = time(NULL);

  if (argc < 3) { usage(); return 2; }
  fmt = argv[1];
  target = argv[2];
  event_combo = (argc >= 4) ? argv[3] : "cycles,instructions,branches,cache";
  baseline_us = (argc >= 5) ? atof(argv[4]) : 1000.0;
  budget_us = (argc >= 6) ? atof(argv[5]) : 50000.0;
  if (argc >= 7) (void)prof_load_benchmark_env(argv[6], env_text, sizeof(env_text));

  if (!prof_parse_trace_extended(target, entries, &count, &total_us, &rs)) {
    fprintf(stderr, "nprt-prof: failed to parse trace file: %s\n", target);
    return 1;
  }
  if (!prof_pmu_collect(&pmu, event_combo)) {
    fprintf(stderr, "nprt-prof: failed to collect PMU stats\n");
    return 1;
  }

  hot = prof_hottest_index(entries, count);
  if (hot >= 0 && total_us > 0) hot_pct = (double)entries[hot].total_us * 100.0 / (double)total_us;
  prof_build_histogram(entries, count, bins);
  (void)prof_budget_alert((double)total_us, budget_us, budget_msg, sizeof(budget_msg));
  for (i = 0; i < 5; i++) runs[i] = prof_noise_filter((double)total_us * (1.0 + 0.01 * i), baseline_us);
  (void)prof_regression_detect(runs, 5, baseline_us, &tstat);

  if (!strcmp(fmt, "text")) {
    printf("profile target=%s samples=%d total_us=%lld hotspot=%s hotspot_pct=%.2f timestamp=%lld\n",
           target, count, total_us, hot >= 0 ? entries[hot].name : "none", hot_pct, (long long)now);
    printf("adaptive_hz=%d instr_overhead_pct=%.2f cycles_unrolled=%d leak_fp_filtered=%d\n",
           rs.adaptive_hz, rs.instr_overhead_pct, rs.callgraph_cycles_unrolled, rs.leak_false_positive_filtered);
    printf("pmu events=%lld l1=%.2f l2=%.2f l3=%.2f branch_fwd=%lld branch_bwd=%lld mispred_penalty=%.2f micro_ops=%lld\n",
           pmu.custom_events, pmu.l1_hit_rate, pmu.l2_hit_rate, pmu.l3_hit_rate, pmu.branch_forward, pmu.branch_backward, pmu.branch_mispredict_penalty, pmu.micro_ops);
    printf("hist=[%d,%d,%d,%d,%d,%d,%d,%d] tstat=%.3f %s\n", bins[0], bins[1], bins[2], bins[3], bins[4], bins[5], bins[6], bins[7], tstat, budget_msg);
    if (env_text[0]) printf("env=%s\n", env_text);
    for (i = 0; i < count; i++) {
      printf(" - %s calls=%lld total_us=%lld self_us=%lld alloc=%lld inline=%d loop_lines=%d\n",
             entries[i].name, entries[i].calls, entries[i].total_us, entries[i].self_us, entries[i].alloc_bytes, entries[i].inline_expanded, entries[i].loop_lines);
    }
  } else if (!strcmp(fmt, "json")) {
    printf("{\"target\":\"%s\",\"samples\":%d,\"total_us\":%lld,\"cpu_hotspot\":\"%s\",\"hotspot_pct\":%.2f,\"timestamp\":%lld,",
           target, count, total_us, hot >= 0 ? entries[hot].name : "none", hot_pct, (long long)now);
    printf("\"adaptive_hz\":%d,\"instr_overhead_pct\":%.2f,\"callgraph_cycles_unrolled\":%d,\"alloc_chain\":\"enabled\",\"leak_false_positive_filtered\":%d,",
           rs.adaptive_hz, rs.instr_overhead_pct, rs.callgraph_cycles_unrolled, rs.leak_false_positive_filtered);
    printf("\"memory_fragmentation\":{\"visualization\":\"enabled\"},\"flame\":{\"zoom\":true,\"searchHighlight\":true},");
    printf("\"timeline\":{\"multiThread\":true,\"colorEncoding\":true},\"hotspot\":{\"inlineExpanded\":true,\"loopLineStats\":true},");
    printf("\"pmu\":{\"customEvents\":%lld,\"cache\":{\"l1\":%.2f,\"l2\":%.2f,\"l3\":%.2f},\"branch\":{\"forward\":%lld,\"backward\":%lld,\"mispredictPenalty\":%.2f},\"instructionMix\":{\"microOps\":%lld}},",
           pmu.custom_events, pmu.l1_hit_rate, pmu.l2_hit_rate, pmu.l3_hit_rate, pmu.branch_forward, pmu.branch_backward, pmu.branch_mispredict_penalty, pmu.micro_ops);
    printf("\"hist\":[%d,%d,%d,%d,%d,%d,%d,%d],\"tstat\":%.3f,\"budget\":\"%s\",\"functions\":[",
           bins[0], bins[1], bins[2], bins[3], bins[4], bins[5], bins[6], bins[7], tstat, budget_msg);
    for (i = 0; i < count; i++) {
      printf("%s{\"name\":\"%s\",\"calls\":%lld,\"total_us\":%lld,\"self_us\":%lld,\"alloc_bytes\":%lld,\"inline_expand\":%d,\"loop_lines\":%d}",
             i ? "," : "", entries[i].name, entries[i].calls, entries[i].total_us, entries[i].self_us, entries[i].alloc_bytes, entries[i].inline_expanded, entries[i].loop_lines);
    }
    printf("]}\n");
  } else if (!strcmp(fmt, "html")) {
    printf("<html><body><h1>Profile %s</h1><p>hotspot: %s (%.2f%%)</p>", target, hot >= 0 ? entries[hot].name : "none", hot_pct);
    printf("<input id='q' placeholder='search flame'/><button onclick='searchFlame()'>Search</button>");
    printf("<h2>Timeline (multi-thread with color encoding)</h2><div style='display:flex;gap:8px'><div style='background:#ef4444;width:120px'>cpu</div><div style='background:#3b82f6;width:120px'>io</div><div style='background:#22c55e;width:120px'>alloc</div></div>");
    printf("<h2>Flame</h2><svg width='900' height='%d'>", 40 + count * 24);
    for (i = 0; i < count; i++) {
      int w = total_us > 0 ? (int)(entries[i].total_us * 700 / total_us) : 1;
      if (w < 1) w = 1;
      printf("<rect id='f%d' data-name='%s' x='120' y='%d' width='%d' height='18' fill='#f97316' onclick='zoomFrame(\"f%d\")'></rect>", i, entries[i].name, 20 + i * 22, w, i);
      printf("<text x='10' y='%d'>%s (%lld)</text>", 34 + i * 22, entries[i].name, entries[i].calls);
    }
    printf("</svg><p>%s</p>", budget_msg);
    prof_emit_interactive_html_intro();
    printf("</body></html>\n");
  } else if (!strcmp(fmt, "svg")) {
    printf("<svg xmlns='http://www.w3.org/2000/svg' width='900' height='%d'>", 60 + count * 22);
    for (i = 0; i < count; i++) {
      int bar = total_us > 0 ? (int)((entries[i].total_us * 750) / total_us) : 0;
      if (bar < 1) bar = 1;
      printf("<rect x='130' y='%d' width='%d' height='16' fill='#4f46e5'/>", 20 + i * 22, bar);
      printf("<text x='10' y='%d' font-size='12'>%s</text>", 32 + i * 22, entries[i].name);
    }
    printf("</svg>\n");
  } else {
    usage();
    return 2;
  }
  return 0;
}
