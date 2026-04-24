# Nullprt Config Overview

Nullprt configuration uses a dedicated syntax and parser to avoid coupling with other ecosystems.

## Key capabilities
- Independent grammar with sections, typed values, expressions, and comments.
- File include and conditional include.
- Environment and CLI overrides.
- Typed query API for runtime/tooling.
- Validation API for required keys and schema checks.

## Primary files
- `NPRTcfg.nprtcfg` (short)
- `NPRTcfg.nullprtconfig` (long alias)
- `deps.nprtpkg`
- `deps.nprtlock`
- `NPRT_CACHE/installed.txt` (package install state for `nprt-pkg`)

## Safety notes
- Security-sensitive values should be loaded via environment or secret files.
- Dev-only high-risk features must remain gated by compile-time and runtime checks.
