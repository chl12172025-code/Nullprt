param(
  [string]$WorkspaceRoot = "."
)

$ErrorActionPreference = "Stop"
function Log($m) { Write-Host "[aegc1-selfhost] $m" }

$root = Resolve-Path $WorkspaceRoot
$build = Join-Path $root "build"
$report = Join-Path $root "scripts/aegc1_selfhost_report.txt"
$detect = Join-Path $root "scripts/detect_toolchain.ps1"
New-Item -ItemType Directory -Force -Path $build | Out-Null

$targetOs = if ($IsWindows) { "windows" } elseif ($IsMacOS) { "macos" } else { "linux" }
$suffix = if ($IsWindows) { ".exe" } else { "" }
$build = Join-Path $build $targetOs
New-Item -ItemType Directory -Force -Path $build | Out-Null

$toolJson = & powershell -ExecutionPolicy Bypass -File $detect -JsonOnly 2>$null
if ($LASTEXITCODE -ne 0 -or -not $toolJson) {
  @"
result: blocked
reason: no C compiler detected
"@ | Set-Content -Encoding UTF8 $report
  exit 1
}
$tool = $toolJson | ConvertFrom-Json
$cc = $tool.compiler_name

$aegc1Exe = Join-Path $build ("aegc1_driver" + $suffix)
$aegc2Exe = Join-Path $build ("aegc2_driver" + $suffix)

$srcBase = Join-Path $root "src/self/aegc1"
$cFiles = @(
  "frontend/lexer.c",
  "frontend/parser.c",
  "semantic/typecheck.c",
  "semantic/borrowcheck.c",
  "semantic/monomorphize.c",
  "ir/nprt_ir.c",
  "ir/dump.c",
  "ir/protect_passes.c",
  "backend/c_emitter.c",
  "backend/native/x86_64/lowering.c",
  "backend/native/x86_64/obj_writer.c",
  "backend/native/x86_64/codegen.c",
  "runtime/dev_gate.c",
  "driver/main.c"
) | ForEach-Object { Join-Path $srcBase $_ }

Push-Location $root
try {
  if ($cc -eq "cl") {
    & cl /nologo /W3 /O2 /Fe:$aegc1Exe $cFiles
  } else {
    & $cc -O2 -o $aegc1Exe $cFiles
  }
} finally {
  Pop-Location
}

if (-not (Test-Path $aegc1Exe)) {
  "result: failed`nreason: could not build aegc1 driver" | Set-Content -Encoding UTF8 $report
  exit 1
}

$selfInput = Join-Path $srcBase "main.nprt"
$a1C = Join-Path $build "a1_out.c"
$a1ObjWin = Join-Path $build "a1_out_win.obj"
$a1ObjLinux = Join-Path $build "a1_out_linux.o"
$a1ObjMac = Join-Path $build "a1_out_macos.o"
& $aegc1Exe -i $selfInput --emit-c $a1C --emit-native $a1ObjWin --target win --dump-ir (Join-Path $build "a1.ir.txt")
$rcWin = $LASTEXITCODE
& $aegc1Exe -i $selfInput --emit-native $a1ObjLinux --target linux
$rcLinux = $LASTEXITCODE
& $aegc1Exe -i $selfInput --emit-native $a1ObjMac --target macos
$rcMac = $LASTEXITCODE
$a1ok = ($rcWin -eq 0 -and $rcLinux -eq 0 -and $rcMac -eq 0)

if ($a1ok) {
  if ($cc -eq "cl") {
    & cl /nologo /W3 /O2 /Fe:$aegc2Exe $a1C
  } else {
    & $cc -O2 -o $aegc2Exe $a1C
  }
}

@"
result: $(if ($a1ok -and (Test-Path $aegc2Exe)) { "partial-pass" } else { "failed" })
target_os: $targetOs
compiler_name: $cc
compiler_path: $($tool.compiler_path)
aegc1_built: $(Test-Path $aegc1Exe)
aegc1_emit_c_ok: $a1ok
aegc1_emit_native_win_ok: $(Test-Path $a1ObjWin)
aegc1_emit_native_linux_ok: $(Test-Path $a1ObjLinux)
aegc1_emit_native_macos_ok: $(Test-Path $a1ObjMac)
aegc2_from_a1_c_built: $(Test-Path $aegc2Exe)
link_cmd_windows: link /nologo a1_out_win.obj /OUT:a1_win.exe
link_cmd_linux: cc a1_out_linux.o -o a1_linux
link_cmd_macos: cc a1_out_macos.o -o a1_macos
note: object writers are minimal-format stubs; full relocation completeness pending.
"@ | Set-Content -Encoding UTF8 $report

Log "done. report: $report"
