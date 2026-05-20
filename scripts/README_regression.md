# 统一回归测试脚本说明

## 作用

`run_regression.py` 是当前仓库的统一回归测试入口。

它会在一次执行中同时检查四类内容：

- `tests/testcases/` 下的正常翻译样例
- `tests/errorcases/lexer/` 下的词法错误样例
- `tests/errorcases/parser/` 下的语法错误恢复样例
- `tests/errorcases/semantic/` 下的语义错误样例

这样在修改 parser、semantic 或错误恢复逻辑之后，可以同时看到：

- 正常 Pascal 到 C 的翻译结果是否回归
- 词法错误检测行为是否回归
- 语法错误恢复和报错信息是否回归
- 语义分析报错行为是否回归

## 相关文件

- 统一回归脚本：`scripts/run_regression.py`
- golden 样例脚本：`scripts/run_golden.py`
- lexer 错误样例期望：`tests/errorcases/lexer/expected_messages.json`
- parser 错误样例期望：`tests/errorcases/parser/expected_messages.json`
- semantic 错误样例期望：`tests/errorcases/semantic/expected_messages.json`

## 基本用法

使用 Release 编译产物运行统一回归：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe
```

如果使用默认的 Debug 路径：

```powershell
python .\scripts\run_regression.py
```

## 脚本会检查什么

### 1. Golden 回归

这一部分内部会调用 `scripts/run_golden.py`。

检查内容是：

- `tests/testcases/pascal/` 中的每个 `.pas`
- 是否能与 `tests/testcases/expected/` 中同名 `.c` 对齐

在真正运行之前，脚本还会先检查 golden 集是否完整。

如果某些 `.pas` 缺少对应的 `.c`，会直接报出缺失项，并将 golden 部分判为失败。

### 2. Lexer 错误回归

这一部分会运行 `tests/errorcases/lexer/` 下的每个错误样例，并检查：

- 返回码是否符合预期
- stderr 是否与期望完全一致

期望基线保存在：

```text
tests/errorcases/lexer/expected_messages.json
```

### 3. Parser 错误恢复回归

这一部分会运行 `tests/errorcases/parser/` 下的每个错误样例，并检查：

- 返回码是否符合预期
- stderr 是否与期望完全一致

期望基线保存在：

```text
tests/errorcases/parser/expected_messages.json
```

因此这部分不是“人工观察”，而是可以稳定回归检查的。

### 4. Semantic 错误回归

这一部分会运行 `tests/errorcases/semantic/` 下的每个错误样例，并检查：

- 返回码是否符合预期
- stderr 是否与期望完全一致

期望基线保存在：

```text
tests/errorcases/semantic/expected_messages.json
```

## 常用命令

只跑 lexer 错误回归：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden --skip-parser-errors --skip-semantic-errors
```

只跑 parser 错误恢复回归：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden --skip-lexer-errors --skip-semantic-errors
```

只跑 semantic 错误回归：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden --skip-lexer-errors --skip-parser-errors
```

只跑 golden 回归：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-parser-errors --skip-semantic-errors
```

只跑一个 lexer 错误样例：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden --skip-parser-errors --skip-semantic-errors --lexer-case 15_invalid_number_leading_dot
```

只跑一个 parser 错误样例：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden --skip-lexer-errors --skip-semantic-errors --parser-case SemicolonBeforeElse
```

一次跑多个 parser 错误样例：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden --skip-lexer-errors --skip-semantic-errors --parser-case SemicolonBeforeElse --parser-case WrongSemicolonAfterBegin
```

只跑一个 semantic 错误样例：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-golden --skip-lexer-errors --skip-parser-errors --semantic-case 01_UNDEFINED_ID
```

只跑一个 golden 样例：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --skip-parser-errors --skip-semantic-errors --golden-case 00_main
```

限制 lexer 错误回归输出的失败详情条数：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --show-lexer-details 3
```

限制 parser 错误回归输出的失败详情条数：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --show-parser-details 3
```

限制 semantic 错误回归输出的失败详情条数：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --show-semantic-details 3
```

将 `--exact-only` 透传给 golden 回归：

```powershell
python .\scripts\run_regression.py --compiler .\build\Release\pascal_s2c.exe --golden-exact-only
```

## 返回码

- 所有选中的测试都通过时，返回 `0`
- 任意一部分失败时，返回非 `0`

因此可以直接用于本地回归检查，也方便后续接入 CI。

## 更新 lexer / parser / semantic 错误样例基线

当 lexer 的错误检测行为被有意修改后，需要同步更新：

```text
tests/errorcases/lexer/expected_messages.json
```

当 parser 的错误恢复或报错信息被有意修改后，需要同步更新：

```text
tests/errorcases/parser/expected_messages.json
```

当 semantic 的报错行为被有意修改后，需要同步更新：

```text
tests/errorcases/semantic/expected_messages.json
```

建议流程：

1. 先重新编译编译器
2. 手动运行受影响的错误样例
3. 确认新的报错行为是你想要的
4. 更新对应的 `expected_messages.json` 条目
5. 重新运行 `run_regression.py`

## 当前限制

如果 golden 样例集本身不完整，`run_regression.py` 会先报告缺失的 expected 文件，并直接将 golden 部分判为失败，而不会继续调用 `run_golden.py`。

目前仓库中仍有部分 golden 样例缺少对应的 expected 文件，所以可能出现：

- lexer 错误回归全部通过
- parser 错误恢复回归全部通过
- semantic 错误回归全部通过
- 但 golden 回归因为缺失样例而失败
