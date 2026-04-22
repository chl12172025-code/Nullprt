param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/selfhost_readiness_report.txt"

$files = @(
  "src/self/aegc1/frontend/lexer.c",
  "src/self/aegc1/frontend/parser.c",
  "src/self/aegc1/semantic/typecheck.c",
  "src/self/aegc1/semantic/borrowcheck.c",
  "src/self/aegc1/semantic/monomorphize.c",
  "src/self/aegc1/ir/nprt_ir.c",
  "src/self/aegc1/backend/c_emitter.c",
  "src/self/aegc1/backend/native/x86_64/lowering.c",
  "src/self/aegc1/backend/native/x86_64/obj_writer.c",
  "src/self/aegc1/driver/main.c",
  "tests/aegc1/run_tests.ps1"
)

$missing = @()
foreach ($f in $files) {
  $p = Join-Path $root $f
  if (-not (Test-Path $p)) { $missing += $f }
}

$status = if ($missing.Count -eq 0) { "ready-for-toolchain-validation" } else { "incomplete" }

@"
status: $status
missing_count: $($missing.Count)
missing_files:
$($missing -join "`n")
next_steps:
- Run scripts/detect_toolchain.ps1
- Build and run scripts/aegc1_selfhost_verify.ps1
- Run tests/aegc1/run_tests.ps1
"@ | Set-Content -Encoding UTF8 $report

Write-Host "[selfhost-readiness] report written: $report"
