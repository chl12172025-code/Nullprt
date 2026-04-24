# NPRT Studio Plugin Development Guide

## Runtime Model
- Plugins are authored in NPRT language.
- Each plugin runs in a sandbox process.
- Host API is permission-gated (fs/network/process/debug/lsp).

## Minimal Plugin
- File: `apps/nprt-studio/plugins/examples/hello_plugin.nprt`
- Entry: `activate(ctx: PluginContext)`.

## API Surface (initial)
- `register_command(id, title, handler)`
- `show_message(text)`
- `workspace_root()`
- `read_text(path)` / `write_text(path, content)` (permission required)

## Security
- Plugin manifest must declare permissions.
- Host shows consent prompt for sensitive permissions.
- Runtime audit log is written for plugin FS/process actions.
