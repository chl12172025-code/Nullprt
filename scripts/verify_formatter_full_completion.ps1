param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_formatter_full_completion_report.txt"
$ok = $true

function Check([string]$Name, [scriptblock]$Action) {
  try { & $Action; Add-Content -Path $report -Value "${Name}: ok" }
  catch { $script:ok = $false; Add-Content -Path $report -Value "${Name}: failed ($($_.Exception.Message))" }
}

Set-Content -Path $report -Value "verify_formatter_full_completion"

$main = Join-Path $root "src/fmt/nprt-fmt/main.c"
$core = Join-Path $root "src/fmt/nprt-fmt/format_core.c"
$align = Join-Path $root "src/fmt/nprt-fmt/align_rules.c"
$sort = Join-Path $root "src/fmt/nprt-fmt/sort_rules.c"
$macro = Join-Path $root "src/fmt/nprt-fmt/macro_attr_rules.c"
$case = Join-Path $root "tests/fmt/cases/fmt_full_sequence.txt"
$expected = Join-Path $root "tests/fmt/expected/fmt_full_expected.txt"

Check "fmt-files" {
  foreach ($p in @($main,$core,$align,$sort,$macro,$case,$expected)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "fmt-markers" {
  $src = Get-Content $main -Raw
  foreach ($k in @("fmt_force_control_flow_braces","fmt_wrap_long_lines","fmt_chain_per_line","fmt_align_columns","fmt_wrap_macro_args")) {
    if ($src -notmatch [regex]::Escape($k)) { throw "missing marker: $k" }
  }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-fmt-full] PASS - report at $report"
  exit 0
}
Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-fmt-full] FAIL - report at $report"
exit 1
