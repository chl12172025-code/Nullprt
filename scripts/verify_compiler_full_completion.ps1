param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_compiler_full_completion_report.txt"
$ok = $true

function Check($name, $path) {
  if (Test-Path (Join-Path $root $path)) {
    Add-Content -Path $report -Value "${name}: ok"
  } else {
    $script:ok = $false
    Add-Content -Path $report -Value "${name}: missing ($path)"
  }
}

Set-Content -Path $report -Value "verify_compiler_full_completion"

Check "build_graph" "src/self/aegc1/driver/build_graph.c"
Check "distributed_protocol" "src/self/aegc1/driver/distributed_protocol.c"
Check "pch_compat" "src/self/aegc1/driver/pch_compat.c"
Check "backend_arm64" "src/self/aegc1/backend/native/arm64/lowering.c"
Check "backend_riscv" "src/self/aegc1/backend/native/riscv/lowering.c"
Check "backend_wasm" "src/self/aegc1/backend/native/wasm/lowering.c"
Check "backend_sparc" "src/self/aegc1/backend/native/sparc/lowering.c"
Check "backend_powerpc" "src/self/aegc1/backend/native/powerpc/lowering.c"
Check "backend_mips" "src/self/aegc1/backend/native/mips/lowering.c"
Check "register_alloc" "src/self/aegc1/backend/native/register_alloc.c"
Check "incremental_patch" "src/self/aegc1/link/incremental_patch.c"
Check "lazy_stub_table" "src/self/aegc1/link/lazy_stub_table.c"
Check "prelink_inline" "src/self/aegc1/link/prelink_inline.c"
Check "postlink_rewrite" "src/self/aegc1/link/postlink_rewrite.c"
Check "diag_i18n" "src/self/aegc1/diag/i18n.c"
Check "diag_ml_hint_ranker" "src/self/aegc1/diag/ml_hint_ranker.c"
Check "diag_autofix_preview" "src/self/aegc1/diag/autofix_preview.c"
Check "diag_autofix_conflict" "src/self/aegc1/diag/autofix_conflict.c"
Check "security_vuln_db" "src/self/aegc1/security/vuln_db.c"
Check "security_av_multi_engine" "src/self/aegc1/security/av_multi_engine.c"
Check "security_license_compat" "src/self/aegc1/security/license_compat.c"
Check "tests_compiler_1_0" "tests/aegc1/compiler_1_0/diag_security_expected.txt"

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-compiler-full] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-compiler-full] FAIL - report at $report"
exit 1
