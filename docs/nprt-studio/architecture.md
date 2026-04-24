# NPRT Studio Architecture (Qt + Scintilla)

## Core Modules
- `core/`: application lifecycle, service wiring.
- `ui/`: main window, docking panels, command entry points.
- `editor/`: Scintilla bridge (fallback editor in scaffold).
- `workspace/`: folder/workspace state.
- `language/`: `nprt-lsp` client abstraction.
- `debug/`: `nprt-debug` (DAP) client abstraction.
- `extensions/`: plugin host and sandbox bridge.
- `storage/`: SQLite persistence.

## Integration Roadmap
1. Replace editor fallback with Scintilla backend.
2. Wire LSP JSON-RPC transport and diagnostics.
3. Wire DAP debug sessions and breakpoint model.
4. Add task runner for `aegc`, `nprt-pkg`, `nprt-fmt`, `nprt-prof`.
5. Add plugin sandbox process and permission model.
