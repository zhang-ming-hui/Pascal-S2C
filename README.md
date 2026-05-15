# Pascal-S2C

这是一个编译原理课程项目，用于把 Pascal-S 子集程序翻译为 C 代码。

## 构建

使用 CMake 构建：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

典型可执行文件路径：

```text
build\Release\pascal_s2c.exe
```

## 基本用法

将 Pascal 源文件翻译为 C：

```powershell
.\build\Release\pascal_s2c.exe .\tests\testcases\pascal\00_main.pas
```

也可以显式指定输出文件：

```powershell
.\build\Release\pascal_s2c.exe .\tests\testcases\pascal\00_main.pas out.c
```

导出词法 token：

```powershell
.\build\Release\pascal_s2c.exe --lexer .\tests\testcases\pascal\00_main.pas
```

默认会生成同名 `.tokens` 文件。

## AST JSON

如果只想跑到 parser，并导出 AST JSON：

```powershell
.\build\Release\pascal_s2c.exe --parse-json .\tests\testcases\pascal\06_func_defn.pas
```

默认会生成同名 `.ast.json` 文件，例如：

```text
tests\testcases\pascal\06_func_defn.ast.json
```

再把 AST JSON 转成 Graphviz DOT：

```powershell
python .\scripts\ast_json_to_dot.py .\tests\testcases\pascal\06_func_defn.ast.json .\build\06_func_defn.dot
```

也可以直接一步生成 SVG：

```powershell
python .\scripts\ast_json_to_dot.py .\tests\testcases\pascal\06_func_defn.ast.json --render svg
```

或者直接生成 PNG：

```powershell
python .\scripts\ast_json_to_dot.py .\tests\testcases\pascal\06_func_defn.ast.json --render png
```

如果需要显式指定 `dot.exe` 路径：

```powershell
python .\scripts\ast_json_to_dot.py .\tests\testcases\pascal\06_func_defn.ast.json --render svg --dot-bin D:\anaconda\Library\bin\dot.exe
```

## 回归测试

### Golden 样例

```powershell
python .\scripts\run_golden.py --compiler .\build\Release\pascal_s2c.exe
```

### 统一回归脚本

更推荐使用统一回归脚本：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe
```

它会同时检查：

- `tests/testcases/` 下的正常翻译样例
- `tests/errorcases/lexer/` 下的词法错误样例
- `tests/errorcases/parser/` 下的语法错误恢复样例

详细用法见：

- [scripts/README_regression.md](scripts/README_regression.md)

## 当前说明

- `case` 语句已经接入 parser 层骨架。
- 语义分析与代码生成对 `case` 仍未完全打通。
- 如果你在修改 lexer / parser 的错误恢复逻辑，建议优先运行：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden
```
