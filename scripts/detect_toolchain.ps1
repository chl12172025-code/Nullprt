param(
  [ValidateSet("x64","x86","arm64")]
  [string]$Arch = "x64",
  [switch]$JsonOnly
)

$ErrorActionPreference = "Stop"

function Get-HostOs {
  if ($env:OS -eq "Windows_NT") { return "windows" }
  if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) { return "windows" }
  if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)) { return "macos" }
  return "linux"
}

function Set-EnvFromSetOutput {
  param([string[]]$Lines)
  foreach ($line in $Lines) {
    if ($line -notmatch "=") { continue }
    $idx = $line.IndexOf("=")
    if ($idx -le 0) { continue }
    $k = $line.Substring(0, $idx)
    $v = $line.Substring($idx + 1)
    if ([string]::IsNullOrWhiteSpace($k)) { continue }
    [Environment]::SetEnvironmentVariable($k, $v, "Process")
  }
}

function Detect-VSCompiler {
  $vsKeys = @(
    "HKLM:\SOFTWARE\Microsoft\VisualStudio\SxS\VS7",
    "HKLM:\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7"
  )
  $roots = @()
  foreach ($k in $vsKeys) {
    if (Test-Path $k) {
      $props = Get-ItemProperty -Path $k
      foreach ($p in $props.PSObject.Properties) {
        if ($p.Name -match '^\d+\.\d+$' -and $p.Value) { $roots += [string]$p.Value }
      }
    }
  }
  $roots += @(
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools",
    "C:\Program Files\Microsoft Visual Studio\2022\Community"
  )
  $roots = $roots | Where-Object { $_ } | Select-Object -Unique

  foreach ($root in $roots) {
    $vsDev = Join-Path $root "Common7\Tools\VsDevCmd.bat"
    if (-not (Test-Path $vsDev)) { continue }
    $setOut = cmd /c "`"$vsDev`" -arch=$Arch >nul && set"
    if (-not $setOut) { continue }
    Set-EnvFromSetOutput -Lines $setOut
    $cl = Get-Command cl -ErrorAction SilentlyContinue
    if ($cl) {
      return @{
        found = $true
        compiler_name = "cl"
        compiler_path = $cl.Source
        source = "vs"
        env_applied = $true
      }
    }
  }
  return @{ found = $false }
}

function Detect-LLVM {
  $clang = Get-Command clang -ErrorAction SilentlyContinue
  if (-not $clang) {
    $fallback = "C:\Program Files\LLVM\bin\clang.exe"
    if (Test-Path $fallback) {
      $bin = Split-Path $fallback -Parent
      $env:PATH = "$bin;$env:PATH"
      $clang = Get-Command clang -ErrorAction SilentlyContinue
    }
  }
  if ($clang) {
    return @{
      found = $true
      compiler_name = "clang"
      compiler_path = $clang.Source
      source = "llvm"
      env_applied = $true
    }
  }
  return @{ found = $false }
}

function Detect-MinGW {
  $gcc = Get-Command gcc -ErrorAction SilentlyContinue
  if (-not $gcc) {
    $fallbacks = @(
      "C:\msys64\mingw64\bin\gcc.exe",
      "C:\mingw64\bin\gcc.exe"
    )
    foreach ($f in $fallbacks) {
      if (Test-Path $f) {
        $bin = Split-Path $f -Parent
        $env:PATH = "$bin;$env:PATH"
        $gcc = Get-Command gcc -ErrorAction SilentlyContinue
        if ($gcc) { break }
      }
    }
  }
  if ($gcc) {
    return @{
      found = $true
      compiler_name = "gcc"
      compiler_path = $gcc.Source
      source = "mingw"
      env_applied = $true
    }
  }
  return @{ found = $false }
}

$result = $null
$hostOs = Get-HostOs

if ($hostOs -eq "windows") {
  $result = Detect-VSCompiler
  if (-not $result.found) { $result = Detect-LLVM }
  if (-not $result.found) { $result = Detect-MinGW }
} else {
  $cc = Get-Command cc -ErrorAction SilentlyContinue
  if ($cc) {
    $result = @{
      found = $true
      compiler_name = "cc"
      compiler_path = $cc.Source
      source = "system"
      env_applied = $false
    }
  } else {
    $result = @{ found = $false }
  }
}

if ($result.found) {
  $json = [PSCustomObject]@{
    compiler_name = $result.compiler_name
    compiler_path = $result.compiler_path
    source = $result.source
    env_applied = [bool]$result.env_applied
  } | ConvertTo-Json -Compress

  if (-not $JsonOnly) {
    Write-Host "[detect-toolchain] Found compiler: $($result.compiler_name) ($($result.compiler_path)) via $($result.source)"
  }
  Write-Output $json
  exit 0
}

$err = @"
[detect-toolchain] No C compiler toolchain detected.
Install one of the following:
- Visual Studio Build Tools: https://visualstudio.microsoft.com/visual-cpp-build-tools/
- LLVM/Clang: https://llvm.org/
- MinGW-w64: https://www.mingw-w64.org/
"@
if (-not $JsonOnly) { Write-Error $err } else { Write-Output $err }
exit 1
