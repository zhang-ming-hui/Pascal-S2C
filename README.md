# Pascal-S2C

编译原理课程项目，用于将 Pascal-S 子集程序翻译为 C 代码。

## 构建

使用 CMake 构建：

```powershell
cmake -S . -B build
cmake --build build
```

典型的 Windows 输出路径为：

```text
build\Debug\pascal_s2c.exe
```

如果使用 Release 配置，请自行切换到：

```text
build\Release\pascal_s2c.exe
```

## 运行编译器

编译一个 Pascal 文件：

```powershell
.\build\Release\pascal_s2c.exe .\tests\testcases\pascal\00_main.pas
```

也可以显式指定输出文件：

```powershell
.\build\Release\pascal_s2c.exe .\tests\testcases\pascal\00_main.pas out.c
```

## 测试

### 1. Golden 样例回归

仓库原有的 golden 回归脚本是：

```powershell
python .\scripts\run_golden.py --compiler .\build\Release\pascal_s2c.exe
```

### 2. 统一回归脚本

推荐优先使用统一回归脚本：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe
```

这个脚本会同时检查：

- `tests/testcases/` 下的正常翻译样例
- `tests/errorcases/lexer/` 下的词法错误样例
- `tests/errorcases/parser/` 下的语法错误恢复样例

因此每次修改 parser、semantic、错误恢复逻辑后，可以直接通过这一条命令确认：

- 正常翻译是否回归
- 词法错误行为是否回归
- 错误恢复和报错信息是否回归

更详细的用法请见：

- [scripts/README_regression.md](scripts/README_regression.md)

## 当前说明

当前仓库中的 `case` 语句已经接入 parser 骨架，但语义分析和代码生成尚未完整打通。

如果你在修改 parser 的错误恢复逻辑，建议至少运行：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden
```

这样可以更快检查 `tests/errorcases/lexer/` 和 `tests/errorcases/parser/` 下的错误行为是否发生回归。
