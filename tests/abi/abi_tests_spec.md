## ABI 测试断言（NPRT 侧应满足）

本文件把 `tests/abi/c_lib` 的 C API 转换为 NPRT 侧需要满足的断言集合，便于实现阶段对照。

### ABI-01：标量
- `abi_add_i32(1,2) == 3`
- `abi_add_u64(0xffff_ffff_ffff_fffe, 1) == 0xffff_ffff_ffff_ffff`

### ABI-02：结构体布局
对于：
```c
typedef struct ABI_PairI32 { int32_t a; int32_t b; } ABI_PairI32;
```
NPRT 侧 `repr_c` 的等价 struct 满足：
- `sizeof(ABI_PairI32) == 8`
- `alignof(ABI_PairI32) == 4`
- 字段偏移：`a==0`，`b==4`

对于：
```c
typedef struct ABI_Mixed { uint8_t tag; uint32_t value; uint16_t small; } ABI_Mixed;
```
NPRT 侧满足：
- `sizeof(ABI_Mixed) == abi_sizeof_mixed()`（跨平台由 C 侧给出事实值）\n

### ABI-03：指针+长度
- 传入 `[1,2,3,4]`，`abi_sum_bytes == 10`
- 传入空切片，返回 `0`

### ABI-05：回调
NPRT 定义回调 `cb(x)=x+1`，断言：
- `abi_call_cb_i32(cb, 41) == 42`
- 传入空指针回调：返回 `-1`（按 C 实现）

### ABI-06：导出名
当 NPRT 导出 `export(extern_c, name="nprt_mul_i32") fn mul(a:i32,b:i32)->i32` 时：\n
- 动态库中必须存在符号 `nprt_mul_i32`（无修饰）\n
