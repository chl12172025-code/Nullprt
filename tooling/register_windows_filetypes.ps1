param(
  [string]$IconPath = "",
  [switch]$WhatIf
)

$ErrorActionPreference = "Stop"

if (-not $IconPath) {
  $IconPath = (Resolve-Path "docs/icons/nullprt.ico" -ErrorAction SilentlyContinue)
  if (-not $IconPath) {
    Write-Host "No .ico found at docs/icons/nullprt.ico, using default shell icon."
    $IconPath = ""
  }
}

$types = @(
  @{ ext = ".nprt";  progId = "Nullprt.Source"; description = "Nullprt Source File" },
  @{ ext = ".nprtcfg"; progId = "Nullprt.Config"; description = "Nullprt Config File" },
  @{ ext = ".nprtpkg"; progId = "Nullprt.Package"; description = "Nullprt Package File" },
  @{ ext = ".nprtir"; progId = "Nullprt.IR"; description = "Nullprt IR File" }
)

foreach ($t in $types) {
  $extKey = "HKCU:\Software\Classes\$($t.ext)"
  $progKey = "HKCU:\Software\Classes\$($t.progId)"
  if ($WhatIf) {
    Write-Host "[WhatIf] set $extKey default = $($t.progId)"
    Write-Host "[WhatIf] set $progKey default = $($t.description)"
    if ($IconPath) { Write-Host "[WhatIf] set $progKey\\DefaultIcon default = $IconPath,0" }
    continue
  }

  New-Item -Path $extKey -Force | Out-Null
  Set-ItemProperty -Path $extKey -Name "(default)" -Value $t.progId

  New-Item -Path $progKey -Force | Out-Null
  Set-ItemProperty -Path $progKey -Name "(default)" -Value $t.description

  if ($IconPath) {
    $iconKey = Join-Path $progKey "DefaultIcon"
    New-Item -Path $iconKey -Force | Out-Null
    Set-ItemProperty -Path $iconKey -Name "(default)" -Value "$IconPath,0"
  }
}

Write-Host "Registered Nullprt file types under HKCU:\\Software\\Classes"
