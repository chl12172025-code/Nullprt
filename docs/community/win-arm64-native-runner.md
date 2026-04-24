# Windows ARM64 Native Runner Plan

## Current State
- `windows-latest` cannot guarantee ARM64-native execution for packaging.
- Existing `win-arm64` artifacts from emulated paths are now labeled `win-arm64-emulated`.

## Native Runner Configuration
- Register a self-hosted runner with labels: `self-hosted`, `Windows`, `ARM64`.
- Set repository variable `NPRT_ENABLE_WIN_ARM64_NATIVE=true` (or pass workflow input).
- Release workflow job `build-windows-arm64-native` generates `nullprt-win-arm64.zip`.

## Operational Notes
- Native job validates `PROCESSOR_ARCHITECTURE=ARM64`.
- Publish job accepts either successful native build or skipped native build.
