param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_debugger_full_completion_report.txt"
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

Set-Content -Path $report -Value "verify_debugger_full_completion"

$main = Join-Path $root "src/debug/nprt-debug/main.c"
$linux = Join-Path $root "src/debug/nprt-debug/platform_linux.c"
$win = Join-Path $root "src/debug/nprt-debug/platform_win.c"
$mac = Join-Path $root "src/debug/nprt-debug/platform_macos.c"
$expr = Join-Path $root "src/debug/nprt-debug/expr_eval.c"
$exprHl = Join-Path $root "src/debug/nprt-debug/expr_highlight.c"
$remote = Join-Path $root "src/debug/nprt-debug/remote_transport.c"
$thread = Join-Path $root "src/debug/nprt-debug/thread_analyze.c"
$dylib = Join-Path $root "src/debug/nprt-debug/dylib_debug.c"
$case = Join-Path $root "tests/debug/cases/debugger_full_sequence.txt"
$expected = Join-Path $root "tests/debug/expected/debugger_full_expected.txt"

Check "debugger-core-files" {
  foreach ($p in @($main,$linux,$win,$mac,$expr,$exprHl,$remote,$thread,$dylib)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "debugger-fixtures" {
  if (-not (Test-Path $case)) { throw "missing debug case sequence" }
  if (-not (Test-Path $expected)) { throw "missing debug expected checklist" }
}

Check "debugger-command-surface" {
  $src = Get-Content $main -Raw
  foreach ($k in @("hbreak-data","hbreak-exec","mem-edit","remote-config","thread-locks","dylib-load-symbol","eval")) {
    if ($src -notmatch [regex]::Escape($k)) { throw "missing command $k" }
  }
}

Check "debugger-feature-hooks" {
  $src = Get-Content $main -Raw
  if ($src -notmatch "ndbg_eval_expr") { throw "missing expr eval integration" }
  if ($src -notmatch "ndbg_remote_compress_payload") { throw "missing remote compression integration" }
  if ($src -notmatch "ndbg_thread_deadlock_detect") { throw "missing deadlock integration" }
  if ($src -notmatch "ndbg_dylib_lazy_symbol_load") { throw "missing dylib lazy-load integration" }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-debugger-full] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-debugger-full] FAIL - report at $report"
exit 1
