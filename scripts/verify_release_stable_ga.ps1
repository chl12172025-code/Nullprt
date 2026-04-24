param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_release_stable_ga_report.txt"
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

Set-Content -Path $report -Value "verify_release_stable_ga"

$workflow = Join-Path $root ".github/workflows/release-alpha.yml"
$qaDoc = Join-Path $root "docs/releases/v1.0.0-stability-and-qa.md"
$qaVerify = Join-Path $root "scripts/verify_release_qa_full_completion.ps1"

Check "stable-assets" {
  foreach ($p in @($workflow, $qaDoc, $qaVerify)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "stable-workflow-trigger" {
  $txt = Get-Content -Raw -Path $workflow
  if ($txt -notmatch [regex]::Escape("name: release-publish")) { throw "missing release-publish name" }
  if ($txt -notmatch [regex]::Escape('- "v*"')) { throw "missing stable tag trigger" }
}

Check "stable-prerelease-conditional" {
  $txt = Get-Content -Raw -Path $workflow
  if ($txt -notmatch [regex]::Escape("steps.notes.outputs.prerelease")) { throw "missing conditional prerelease output" }
  if ($txt -notmatch [regex]::Escape('prerelease: ${{')) { throw "missing prerelease expression" }
}

Check "stable-qa-markers" {
  $txt = Get-Content -Raw -Path $qaDoc
  foreach ($k in @("release_checklist.json", "RC-14", "qafull")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing marker: $k" }
  }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-release-stable-ga] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-release-stable-ga] FAIL - report at $report"
exit 1
