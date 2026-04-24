param([string]$WorkspaceRoot = ".")

$ErrorActionPreference = "Stop"
$root = Resolve-Path $WorkspaceRoot
$report = Join-Path $root "scripts/verify_nprt_studio_scaffold_report.txt"
$ok = $true

function Check([string]$Name, [scriptblock]$Action) {
  try {
    & $Action
    Add-Content -Path $report -Value "${Name}: ok"
  } catch {
    $script:ok = $false
    Add-Content -Path $report -Value "${Name}: failed ($($_.Exception.Message))"
  }
}

Set-Content -Path $report -Encoding UTF8 -Value "verify_nprt_studio_scaffold"

$cmake = Join-Path $root "apps/nprt-studio/CMakeLists.txt"
$mainCpp = Join-Path $root "apps/nprt-studio/src/main.cpp"
$appCore = Join-Path $root "apps/nprt-studio/src/core/Application.cpp"
$mainWindow = Join-Path $root "apps/nprt-studio/src/ui/MainWindow.cpp"
$editor = Join-Path $root "apps/nprt-studio/src/editor/EditorWidget.cpp"
$db = Join-Path $root "apps/nprt-studio/src/storage/StudioDatabase.cpp"
$doc = Join-Path $root "docs/nprt-studio/architecture.md"
$lspHeader = Join-Path $root "apps/nprt-studio/src/language/LspClient.h"
$lspImpl = Join-Path $root "apps/nprt-studio/src/language/LspClient.cpp"
$editorHeader = Join-Path $root "apps/nprt-studio/src/editor/EditorWidget.h"
$editorImpl = Join-Path $root "apps/nprt-studio/src/editor/EditorWidget.cpp"
$buildHeader = Join-Path $root "apps/nprt-studio/src/build/BuildService.h"
$buildImpl = Join-Path $root "apps/nprt-studio/src/build/BuildService.cpp"
$dapHeader = Join-Path $root "apps/nprt-studio/src/debug/DapClient.h"
$dapImpl = Join-Path $root "apps/nprt-studio/src/debug/DapClient.cpp"
$pluginDoc = Join-Path $root "docs/nprt-studio/plugin-development-guide.md"
$i18nDoc = Join-Path $root "docs/nprt-studio/i18n-supported-locales.md"
$pluginExample = Join-Path $root "apps/nprt-studio/plugins/examples/hello_plugin.nprt"
$localeEn = Join-Path $root "apps/nprt-studio/resources/i18n/en-US.json"
$localeZh = Join-Path $root "apps/nprt-studio/resources/i18n/zh-CN.json"
$packagingDoc = Join-Path $root "packaging/nprt-studio/README.md"

Check "studio-files-exist" {
  foreach ($p in @($cmake, $mainCpp, $appCore, $mainWindow, $editor, $db, $doc, $lspHeader, $lspImpl, $editorHeader, $editorImpl, $buildHeader, $buildImpl, $dapHeader, $dapImpl, $pluginDoc, $i18nDoc, $pluginExample, $localeEn, $localeZh, $packagingDoc)) {
    if (-not (Test-Path $p)) { throw "missing $p" }
  }
}

Check "studio-cmake-markers" {
  $txt = Get-Content -Raw -Path $cmake
  foreach ($k in @("project(nprt_studio", "Qt6::Widgets", "Qt6::Sql", "NPRT_STUDIO_WITH_SCINTILLA")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-main-window-markers" {
  $txt = Get-Content -Raw -Path $mainWindow
  foreach ($k in @("NPRT Studio", "Explorer", "Problems", "Terminal")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-storage-markers" {
  $txt = Get-Content -Raw -Path $db
  foreach ($k in @("QSQLITE", "studio.db", "recent_workspace")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-lsp-transport-markers" {
  $txt = Get-Content -Raw -Path $lspImpl
  foreach ($k in @("QProcess", "Content-Length", "textDocument/didOpen", "textDocument/publishDiagnostics", "textDocument/completion", "textDocument/definition", "setCompletionCallback", "setDefinitionCallback")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-editor-change-hook-markers" {
  $txt = Get-Content -Raw -Path $editorImpl
  foreach ($k in @("textChanged", "setTextChangedCallback", "currentLine", "currentCharacter", "insertTextAtCursor")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-build-service-markers" {
  $txt = Get-Content -Raw -Path $buildImpl
  foreach ($k in @("runAegcBuild", "runNprtPkgResolve", "QProcess")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-dap-transport-markers" {
  $txt = Get-Content -Raw -Path $dapImpl
  foreach ($k in @("initialize", "Content-Length", "QProcess", "DAP started")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-ui-panels-markers" {
  $txt = Get-Content -Raw -Path $mainWindow
  foreach ($k in @("QFileSystemModel", "QTreeView", "QListWidget", "appendProblem", "updateCompletionItems", "Command Palette", "openWorkspaceFolder", "Ctrl+Space", "F12", "Completion")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-plugin-markers" {
  $txt = Get-Content -Raw -Path $pluginDoc
  foreach ($k in @("sandbox process", "permission", "register_command")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

Check "studio-i18n-markers" {
  $txt = Get-Content -Raw -Path $i18nDoc
  foreach ($k in @("zh-CN", "en-US", "ja-JP", "ru-RU")) {
    if ($txt -notmatch [regex]::Escape($k)) { throw "missing $k" }
  }
}

if ($ok) {
  Add-Content -Path $report -Value "result: pass"
  Write-Host "[verify-nprt-studio-scaffold] PASS - report at $report"
  exit 0
}

Add-Content -Path $report -Value "result: fail"
Write-Host "[verify-nprt-studio-scaffold] FAIL - report at $report"
exit 1
