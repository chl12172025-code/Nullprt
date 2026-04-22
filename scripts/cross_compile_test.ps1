param(
  [string]$WorkspaceRoot = "."
)

$ErrorActionPreference = "Stop"

function Log($m) { Write-Host "[cross-compile-test] $m" }

$root = Resolve-Path $WorkspaceRoot
$targetOs = if ($IsWindows) { "windows" } elseif ($IsMacOS) { "macos" } else { "linux" }
$build = Join-Path $root ("build/" + $targetOs)
$report = Join-Path $root "scripts/cross_compile_report.txt"
$detect = Join-Path $root "scripts/detect_toolchain.ps1"
$bootstrap = Join-Path $root "scripts/bootstrap_verify.ps1"

New-Item -ItemType Directory -Force -Path $build | Out-Null

if (-not (Test-Path $detect)) {
  "result: failed`nreason: detect_toolchain.ps1 missing" | Set-Content -Encoding UTF8 $report
  throw "detect_toolchain.ps1 missing"
}

$toolJson = & powershell -ExecutionPolicy Bypass -File $detect -JsonOnly 2>$null
if ($LASTEXITCODE -ne 0 -or -not $toolJson) {
  @"
result: blocked
reason: no toolchain detected on Windows
windows_compile: false
linux_cmd: cc generated.c -o out_linux
macos_cmd: cc generated.c -o out_macos
"@ | Set-Content -Encoding UTF8 $report
  Log "No compiler toolchain detected. Report: $report"
  exit 1
}

$tool = $toolJson | ConvertFrom-Json
Log "Detected compiler: $($tool.compiler_name) at $($tool.compiler_path)"

& powershell -ExecutionPolicy Bypass -File $bootstrap -WorkspaceRoot $root
$okBootstrap = ($LASTEXITCODE -eq 0)

$generatedC = Join-Path $build "aegc1_from_a0.c"
$winBinary = Join-Path $build "aegc1.exe"
$smoke = $false
if ((Test-Path $generatedC) -and (Test-Path $winBinary)) {
  & $winBinary *> $null
  $smoke = ($LASTEXITCODE -eq 0)
}

@"
result: $(if ($okBootstrap) { "pass" } else { "partial-fail" })
target_os: $targetOs
compiler_name: $($tool.compiler_name)
compiler_path: $($tool.compiler_path)
windows_generated_c: $(Test-Path $generatedC)
windows_binary_built: $(Test-Path $winBinary)
windows_smoke_run: $smoke
linux_cmd: cc generated.c -o out_linux
macos_cmd: cc generated.c -o out_macos
note: Linux/macOS commands are recorded for CI follow-up.
"@ | Set-Content -Encoding UTF8 $report

Log "Done. Report: $report"
