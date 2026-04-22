# 快速入门

1. 从 Release 下载对应平台 `0.2.0-beta` 包并解压。  
2. 将 `bin/` 加入 `PATH`。  
3. 执行 `aegc1 -i src/main.nprt --emit-c build/out.c`。  
4. 用系统 C 编译器将 `build/out.c` 编译为可执行文件。  

示例项目可参考 `examples/hello_nprt/`。
