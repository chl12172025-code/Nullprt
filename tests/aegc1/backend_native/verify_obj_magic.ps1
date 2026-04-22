param([string]$ObjPath, [string]$Target)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $ObjPath)) {
  Write-Error "object file not found: $ObjPath"
  exit 1
}

$bytes = [System.IO.File]::ReadAllBytes((Resolve-Path $ObjPath))
if ($bytes.Length -lt 4) {
  Write-Error "object file too small"
  exit 1
}

$hex = ($bytes[0..3] | ForEach-Object { $_.ToString("x2") }) -join ""

switch ($Target) {
  "win" {
    # our minimal writer emits ASCII 'COFF'
    $ok = ($hex -eq "434f4646")
  }
  "linux" {
    $ok = ($hex -eq "7f454c46")
  }
  "macos" {
    # feedfacf (little-endian bytes: cffaedfe)
    $ok = ($hex -eq "cffaedfe" -or $hex -eq "feedfacf")
  }
  default {
    Write-Error "unknown target: $Target"
    exit 1
  }
}

if (-not $ok) {
  Write-Error "unexpected magic: $hex target=$Target"
  exit 1
}

Write-Host "ok magic=$hex target=$Target"
