param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_release_nprt_studio_report.txt"
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

Set-Content -Path $report -Encoding UTF8 -Value "verify_release_nprt_studio"

$releaseDoc = Join-Path $root "docs/releases/nprt-studio-v1.0.0-release-standard.md"
$scaffoldVerify = Join-Path $root "scripts/verify_nprt_studio_scaffold.ps1"
$buildScript = Join-Path $root "scripts/build_nprt_studio.ps1"
$packagingDoc = Join-Path $root "packaging/nprt-studio/README.md"
$ciWorkflow = Join-Path $root ".github/workflows/nullprt-ci.yml"

Check "nprt-studio-release-assets" {
  foreach ($p in @($releaseDoc, $scaffoldVerify, $buildScript, $packagingDoc, $ciWorkflow)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "nprt-studio-release-doc-markers" {
  $txt = Get-Content -Raw -Path $releaseDoc
  foreach ($k in @(
    "LSP path",
    "Debug path",
    "Build path",
    "Completion interaction",
    "verify_nprt_studio_scaffold.ps1",
    "verify_release_nprt_studio.ps1",
    "scripts/build_nprt_studio.ps1"
  )) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing marker: $k" }
  }
}

Check "nprt-studio-ci-wireup" {
  $txt = Get-Content -Raw -Path $ciWorkflow
  foreach ($k in @("verify_nprt_studio_scaffold.ps1", "nprtstudiofull", "verify_release_nprt_studio_report.txt")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing ci marker: $k" }
  }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-release-nprt-studio] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-release-nprt-studio] FAIL - report at $report"
exit 1
