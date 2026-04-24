param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_lsp_full_completion_report.txt"
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

Set-Content -Path $report -Value "verify_lsp_full_completion"

$main = Join-Path $root "src/lsp/nprt-lsp/main.c"
$indexH = Join-Path $root "src/lsp/nprt-lsp/index.h"
$indexC = Join-Path $root "src/lsp/nprt-lsp/index.c"
$caseFile = Join-Path $root "tests/lsp/cases/lsp_full_requests.jsonl"
$expectedFile = Join-Path $root "tests/lsp/expected/lsp_full_expected.txt"

Check "lsp-index-files" {
  if (-not (Test-Path $indexH)) { throw "missing index.h" }
  if (-not (Test-Path $indexC)) { throw "missing index.c" }
}

Check "lsp-fixtures" {
  if (-not (Test-Path $caseFile)) { throw "missing replay requests" }
  if (-not (Test-Path $expectedFile)) { throw "missing expected checklist" }
}

Check "lsp-capabilities-routing" {
  $src = Get-Content $main -Raw
  if ($src -notmatch "textDocument/diagnostic") { throw "missing pull diagnostic handler" }
  if ($src -notmatch "semanticTokens/range") { throw "missing semantic range handler" }
  if ($src -notmatch "workspace/executeCommand") { throw "missing executeCommand handler" }
  if ($src -notmatch "nprt/refactorPreview") { throw "missing refactor preview handler" }
}

Check "lsp-completion-enhancements" {
  $src = Get-Content $main -Raw
  if ($src -notmatch "insertTextFormat") { throw "missing snippet completion" }
  if ($src -notmatch "sortText") { throw "missing completion ranking key" }
  if ($src -notmatch "\\[fn\\]") { throw "missing symbol icon detail" }
}

Check "lsp-cross-file-features" {
  $src = Get-Content $main -Raw
  if ($src -notmatch "nls_find_definition") { throw "missing cross-file definition index usage" }
  if ($src -notmatch "nls_find_references") { throw "missing workspace references usage" }
  if ($src -notmatch "documentChanges") { throw "missing multi-document rename edits" }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-lsp-full] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-lsp-full] FAIL - report at $report"
exit 1
