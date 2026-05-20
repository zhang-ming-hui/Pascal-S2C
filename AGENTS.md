+# Repository Guidelines

## Project Structure & Module Organization
`src/` contains the compiler pipeline, split by stage: `lexer/`, `parser/`, `semantic/`, `lower/`, `codegen/`, plus shared utilities in `common/` and orchestration in `driver/`. The entry point is `src/main.cpp`. Tests live under `tests/testcases/`: Pascal inputs in `pascal/`, expected C output in `expected/`, and optional stdin fixtures in `input/`. Helper automation is in `scripts/`, and reference material belongs in `docs/`. Treat `src copy/` as non-authoritative unless a task explicitly targets it.

## Build, Test, and Development Commands
Configure and build with CMake:

```powershell
cmake -S . -B build
cmake --build build
```

This produces `build/Debug/pascal_s2c.exe` on typical MSVC builds. Run the compiler directly with:

```powershell
.\build\Debug\pascal_s2c.exe tests\testcases\pascal\00_main.pas
```

Run the golden-file regression suite with:

```powershell
python scripts\run_golden.py --compiler build\Debug\pascal_s2c.exe
```

Use `--case 07_var_defn_func` to limit scope while iterating.

## Coding Style & Naming Conventions
Use C++17 and keep changes warning-clean under `/W4` or `-Wall -Wextra -Wpedantic`. Follow the existing style: 4-space indentation, opening braces on the same line, and one logical responsibility per file. Prefer `UpperCamelCase` for types (`CompilerError`), `lowerCamelCase` for functions and locals (`parseArgs`), and lowercase file names grouped by subsystem (`parser/parser.cpp`). Keep headers paired with source files and include only what is required.

## Testing Guidelines
There is no separate unit-test framework here; correctness is enforced through golden tests. For every new Pascal fixture, add a matching `.pas` file in `tests/testcases/pascal/` and the expected generated `.c` file in `tests/testcases/expected/`. Add a file under `tests/testcases/input/` only when runtime input is required. Do not merge changes that introduce semantic mismatches in `run_golden.py`.

## Commit & Pull Request Guidelines
Recent history uses short, imperative subjects, often in Chinese, with occasional scoped prefixes such as `docs:`. Keep commit titles brief and specific to one change. Pull requests should state the compiler stage touched, list the test command run, and include representative before/after output or diff snippets when code generation changes.
