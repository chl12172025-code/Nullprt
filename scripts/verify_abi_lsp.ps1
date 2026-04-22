param(
  [string]$WorkspaceRoot = "."
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/abi_lsp_verify_report.txt"

$abiSpec = Join-Path $root "tests/abi/abi_tests_spec.md"
$lspReplay = Join-Path $root "scripts/lsp_replay_requests.jsonl"

$abiOk = Test-Path $abiSpec
$lspOk = Test-Path $lspReplay

@"
abi_spec_present: $abiOk
lsp_replay_present: $lspOk
note:
- ABI replay executable driver integration is pending compiler+FFI integration.
- LSP replay transport harness is pending nprt-lsp executable build.
"@ | Set-Content -Encoding UTF8 $report

Write-Host "[verify-abi-lsp] report written: $report"
