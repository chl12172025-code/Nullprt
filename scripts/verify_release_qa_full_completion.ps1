param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_release_qa_full_completion_report.txt"
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

Set-Content -Path $report -Value "verify_release_qa_full_completion"

$qaDoc = Join-Path $root "docs/releases/v1.0.0-stability-and-qa.md"
$case = Join-Path $root "tests/qa/cases/release_qa_full_sequence.txt"
$expected = Join-Path $root "tests/qa/expected/release_qa_full_expected.txt"
$ci = Join-Path $root ".github/workflows/nullprt-ci.yml"

Check "qa-assets" {
  foreach ($p in @($qaDoc,$case,$expected,$ci)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "qa-document-coverage" {
  $txt = Get-Content $qaDoc -Raw
  foreach ($k in @(
    "release_checklist.json",
    "RC-14",
    "scripts/verify_release_qa_full_completion.ps1",
    "verify_release_qa_full_completion_report.txt",
    "qafull"
  )) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing qa doc marker: $k" }
  }
}

Check "qa-ci-gate-wired" {
  $c = Get-Content $ci -Raw
  if ($c -notmatch "verify_release_qa_full_completion.ps1") { throw "missing qafull script invocation" }
  if ($c -notmatch "qafull") { throw "missing qafull id/outcome wiring" }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-release-qa-full] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-release-qa-full] FAIL - report at $report"
exit 1
