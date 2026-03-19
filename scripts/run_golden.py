#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass
from typing import Iterable

ROOT = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_BUILD = ROOT / "build" / "Debug" / "pascal_s2c.exe"
PASCAL_DIR = ROOT / "tests" / "testcases" / "pascal"
EXPECTED_DIR = ROOT / "tests" / "testcases" / "expected"
GENERATED_DIR = ROOT / "build" / "golden"
TOKEN_RE = re.compile(
    r'"(?:\\.|[^"\\])*"'
    r"|'(?:\\.|[^'\\])*'"
    r"|[A-Za-z_][A-Za-z0-9_]*"
    r"|\d+(?:\.\d+)?"
    r"|==|!=|<=|>=|&&|\|\||\+\+|--|->"
    r"|[{}()\[\];,.*+\-/%<>=!&|~?:]"
)


@dataclass
class CaseResult:
    name: str
    exact_match: bool
    normalized_match: bool
    diff: str | None = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run golden-file comparison for pascal_s2c outputs.")
    parser.add_argument("--compiler", type=pathlib.Path, default=DEFAULT_BUILD, help="Path to pascal_s2c executable")
    parser.add_argument("--case", action="append", default=[], help="Run only specific case stem, e.g. 07_var_defn_func")
    parser.add_argument("--out-dir", type=pathlib.Path, default=GENERATED_DIR, help="Directory for generated C files")
    parser.add_argument("--show-diff", type=int, default=10, help="Show unified diff for the first N failing cases")
    parser.add_argument("--exact-only", action="store_true", help="Fail normalized-only mismatches as well")
    return parser.parse_args()


def collect_cases(selected: Iterable[str]) -> list[pathlib.Path]:
    files = sorted(PASCAL_DIR.glob("*.pas"))
    if not selected:
        return files
    wanted = set(selected)
    return [path for path in files if path.stem in wanted]


def ensure_compiler(path: pathlib.Path) -> pathlib.Path:
    compiler = path if path.is_absolute() else (ROOT / path)
    if not compiler.exists():
        raise SystemExit(f"compiler not found: {compiler}")
    return compiler


def normalize_c_text(text: str) -> str:
    tokens = TOKEN_RE.findall(text)
    return "\n".join(tokens)


def run_case(compiler: pathlib.Path, pas_path: pathlib.Path, out_dir: pathlib.Path) -> CaseResult:
    expected_path = EXPECTED_DIR / f"{pas_path.stem}.c"
    if not expected_path.exists():
        raise SystemExit(f"missing expected file: {expected_path}")

    out_dir.mkdir(parents=True, exist_ok=True)
    generated_path = out_dir / expected_path.name

    completed = subprocess.run(
        [str(compiler), str(pas_path), str(generated_path)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
    )
    if completed.returncode != 0:
        message = completed.stderr.strip() or completed.stdout.strip() or "compiler execution failed"
        raise SystemExit(f"{pas_path.stem}: {message}")

    expected = expected_path.read_text(encoding="utf-8")
    generated = generated_path.read_text(encoding="utf-8")
    exact = generated == expected
    normalized = normalize_c_text(generated) == normalize_c_text(expected)
    diff = None
    if not exact:
        diff = "\n".join(
            difflib.unified_diff(
                expected.splitlines(),
                generated.splitlines(),
                fromfile=str(expected_path.relative_to(ROOT)),
                tofile=str(generated_path.relative_to(ROOT)),
                lineterm="",
            )
        )
    return CaseResult(pas_path.stem, exact, normalized, diff)


def main() -> int:
    args = parse_args()
    compiler = ensure_compiler(args.compiler)
    cases = collect_cases(args.case)
    if not cases:
        print("no cases selected", file=sys.stderr)
        return 1

    results = [run_case(compiler, path, args.out_dir) for path in cases]
    exact_ok = sum(1 for item in results if item.exact_match)
    normalized_only = [item for item in results if not item.exact_match and item.normalized_match]
    semantic_fail = [item for item in results if not item.normalized_match]

    print(f"cases: {len(results)}")
    print(f"exact match: {exact_ok}")
    print(f"normalized-only mismatch: {len(normalized_only)}")
    print(f"semantic/structural mismatch: {len(semantic_fail)}")

    if normalized_only:
        print("normalized-only:")
        for item in normalized_only:
            print(f"  {item.name}")

    if semantic_fail:
        print("semantic/structural:")
        for item in semantic_fail:
            print(f"  {item.name}")

    shown = 0
    for item in results:
        if item.exact_match or item.diff is None:
            continue
        if shown >= args.show_diff:
            break
        print()
        print(f"--- diff: {item.name} ---")
        print(item.diff)
        shown += 1

    if args.exact_only:
        return 0 if exact_ok == len(results) else 1
    return 0 if not semantic_fail else 1


if __name__ == "__main__":
    raise SystemExit(main())
