param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_docs_site_full_completion_report.txt"
$ok = $true

function Check([string]$Name, [scriptblock]$Action) {
  try { & $Action; Add-Content -Path $report -Value "${Name}: ok" }
  catch { $script:ok = $false; Add-Content -Path $report -Value "${Name}: failed ($($_.Exception.Message))" }
}

Set-Content -Path $report -Value "verify_docs_site_full_completion"

$index = Join-Path $root "docs/index.html"
$api = Join-Path $root "docs/api/index.html"
$app = Join-Path $root "docs/site/app.js"
$css = Join-Path $root "docs/site/styles.css"
$release = Join-Path $root "docs/releases/v0.2.0-beta.md"
$case = Join-Path $root "tests/docs_site/cases/docs_site_full_sequence.txt"
$expected = Join-Path $root "tests/docs_site/expected/docs_site_full_expected.txt"

Check "docs-site-files" {
  foreach ($p in @($index,$api,$app,$css,$release,$case,$expected)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "docs-site-feature-markers" {
  $idx = Get-Content $index -Raw
  $js = Get-Content $app -Raw
  $st = Get-Content $css -Raw
  $rel = Get-Content $release -Raw
  foreach ($k in @("gh-stars","contributors-wall","editor","runtime-output","gradeExercise","demo-frame","breadcrumbs")) {
    if ($idx -notmatch [regex]::Escape($k)) { throw "missing index marker: $k" }
  }
  foreach ($k in @("loadGithubMeta","localStorage","keydown","toggleMobileMenu","toggleTheme","copyCode","filterNav")) {
    if ($js -notmatch [regex]::Escape($k)) { throw "missing app marker: $k" }
  }
  foreach ($k in @("transition","@media (max-width: 960px)","@media (min-width: 961px)")) {
    if ($st -notmatch [regex]::Escape($k)) { throw "missing style marker: $k" }
  }
  if ($rel -notmatch "compare") { throw "missing release compare link" }
  if ($rel -notmatch "SECURITY ADVISORY") { throw "missing security advisory highlight" }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-docs-site-full] PASS - report at $report"
  exit 0
}
Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-docs-site-full] FAIL - report at $report"
exit 1
