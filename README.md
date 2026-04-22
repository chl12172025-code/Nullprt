[![CI](https://github.com/chl12172025-code/Nullprt/actions/workflows/nullprt-ci.yml/badge.svg)](https://github.com/chl12172025-code/Nullprt/actions/workflows/nullprt-ci.yml)
CI passes with expected diagnostics in early-stage bootstrap environments.

## Nullprt（NPRT）支持仓库

本仓库当前包含 **规格文档 + MVP 工程路线图支撑材料**（尚未实现编译器/工具链代码）。

### 入口
- 规格文档：`docs/`\n
  - `docs/00-overview.md`\n
  - `docs/10-language.md`\n
  - `docs/20-abi-ffi.md`\n
  - `docs/30-config-format.md`\n
  - `docs/40-toolchain.md`\n
  - `docs/50-runtime-stdlib.md`\n
  - `docs/60-security-release.md`\n
- 工程化规格（便于实现）：`spec/`\n
  - 配置键空间与示例：`spec/config/`\n
  - `aegc` MVP：`spec/aegc/mvp-compiler.md`\n
  - `npkg` 协议与锁文件：`spec/npkg/`\n
  - LSP MVP：`spec/lsp/mvp.md`\n
  - 运行时/标准库模块地图与 PAL：`spec/runtime/`\n
  - 发行版保护流水线：`spec/security/`\n
  - 自举策略：`spec/bootstrap-strategy.md`\n
- ABI 测试集：`tests/abi/`\n
- 示例项目：`examples/hello_nprt/`\n

### Stage0 开发环境依赖
- Windows：需安装以下任一 C 工具链（Stage0 自举使用）  
  - Visual Studio Build Tools: [https://visualstudio.microsoft.com/visual-cpp-build-tools/](https://visualstudio.microsoft.com/visual-cpp-build-tools/)  
  - LLVM/Clang: [https://llvm.org/](https://llvm.org/)  
  - MinGW-w64: [https://www.mingw-w64.org/](https://www.mingw-w64.org/)
- macOS：Xcode Command Line Tools（`xcode-select --install`）
- Linux：`gcc` 或 `clang`（通过发行版包管理器安装）

上述外部编译器依赖仅在 Stage0 bootstrap 阶段需要；最终 Nullprt 发行包不依赖任何外部编译器。

## 安装

### 下载预编译二进制

从 [Releases](https://github.com/chl12172025-code/Nullprt/releases) 页面下载对应平台的压缩包，解压后将 `bin/` 目录添加到 `PATH`。
