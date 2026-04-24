param(
  [string]$WorkspaceRoot = "../..",
  [switch]$Run
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path $WorkspaceRoot
$bootstrapScript = Join-Path $root "scripts/bootstrap_verify.ps1"
$targetOs = if ($IsWindows) { "windows" } elseif ($IsMacOS) { "macos" } else { "linux" }
$suffix = if ($IsWindows) { ".exe" } else { "" }
$buildDir = Join-Path (Resolve-Path ".") "build"
$workspaceBuildDir = Join-Path $root ("build/" + $targetOs)
$aegc0Exe = Join-Path $workspaceBuildDir ("aegc0" + $suffix)
$inputFile = Join-Path (Resolve-Path ".") "src/main.nprt"
$outputExe = Join-Path $buildDir ("hello_world" + $suffix)
$emitC = Join-Path $buildDir "hello_world.generated.c"

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

if (-not (Test-Path $aegc0Exe)) {
  Write-Host "[hello_world] aegc0 not found, running bootstrap verify to build it..."
  powershell -ExecutionPolicy Bypass -File $bootstrapScript -WorkspaceRoot $root | Out-Host
}

if (-not (Test-Path $aegc0Exe)) {
  throw "aegc0 binary still missing after bootstrap: $aegc0Exe"
}

& $aegc0Exe -i $inputFile -o $outputExe --emit-c $emitC
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $outputExe)) {
  throw "hello_world compilation failed"
}

Write-Host "[hello_world] compiled: $outputExe"
Write-Host "[hello_world] emitted C: $emitC"

if ($Run) {
  Write-Host "[hello_world] running executable..."
  & $outputExe
  exit $LASTEXITCODE
}
