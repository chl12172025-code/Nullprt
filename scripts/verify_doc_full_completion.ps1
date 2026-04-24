param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_doc_full_completion_report.txt"
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

Set-Content -Path $report -Value "verify_doc_full_completion"

$main = Join-Path $root "src/doc/nprt-doc/main.c"
$case = Join-Path $root "tests/doc/cases/doc_full_sequence.txt"
$expected = Join-Path $root "tests/doc/expected/doc_full_expected.txt"

Check "doc-fixtures" {
  if (-not (Test-Path $case)) { throw "missing doc case file" }
  if (-not (Test-Path $expected)) { throw "missing doc expected file" }
}

Check "doc-source-markers" {
  $src = Get-Content $main -Raw
  foreach ($k in @("schema_version","search_index.incremental.json","hoverPreview","moveGroup","dep_depth_limit","recursive","generic_expand")) {
    if ($src -notmatch [regex]::Escape($k)) { throw "missing marker: $k" }
  }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-doc-full] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-doc-full] FAIL - report at $report"
exit 1
