param(
  [string]$WorkspaceRoot = "."
)

$ErrorActionPreference = "Stop"

function Get-HostOs {
  if ($env:OS -eq "Windows_NT") { return "windows" }
  if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) { return "windows" }
  if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)) { return "macos" }
  return "linux"
}

function Write-Step($msg) {
  Write-Host "[bootstrap-verify] $msg"
}

$root = Resolve-Path $WorkspaceRoot
$reportPath = Join-Path $root "scripts/bootstrap_verify_report.txt"
$detectScript = Join-Path $root "scripts/detect_toolchain.ps1"

function Get-ExeSuffix {
  if ((Get-HostOs) -eq "windows") { return ".exe" }
  return ""
}

function Get-TargetOs {
  return (Get-HostOs)
}

function Build-Aegc0 {
  param(
    [string]$Compiler,
    [string]$Aegc0Dir,
    [string]$OutExe
  )
  Push-Location $Aegc0Dir
  try {
    if ($Compiler -eq "cl") {
      & cl /nologo /W3 /O2 /Fe:$OutExe main.c arena.c lexer.c parser.c sema.c emit_c.c
    } else {
      & $Compiler -O2 -o $OutExe main.c arena.c lexer.c parser.c sema.c emit_c.c
    }
    return ($LASTEXITCODE -eq 0)
  } finally {
    Pop-Location
  }
}

Write-Step "workspace: $root"

if (-not (Test-Path $detectScript)) {
  @"
result: blocked
reason: detect_toolchain.ps1 not found
stage: Stage0 (aegc0 build)
"@ | Set-Content -Encoding UTF8 $reportPath
  Write-Step "blocked: detect_toolchain.ps1 missing. Report written to $reportPath"
  exit 1
}

$psExe = if (Get-Command pwsh -ErrorAction SilentlyContinue) { "pwsh" } else { "powershell" }
$toolJson = & $psExe -ExecutionPolicy Bypass -File $detectScript -JsonOnly 2>$null
if ($LASTEXITCODE -ne 0 -or -not $toolJson) {
  @"
result: blocked
reason: no C compiler toolchain detected
stage: Stage0 (aegc0 build)
toolchain_detected: false
hint: install Visual Studio Build Tools or LLVM/Clang or MinGW
"@ | Set-Content -Encoding UTF8 $reportPath
  Write-Step "blocked: no compiler toolchain found. Install VS Build Tools / LLVM / MinGW."
  exit 1
}

$tool = $toolJson | ConvertFrom-Json
$cc = $tool.compiler_name
Write-Step "C compiler found: $cc ($($tool.compiler_path))"

$aegc0Dir = Join-Path $root "src/seed/aegc0"
$targetOs = Get-TargetOs
$suffix = Get-ExeSuffix
$buildDir = Join-Path $root ("build/" + $targetOs)
$aegc0Exe = Join-Path $buildDir ("aegc0" + $suffix)
$aegc1Exe = Join-Path $buildDir ("aegc1" + $suffix)
$aegc2Exe = Join-Path $buildDir ("aegc2" + $suffix)
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$okBuildA0 = Build-Aegc0 -Compiler $cc -Aegc0Dir $aegc0Dir -OutExe $aegc0Exe

if (-not $okBuildA0 -or -not (Test-Path $aegc0Exe)) {
  @"
result: failed
reason: failed to build aegc0
stage: Stage0
toolchain_detected: true
compiler_name: $cc
compiler_path: $($tool.compiler_path)
target_os: $targetOs
"@ | Set-Content -Encoding UTF8 $reportPath
  exit 1
}

Write-Step "built aegc0"

$aegc1Src = Join-Path $root "src/self/aegc1/main.nprt"
$aegc2Src = Join-Path $root "src/self/aegc1/main.nprt"

& $aegc0Exe -i $aegc1Src -o $aegc1Exe --emit-c (Join-Path $buildDir "aegc1_from_a0.c")
if (-not (Test-Path $aegc1Exe)) {
  @"
result: failed
reason: aegc0 could not produce aegc1 executable
stage: Stage1
toolchain_detected: true
compiler_name: $cc
compiler_path: $($tool.compiler_path)
target_os: $targetOs
"@ | Set-Content -Encoding UTF8 $reportPath
  exit 1
}
Write-Step "built aegc1 via aegc0"

& $aegc0Exe -i $aegc2Src -o $aegc2Exe --emit-c (Join-Path $buildDir "aegc2_from_a1_placeholder.c")
if (-not (Test-Path $aegc2Exe)) {
  @"
result: failed
reason: could not produce aegc2 executable
stage: Stage2
toolchain_detected: true
compiler_name: $cc
compiler_path: $($tool.compiler_path)
target_os: $targetOs
"@ | Set-Content -Encoding UTF8 $reportPath
  exit 1
}
Write-Step "built aegc2 placeholder chain"

@"
result: partial-pass
toolchain_detected: true
compiler_name: $cc
compiler_path: $($tool.compiler_path)
target_os: $targetOs
notes:
- a0->a1 and a0->a2 placeholder chain completed
- true a1->a2 self-compile requires aegc1 full compiler functionality (pending)
- ABI/LSP replay should be run after compiler integration
"@ | Set-Content -Encoding UTF8 $reportPath

Write-Step "done. report: $reportPath"
