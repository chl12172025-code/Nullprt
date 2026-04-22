param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$outDir = Join-Path $root "site/api"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$headers = @(
  "src/pal/pal.h",
  "src/pkg/nprt-pkg/resolver.h",
  "src/pkg/nprt-pkg/nprt_pkg.h",
  "src/self/aegc1/runtime/dev_gate.h"
) | ForEach-Object { Join-Path $root $_ }

$items = @()
foreach ($h in $headers) {
  if (-not (Test-Path $h)) { continue }
  $lines = Get-Content $h
  $comment = @()
  foreach ($line in $lines) {
    if ($line.TrimStart().StartsWith("//")) {
      $comment += $line.Trim().Substring(2).Trim()
      continue
    }
    if ($line -match "^[A-Za-z_][A-Za-z0-9_\\s\\*]+\\([A-Za-z0-9_\\s\\*\\,\\.\\[\\]-]*\\);$") {
      $items += [PSCustomObject]@{
        Header = [System.IO.Path]::GetFileName($h)
        Signature = $line.Trim()
        Comment = ($comment -join " ")
      }
      $comment = @()
    } elseif ($line.Trim() -eq "") {
      $comment = @()
    }
  }
}

$html = @()
$html += "<!doctype html><html><head><meta charset='utf-8'><title>Nullprt API Reference</title></head><body>"
$html += "<h1>Nullprt API Reference</h1>"
$html += "<p>Ethical use warning: sensitive APIs are for authorized security research and compatibility testing only.</p>"
function Escape-Html([string]$s) {
  if ($null -eq $s) { return "" }
  return $s.Replace("&","&amp;").Replace("<","&lt;").Replace(">","&gt;")
}

$html += "<ul>"
foreach ($it in $items) {
  $html += "<li><b>$($it.Header)</b><br/><code>$(Escape-Html $it.Signature)</code><br/>$(Escape-Html $it.Comment)</li>"
}
$html += "</ul></body></html>"

$index = Join-Path $outDir "index.html"
[System.IO.File]::WriteAllLines($index, $html)
Write-Host "[api-docs] wrote $index"
