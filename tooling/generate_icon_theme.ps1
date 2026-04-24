param(
  [string]$OutputPath = "docs/icons/nullprt-icon-theme.json"
)

$mappings = @(
  @{ ext = "nprt"; icon = "nprt.source" },
  @{ ext = "nullprt"; icon = "nprt.source" },
  @{ ext = "nprti"; icon = "nprt.source" },
  @{ ext = "nullprtinterface"; icon = "nprt.source" },
  @{ ext = "nprtm"; icon = "nprt.source" },
  @{ ext = "nullprtmodule"; icon = "nprt.source" },
  @{ ext = "nprtcfg"; icon = "nprt.config" },
  @{ ext = "nprtconfig"; icon = "nprt.config" },
  @{ ext = "nullprtconfig"; icon = "nprt.config" },
  @{ ext = "nprtenv"; icon = "nprt.config" },
  @{ ext = "nullprtenvironment"; icon = "nprt.config" },
  @{ ext = "nprtsecret"; icon = "nprt.security" },
  @{ ext = "nullprtsecret"; icon = "nprt.security" },
  @{ ext = "nprtpkg"; icon = "nprt.binary" },
  @{ ext = "nullprtpackage"; icon = "nprt.binary" },
  @{ ext = "nprtlock"; icon = "nprt.binary" },
  @{ ext = "nullprtlock"; icon = "nprt.binary" },
  @{ ext = "nprtmanifest"; icon = "nprt.binary" },
  @{ ext = "nullprtmanifest"; icon = "nprt.binary" },
  @{ ext = "nprto"; icon = "nprt.binary" },
  @{ ext = "nullprtobject"; icon = "nprt.binary" },
  @{ ext = "nprtl"; icon = "nprt.binary" },
  @{ ext = "nullprtlibrary"; icon = "nprt.binary" },
  @{ ext = "nprta"; icon = "nprt.binary" },
  @{ ext = "nullprtarchive"; icon = "nprt.binary" },
  @{ ext = "nprtso"; icon = "nprt.binary" },
  @{ ext = "nullprtsharedobject"; icon = "nprt.binary" },
  @{ ext = "nprtdll"; icon = "nprt.binary" },
  @{ ext = "nullprtdynamiclibrary"; icon = "nprt.binary" },
  @{ ext = "nprtb"; icon = "nprt.binary" },
  @{ ext = "nullprtbytecode"; icon = "nprt.binary" },
  @{ ext = "nprtdoc"; icon = "nprt.docs" },
  @{ ext = "nullprtdocument"; icon = "nprt.docs" },
  @{ ext = "nprttut"; icon = "nprt.docs" },
  @{ ext = "nullprttutorial"; icon = "nprt.docs" },
  @{ ext = "nprtex"; icon = "nprt.docs" },
  @{ ext = "nullprtexample"; icon = "nprt.docs" },
  @{ ext = "nprtkey"; icon = "nprt.security" },
  @{ ext = "nullprtkey"; icon = "nprt.security" },
  @{ ext = "nprtcert"; icon = "nprt.security" },
  @{ ext = "nullprtcertificate"; icon = "nprt.security" },
  @{ ext = "nprtlicense"; icon = "nprt.security" },
  @{ ext = "nullprtlicense"; icon = "nprt.security" },
  @{ ext = "nprtsig"; icon = "nprt.security" },
  @{ ext = "nullprtsignature"; icon = "nprt.security" },
  @{ ext = "nprtir"; icon = "nprt.ir" },
  @{ ext = "nullprtintermediaterepresentation"; icon = "nprt.ir" },
  @{ ext = "nprthir"; icon = "nprt.ir" },
  @{ ext = "nullprthir"; icon = "nprt.ir" },
  @{ ext = "nprtmir"; icon = "nprt.ir" },
  @{ ext = "nullprtmir"; icon = "nprt.ir" },
  @{ ext = "nprtlir"; icon = "nprt.ir" },
  @{ ext = "nullprtlir"; icon = "nprt.ir" }
)

$fileExtensions = @{}
foreach ($item in $mappings) {
  $fileExtensions[$item.ext] = $item.icon
}

$payload = [ordered]@{
  '$schema' = 'vscode://schemas/icon-theme'
  iconDefinitions = [ordered]@{
    'nprt.source' = @{ iconPath = './nullprt.svg' }
    'nprt.config' = @{ iconPath = './nullprt-config.svg' }
    'nprt.binary' = @{ iconPath = './nullprt-binary.svg' }
    'nprt.docs' = @{ iconPath = './nullprt.svg' }
    'nprt.security' = @{ iconPath = './nullprt-config.svg' }
    'nprt.ir' = @{ iconPath = './nullprt-binary.svg' }
  }
  fileExtensions = $fileExtensions
}

$json = $payload | ConvertTo-Json -Depth 16
$outputDir = Split-Path -Parent $OutputPath
if ($outputDir -and -not (Test-Path $outputDir)) {
  New-Item -ItemType Directory -Path $outputDir | Out-Null
}
[System.IO.File]::WriteAllText($OutputPath, $json + [Environment]::NewLine)
Write-Host "Generated icon theme at $OutputPath"
