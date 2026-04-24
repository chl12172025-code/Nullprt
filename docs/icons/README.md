# Nullprt Icon Theme

This directory contains the file icon assets and a ready-to-use VSCode icon theme mapping.

## Files
- `nullprt.svg`: default source icon
- `nullprt-config.svg`: config and security icon
- `nullprt-binary.svg`: binary and IR icon
- `nullprt-icon-theme.json`: VSCode icon theme payload

## Regenerate theme mapping
Run:

```powershell
powershell -ExecutionPolicy Bypass -File tooling/generate_icon_theme.ps1
```

## Use in VSCode
1. Copy this directory into a VSCode extension package, or point a local extension to this icon theme file.
2. Register `docs/icons/nullprt-icon-theme.json` under `contributes.iconThemes`.

## Publish to VS Code Marketplace
1. Create extension scaffold (or reuse `nprt-vscode` extension package).
2. Add `contributes.iconThemes` entry pointing to `nullprt-icon-theme.json`.
3. Package and publish with `vsce publish` using publisher token.
4. Version icon theme updates with extension releases so users do not need manual setup.
