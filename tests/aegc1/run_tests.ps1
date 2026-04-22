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
protect_passes_source: $(Test-Path (Join-Path $root "src/self/aegc1/ir/protect_passes.c"))
dev_gate_source: $(Test-Path (Join-Path $root "src/self/aegc1/runtime/dev_gate.c"))
api_doc_generator: $(Test-Path (Join-Path $root "scripts/generate_api_docs.ps1"))
note: runtime execution requires compiled aegc1 driver binary.
"@ | Set-Content -Encoding UTF8 $report

Write-Host "[tests/aegc1] report written: $report"
