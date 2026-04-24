# NPRT Studio v1.0.0 Release Standard

## Scope
- Native desktop IDE implementation with Qt + Scintilla architecture.
- Cross-platform packaging plan for Windows/macOS/Linux.
- Deep NPRT integration (`aegc`, `nprt-pkg`, `nprt-lsp`, `nprt-debug`).

## Required Functional Gates
- Editor core: tabbed editing, split view, undo/redo, autosave/recovery.
- LSP path: initialize, didOpen/didChange, diagnostics, completion, definition.
- Debug path: DAP initialize handshake and adapter lifecycle.
- Build path: `aegc` and `nprt-pkg` task execution.
- Workspace path: folder open and file tree navigation.
- Completion interaction: completion panel selectable and insertable.

## Required Quality Gates
- `scripts/verify_nprt_studio_scaffold.ps1` must pass.
- `scripts/verify_release_nprt_studio.ps1` must pass.
- `tests/aegc1/run_tests.ps1` must include studio verification markers.
- CI must include `nprtstudiorelease` step outcome in summary.

## Packaging Gates
- Packaging matrix document present at `packaging/nprt-studio/README.md`.
- Build script present at `scripts/build_nprt_studio.ps1`.
- Release notes include NPRT Studio module state and known limitations.
