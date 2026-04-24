# Minimal Toolchain Verification Checklist

This checklist verifies the currently implemented minimal runnable behaviors.

## 1) Static source verification

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_minimal_toolchain.ps1
```

Expected:
- Exit code `0`
- Report written to `scripts/verify_minimal_toolchain_report.txt`

## 2) Package manager state flow (`nprt-pkg`)

Assuming `nprt-pkg` binary is available:

```powershell
nprt-pkg install demo 0.1.0
nprt-pkg list
nprt-pkg update demo 0.1.1
nprt-pkg search demo
nprt-pkg remove demo
```

Expected:
- Local install state stored in `NPRT_CACHE/installed.txt`
- `list/search` reflect updates and removal

## 3) Formatter flow (`nprt-fmt`)

```powershell
nprt-fmt --check examples/hello_world/src/main.nprt
nprt-fmt --diff examples/hello_world/src/main.nprt
nprt-fmt --check-all examples
```

Expected:
- `--diff` shows line-level before/after preview
- `--check-all` prints summary counters

## 4) Documentation generator (`nprt-doc`)

```powershell
nprt-doc docs/api examples/hello_world/src/main.nprt
```

Expected output files:
- `docs/api/index.html`
- `docs/api/index.md`
- `docs/api/index.json`
- `docs/api/sources.txt`
- `docs/api/symbols.txt`

## 5) Profiler (`nprt-prof`)

Create a trace file (example):

```text
main 100
parse 50
parse 25
emit 40
```

Run:

```powershell
nprt-prof text trace.txt
nprt-prof json trace.txt
nprt-prof html trace.txt
nprt-prof svg trace.txt
```

Expected:
- Aggregated totals and hotspot are based on trace values

## 6) Debugger session (`nprt-debug`, dev-gated)

```powershell
nprt-debug status
nprt-debug attach 1234
nprt-debug break main
nprt-debug step over
nprt-debug stack
nprt-debug detach
```

Expected:
- Session state persisted in `NPRT_CACHE/debug_session.state`
- Command order validation (must attach before break/step/stack)
