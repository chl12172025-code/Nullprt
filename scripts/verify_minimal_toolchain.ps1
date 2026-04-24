param(
  [string]$WorkspaceRoot = "."
)

$ErrorActionPreference = "Stop"

function Step($msg) {
  Write-Host "[verify-minimal] $msg"
}

$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_minimal_toolchain_report.txt"
$ok = $true

function Invoke-Check {
  param(
    [string]$Name,
    [scriptblock]$Action
  )
  try {
    & $Action
    Add-Content -Path $report -Value "${Name}: ok"
  } catch {
    $script:ok = $false
    Add-Content -Path $report -Value "${Name}: failed ($($_.Exception.Message))"
  }
}

Set-Content -Path $report -Value "verify_minimal_toolchain"

Invoke-Check "check-doc-assets" {
  $required = @(
    "docs/icons/nullprt-icon-theme.json",
    "spec/extensions/extension-catalog.md",
    "src/self/aegc1/runtime/extension_registry.c"
  )
  foreach ($r in $required) {
    $p = Join-Path $root $r
    if (-not (Test-Path $p)) { throw "missing $r" }
  }
}

Invoke-Check "check-readme-links" {
  $readme = Get-Content (Join-Path $root "README.md") -Raw
  if ($readme -notmatch "docs/extensions/icon-integration.md") { throw "missing icon integration link" }
  if ($readme -notmatch "nprt-doc") { throw "missing nprt-doc item" }
}

Invoke-Check "check-nprt-doc-source" {
  $docMain = Get-Content (Join-Path $root "src/doc/nprt-doc/main.c") -Raw
  if ($docMain -notmatch "index.json") { throw "missing json output support" }
  if ($docMain -notmatch "symbols.txt") { throw "missing symbols list output" }
}

Invoke-Check "check-nprt-prof-source" {
  $profMain = Get-Content (Join-Path $root "src/prof/nprt-prof/main.c") -Raw
  if ($profMain -notmatch "trace line format") { throw "missing trace format usage" }
  if ($profMain -notmatch "total_us") { throw "missing aggregated metric output" }
}

Invoke-Check "check-nprt-fmt-source" {
  $fmtMain = Get-Content (Join-Path $root "src/fmt/nprt-fmt/main.c") -Raw
  if ($fmtMain -notmatch "--diff-all") { throw "missing diff-all mode" }
}

Invoke-Check "check-nprt-debug-source" {
  $dbgMain = Get-Content (Join-Path $root "src/debug/nprt-debug/main.c") -Raw
  if ($dbgMain -notmatch "debug_session.state") { throw "missing debug state persistence" }
  if ($dbgMain -notmatch "status") { throw "missing status command" }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Step "PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Step "FAIL - report at $report"
exit 1
