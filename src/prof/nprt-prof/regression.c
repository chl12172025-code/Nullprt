#include <math.h>
#include <stdbool.h>

double prof_noise_filter(double v, double baseline) {
  double d = fabs(v - baseline);
  if (d < baseline * 0.03) return baseline;
  return v;
}

double prof_t_stat(const double* runs, int n, double baseline) {
  int i;
  double mean = 0.0, var = 0.0;
  if (!runs || n <= 1) return 0.0;
  for (i = 0; i < n; i++) mean += runs[i];
  mean /= (double)n;
  for (i = 0; i < n; i++) { double d = runs[i] - mean; var += d * d; }
  var /= (double)(n - 1);
  if (var <= 0.0) return 0.0;
  return (mean - baseline) / sqrt(var / (double)n);
}

bool prof_regression_detect(const double* runs, int n, double baseline, double* out_t) {
  double t = prof_t_stat(runs, n, baseline);
  if (out_t) *out_t = t;
  return t > 2.0;
}
