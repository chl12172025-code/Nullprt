param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_profiler_full_completion_report.txt"
$ok = $true

function Check([string]$Name, [scriptblock]$Action) {
  try {
    & $Action
    Add-Content -Path $report -Value "${Name}: ok"
  } catch {
    $script:ok = $false
    Add-Content -Path $report -Value "${Name}: failed ($($_.Exception.Message))"
  }
}

Set-Content -Path $report -Value "verify_profiler_full_completion"

$main = Join-Path $root "src/prof/nprt-prof/main.c"
$coreH = Join-Path $root "src/prof/nprt-prof/analysis_core.h"
$coreC = Join-Path $root "src/prof/nprt-prof/analysis_core.c"
$pmuH = Join-Path $root "src/prof/nprt-prof/pmu.h"
$pmuL = Join-Path $root "src/prof/nprt-prof/pmu_linux.c"
$pmuW = Join-Path $root "src/prof/nprt-prof/pmu_win.c"
$pmuM = Join-Path $root "src/prof/nprt-prof/pmu_macos.c"
$view = Join-Path $root "src/prof/nprt-prof/view_html.c"
$reg = Join-Path $root "src/prof/nprt-prof/regression.c"
$benv = Join-Path $root "src/prof/nprt-prof/benchmark_env.c"
$case = Join-Path $root "tests/prof/cases/prof_full_sequence.txt"
$expected = Join-Path $root "tests/prof/expected/prof_full_expected.txt"

Check "prof-core-files" {
  foreach ($p in @($main,$coreH,$coreC,$pmuH,$pmuL,$pmuW,$pmuM,$view,$reg,$benv)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "prof-fixtures" {
  if (-not (Test-Path $case)) { throw "missing profiler case file" }
  if (-not (Test-Path $expected)) { throw "missing profiler expected file" }
}

Check "prof-feature-wiring" {
  $src = Get-Content $main -Raw
  foreach ($k in @("adaptive_hz","instr_overhead_pct","flame","timeline","inlineExpanded","cache","l1","mispredictPenalty","hist","budget")) {
    if ($src -notmatch [regex]::Escape($k)) { throw "missing feature marker: $k" }
  }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-profiler-full] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-profiler-full] FAIL - report at $report"
exit 1
