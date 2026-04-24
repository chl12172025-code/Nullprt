param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_examples_full_completion_report.txt"
$ok = $true

function Check([string]$Name, [scriptblock]$Action) {
  try { & $Action; Add-Content -Path $report -Value "${Name}: ok" }
  catch { $script:ok = $false; Add-Content -Path $report -Value "${Name}: failed ($($_.Exception.Message))" }
}

Set-Content -Path $report -Value "verify_examples_full_completion"

$helloMain = Join-Path $root "examples/hello_world/src/main.nprt"
$helloMath = Join-Path $root "examples/hello_world/src/math.nprt"
$helloTest = Join-Path $root "examples/hello_world/tests/unit_test.nprt"
$helloBench = Join-Path $root "examples/hello_world/bench/bench_fib.nprt"
$httpMain = Join-Path $root "examples/http_server/src/main.nprt"
$licMain = Join-Path $root "examples/license_verification/src/main.nprt"
$selfMain = Join-Path $root "examples/self_protection/src/main.nprt"
$cMain = Join-Path $root "examples/c_interop/src/main.nprt"
$secMain = Join-Path $root "examples/security_research/src/main.nprt"
$case = Join-Path $root "tests/examples/cases/examples_full_sequence.txt"
$expected = Join-Path $root "tests/examples/expected/examples_full_expected.txt"

Check "examples-files" {
  foreach ($p in @($helloMain,$helloMath,$helloTest,$helloBench,$httpMain,$licMain,$selfMain,$cMain,$secMain,$case,$expected)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "examples-feature-markers" {
  $http = Get-Content $httpMain -Raw
  if ($http -notmatch "https_enabled") { throw "http https marker missing" }
  if ($http -notmatch "websocket") { throw "http websocket marker missing" }
  $lic = Get-Content $licMain -Raw
  if ($lic -notmatch "offline_activation_code") { throw "license offline marker missing" }
  if ($lic -notmatch "revocation") { throw "license revocation marker missing" }
  $self = Get-Content $selfMain -Raw
  if ($self -notmatch "integrity_interval_sec") { throw "self integrity marker missing" }
  if ($self -notmatch "tamper_callback") { throw "self tamper marker missing" }
  $c = Get-Content $cMain -Raw
  if ($c -notmatch "callback") { throw "c interop callback marker missing" }
  if ($c -notmatch "union") { throw "c interop union marker missing" }
  $sec = Get-Content $secMain -Raw
  if ($sec -notmatch "hw_sim_profile") { throw "security research hw marker missing" }
  if ($sec -notmatch "scan_region") { throw "security research cross-process marker missing" }
}

Check "examples-readme-common-sections" {
  $readmes = @(
    "examples/hello_world/README.md",
    "examples/http_server/README.md",
    "examples/license_verification/README.md",
    "examples/self_protection/README.md",
    "examples/c_interop/README.md",
    "examples/security_research/README.md",
    "examples/hello_nprt/README.md",
    "examples/security_research_tool/README.md"
  ) | ForEach-Object { Join-Path $root $_ }
  foreach ($r in $readmes) {
    $txt = Get-Content $r -Raw
    if ($txt -notmatch "性能基准数据") { throw "missing benchmark section in $r" }
    if ($txt -notmatch "安全注意事项") { throw "missing security section in $r" }
    if ($txt -notmatch "FAQ") { throw "missing faq section in $r" }
    if ($txt -notmatch "扩展练习") { throw "missing exercises section in $r" }
  }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-examples-full] PASS - report at $report"
  exit 0
}
Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-examples-full] FAIL - report at $report"
exit 1
