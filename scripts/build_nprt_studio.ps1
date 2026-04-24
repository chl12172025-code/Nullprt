param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$studioRoot = Join-Path $root "apps/nprt-studio"
$buildDir = Join-Path $root "build/nprt-studio"
$report = Join-Path $root "scripts/build_nprt_studio_report.txt"

Set-Content -Path $report -Encoding UTF8 -Value "build_nprt_studio"

if (-not (Test-Path $studioRoot)) {
  Add-Content -Path $report -Value "result: fail"
  Add-Content -Path $report -Value "reason: missing apps/nprt-studio"
  exit 1
}

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
  $fallback = "C:\Program Files\CMake\bin\cmake.exe"
  if (Test-Path $fallback) {
    $cmake = @{ Source = $fallback }
  }
}
if (-not $cmake) {
  Add-Content -Path $report -Value "result: blocked"
  Add-Content -Path $report -Value "reason: cmake not found in PATH"
  Write-Host "[build-nprt-studio] BLOCKED - cmake not found"
  exit 2
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$qt6Dir = $env:Qt6_DIR
if (-not $qt6Dir -or -not (Test-Path $qt6Dir)) {
  $qtRoots = @("C:\Qt", "D:\Qt")
  foreach ($rootDir in $qtRoots) {
    if (-not (Test-Path $rootDir)) { continue }
    $configs = Get-ChildItem -Path $rootDir -Filter "Qt6Config.cmake" -Recurse -ErrorAction SilentlyContinue |
      Where-Object { $_.FullName -match "msvc2022_64\\lib\\cmake\\Qt6\\Qt6Config.cmake$" } |
      Sort-Object FullName -Descending
    if ($configs -and $configs.Count -gt 0) {
      $qt6Dir = Split-Path -Parent $configs[0].FullName
      break
    }
  }
}
if (-not $qt6Dir -or -not (Test-Path $qt6Dir)) {
  Add-Content -Path $report -Value "result: blocked"
  Add-Content -Path $report -Value "reason: Qt6 SDK not found (set Qt6_DIR or install Qt6)"
  Write-Host "[build-nprt-studio] BLOCKED - Qt6 SDK not found"
  exit 2
}

Push-Location $buildDir
try {
  & $cmake.Source -S $studioRoot -B $buildDir -DQt6_DIR="$qt6Dir"
  if ($LASTEXITCODE -ne 0) {
    Add-Content -Path $report -Value "result: fail"
    Add-Content -Path $report -Value "stage: configure"
    Add-Content -Path $report -Value "qt6_dir: $qt6Dir"
    exit 1
  }
  & $cmake.Source --build $buildDir --config Release
  if ($LASTEXITCODE -ne 0) {
    Add-Content -Path $report -Value "result: fail"
    Add-Content -Path $report -Value "stage: build"
    exit 1
  }
} finally {
  Pop-Location
}

Add-Content -Path $report -Value "result: pass"
Add-Content -Path $report -Value "build_dir: $buildDir"
Write-Host "[build-nprt-studio] PASS"
exit 0
