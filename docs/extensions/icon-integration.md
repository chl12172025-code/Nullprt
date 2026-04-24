# Nullprt Extension Icon Integration

This guide documents practical icon integration for Nullprt extensions.

## VSCode
- Theme file: `docs/icons/nullprt-icon-theme.json`
- Assets: `docs/icons/nullprt.svg`, `docs/icons/nullprt-config.svg`, `docs/icons/nullprt-binary.svg`
- Regenerate theme:
  - `powershell -ExecutionPolicy Bypass -File tooling/generate_icon_theme.ps1`

## Windows file association
- Script: `tooling/register_windows_filetypes.ps1`
- Default command:
  - `powershell -ExecutionPolicy Bypass -File tooling/register_windows_filetypes.ps1`
- Dry-run:
  - `powershell -ExecutionPolicy Bypass -File tooling/register_windows_filetypes.ps1 -WhatIf`

The script writes under `HKCU\Software\Classes` and does not require admin rights.

## Notes
- For Explorer custom icons, provide an `.ico` file at `docs/icons/nullprt.ico`.
- If `nullprt.ico` is missing, association still works and Windows uses default icons.
