[![CI](https://github.com/chl12172025-code/Nullprt/actions/workflows/nullprt-ci.yml/badge.svg)](https://github.com/chl12172025-code/Nullprt/actions/workflows/nullprt-ci.yml)
CI passes with expected diagnostics in early-stage bootstrap environments.

## Nullprt（NPRT）支持仓库

当前版本目标：`0.2.0-beta`。

### 文档入口
- 文档站点主页：`docs/index.html`
- API 参考（HTML）：`site/api/index.html`（由 `scripts/generate_api_docs.ps1` 生成）
- 教程目录：`docs/tutorials/`
- 工程规格：`spec/`

### 示例项目
- `examples/hello_nprt/`
- `examples/http_server/`
- `examples/license_verification/`
- `examples/self_protection/`
- `examples/c_interop/`
- `examples/security_research_tool/`（仅开发者版本）

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

从 [Releases](https://github.com/chl12172025-code/Nullprt/releases) 页面下载 `v0.2.0-beta` 对应平台压缩包（Windows/Linux/macOS x x86_64/arm64），解压后将 `bin/` 目录加入 `PATH`。

## 道德与法律合规

安全研究相关能力（硬件标识模拟、VM 行为模拟、跨进程内存操作）仅限授权的安全研究和兼容性测试。  
默认发行构建必须关闭开发者研究能力，不得用于攻击第三方系统或绕过许可验证。
