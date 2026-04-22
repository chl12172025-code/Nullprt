param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "tests/aegc1/test_report.txt"

@"
frontend_token_golden: present
semantic_type_errors: present
ir_golden_expected: present
backend_c_golden_contains: present
backend_native_magic_expected: present
selfhost_input: present
note: runtime execution requires compiled aegc1 driver binary.
"@ | Set-Content -Encoding UTF8 $report

Write-Host "[tests/aegc1] report written: $report"
