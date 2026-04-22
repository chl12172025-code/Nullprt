## Release Protection Pipeline（发行版自保护流水线）v0

本文件把 `docs/60-security-release.md` 的“允许能力”拆为可实现、可开关、可验证的流水线阶段。\n
原则：**默认安全、可回退、可诊断、仅自保护**。

### 1. 总体结构
在 `build.profile == "release"` 且 `build.obfuscate == true` 时启用：

```text
parse/typecheck
  -> nprt_ir_gen
  -> ir_opt (opt_level)
  -> protect_ir_passes (configurable)
  -> codegen
  -> link
  -> postlink_integrity (optional)
```

### 2. 配置键（建议写入 `NPRTcfg`）
（键名建议，最终以 `spec/config/nprtcfg-keyspace.md` 为准）
- `security.protect.enabled` (bool)\n
- `security.protect.deterministic` (bool) 默认 true\n
- `security.protect.passes` (list[string])\n
- `security.integrity.enabled` (bool)\n
- `security.integrity.mode` (`startup|startup_and_periodic`)\n
- `security.antidebug.enabled` (bool)\n
- `security.antidebug.level` (`off|basic|strict`)\n

### 3. IR 级保护 passes（MVP 最小集合）
#### PASS-01：CFG Flattening（控制流扁平化）
- **输入**：函数的 CFG\n
- **输出**：等价 CFG（引入 dispatcher + state）\n
- **约束**：\n
  - 保持可验证性：提供 `--emit-ir` 以便对照\n
  - deterministic 模式下 state 分配与布局固定\n

#### PASS-02：Instruction Substitution（等价替换）
- **输入**：IR 指令序列\n
- **输出**：等价指令序列（例如 `x+y` 的变形、常量分解）\n
- **约束**：\n
  - 不引入未定义行为\n
  - 不改变溢出语义（若语言定义为二进制补码环绕，需要保持）\n

#### PASS-03：Opaque Predicates（不透明谓词）
- **输入**：条件分支点\n
- **输出**：插入恒真/恒假的复杂判定\n
- **约束**：\n
  - deterministic 模式固定生成策略\n
  - 不依赖环境可变信息（避免时钟/随机数导致不稳定）\n

### 4. 完整性校验（MVP：启动校验）
#### 4.1 目标
- 检测二进制在分发后是否被篡改（自文件完整性）\n

#### 4.2 方案（建议）
- 链接时生成一个 **Integrity Manifest**（段范围 + sha256）\n
- 运行时启动时读取自身映像（或嵌入 manifest）并校验\n
- 校验失败：安全退出（返回码固定）并擦除进程内敏感数据（若有）\n

#### 约束
- 不读取/修改他进程内存\n
- 不隐藏行为；不阻断系统审计\n

### 5. 反调试（MVP：basic）
允许实现：
- 检测本进程是否被调试\n
- 检测常见动态分析环境的最小信号（但不得试图破坏/对抗系统工具）\n

禁止实现：
- 干扰系统安全机制\n
- 针对第三方软件的对抗行为\n

### 6. 可诊断性与回退
必须提供：
- `aegc build --no-protect`：强制关闭所有保护\n
- `aegc build --protect-debug`：输出保护前后 IR diff 摘要（不泄露密钥）\n
- `aegc build --protect-seed <seed>`：仅在非 deterministic 模式下允许显式 seed\n

### 7. 禁区（硬约束）
流水线不得包含或生成以下能力：
- 跨进程注入/模块加载\n
- 未授权访问他进程内存\n
- 修改硬件标识/伪装虚拟机\n
- 隐藏进程/文件/网络连接\n
