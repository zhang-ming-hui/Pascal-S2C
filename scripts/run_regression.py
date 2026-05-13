#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
from dataclasses import dataclass

ROOT = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_COMPILER = ROOT / "build" / "Debug" / "pascal_s2c.exe"
GOLDEN_SCRIPT = ROOT / "scripts" / "run_golden.py"
GOLDEN_PASCAL_DIR = ROOT / "tests" / "testcases" / "pascal"
GOLDEN_EXPECTED_DIR = ROOT / "tests" / "testcases" / "expected"
LEXER_ERROR_DIR = ROOT / "tests" / "errorcases" / "lexer"
LEXER_EXPECTED_PATH = LEXER_ERROR_DIR / "expected_messages.json"
PARSER_ERROR_DIR = ROOT / "tests" / "errorcases" / "parser"
PARSER_EXPECTED_PATH = PARSER_ERROR_DIR / "expected_messages.json"


@dataclass
class ErrorCaseResult:
    name: str
    ok: bool
    expected_returncode: int
    actual_returncode: int
    expected_stderr: str
    actual_stderr: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run unified regression checks for golden outputs and parser error-recovery cases."
    )
    parser.add_argument(
        "--compiler",
        type=pathlib.Path,
        default=DEFAULT_COMPILER,
        help="Path to pascal_s2c executable",
    )
    parser.add_argument(
        "--skip-golden",
        action="store_true",
        help="Skip golden-file regression tests",
    )
    parser.add_argument(
        "--skip-lexer-errors",
        action="store_true",
        help="Skip lexer error-recovery regression tests",
    )
    parser.add_argument(
        "--skip-parser-errors",
        action="store_true",
        help="Skip parser error-recovery regression tests",
    )
    parser.add_argument(
        "--golden-case",
        action="append",
        default=[],
        help="Forwarded to run_golden.py --case",
    )
    parser.add_argument(
        "--lexer-case",
        action="append",
        default=[],
        help="Run only specific lexer error case stems",
    )
    parser.add_argument(
        "--parser-case",
        action="append",
        default=[],
        help="Run only specific parser error case stems",
    )
    parser.add_argument(
        "--show-lexer-details",
        type=int,
        default=10,
        help="Show mismatch details for up to N lexer error cases",
    )
    parser.add_argument(
        "--show-parser-details",
        type=int,
        default=10,
        help="Show mismatch details for up to N parser error cases",
    )
    parser.add_argument(
        "--golden-show-diff",
        type=int,
        default=10,
        help="Forwarded to run_golden.py --show-diff",
    )
    parser.add_argument(
        "--golden-exact-only",
        action="store_true",
        help="Forwarded to run_golden.py --exact-only",
    )
    return parser.parse_args()


def ensure_compiler(path: pathlib.Path) -> pathlib.Path:
    compiler = path if path.is_absolute() else (ROOT / path)
    if not compiler.exists():
        raise SystemExit(f"compiler not found: {compiler}")
    return compiler


def load_parser_expectations() -> dict[str, dict[str, object]]:
    if not PARSER_EXPECTED_PATH.exists():
        raise SystemExit(f"missing parser error expectations: {PARSER_EXPECTED_PATH}")
    with PARSER_EXPECTED_PATH.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def load_lexer_expectations() -> dict[str, dict[str, object]]:
    if not LEXER_EXPECTED_PATH.exists():
        raise SystemExit(f"missing lexer error expectations: {LEXER_EXPECTED_PATH}")
    with LEXER_EXPECTED_PATH.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def run_golden_suite(compiler: pathlib.Path, args: argparse.Namespace) -> int:
    pascal_cases = {path.stem for path in GOLDEN_PASCAL_DIR.glob("*.pas")}
    expected_cases = {path.stem for path in GOLDEN_EXPECTED_DIR.glob("*.c")}
    missing_expected = sorted(pascal_cases - expected_cases)
    if missing_expected:
        print("golden suite is incomplete: missing expected outputs for")
        for name in missing_expected:
            print(f"  {name}")
        return 1

    command = [
        sys.executable,
        str(GOLDEN_SCRIPT),
        "--compiler",
        str(compiler),
        "--show-diff",
        str(args.golden_show_diff),
    ]
    if args.golden_exact_only:
        command.append("--exact-only")
    for case in args.golden_case:
        command.extend(["--case", case])

    completed = subprocess.run(command, cwd=ROOT)
    return completed.returncode


def collect_error_cases(
    case_dir: pathlib.Path,
    selected: list[str],
    expectations: dict[str, dict[str, object]],
    label: str,
) -> list[pathlib.Path]:
    files = sorted(case_dir.glob("*.pas"))
    if not selected:
        return files
    wanted = set(selected)
    selected_files = [path for path in files if path.stem in wanted]
    missing = sorted(wanted - {path.stem for path in selected_files})
    if missing:
        raise SystemExit(f"unknown {label} cases: {', '.join(missing)}")
    missing_expectations = sorted(name for name in wanted if name not in expectations)
    if missing_expectations:
        raise SystemExit(f"missing {label} expectations: {', '.join(missing_expectations)}")
    return selected_files


def run_error_case(
    compiler: pathlib.Path,
    path: pathlib.Path,
    expectations: dict[str, dict[str, object]],
    output_arg: str | None = None,
) -> ErrorCaseResult:
    expected = expectations.get(path.stem)
    if expected is None:
        raise SystemExit(f"missing expectation for case: {path.stem}")

    command = [str(compiler), str(path)]
    if output_arg is not None:
        command.append(output_arg)

    completed = subprocess.run(
        command,
        cwd=ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
    )
    actual_stderr = (completed.stderr or completed.stdout).strip()
    expected_stderr = str(expected["stderr"]).strip()
    expected_returncode = int(expected["returncode"])
    ok = completed.returncode == expected_returncode and actual_stderr == expected_stderr
    return ErrorCaseResult(
        name=path.stem,
        ok=ok,
        expected_returncode=expected_returncode,
        actual_returncode=completed.returncode,
        expected_stderr=expected_stderr,
        actual_stderr=actual_stderr,
    )


def run_lexer_suite(compiler: pathlib.Path, args: argparse.Namespace) -> int:
    expectations = load_lexer_expectations()
    cases = collect_error_cases(LEXER_ERROR_DIR, args.lexer_case, expectations, "lexer")
    if not cases:
        print("lexer error cases: 0")
        return 0

    results = [run_error_case(compiler, path, expectations) for path in cases]
    passed = sum(1 for item in results if item.ok)
    failed = [item for item in results if not item.ok]

    print(f"lexer error cases: {len(results)}")
    print(f"lexer error passed: {passed}")
    print(f"lexer error failed: {len(failed)}")

    for item in failed[: args.show_lexer_details]:
        print()
        print(f"--- lexer mismatch: {item.name} ---")
        print(f"expected return code: {item.expected_returncode}")
        print(f"actual return code:   {item.actual_returncode}")
        print("expected stderr:")
        print(item.expected_stderr)
        print("actual stderr:")
        print(item.actual_stderr)

    return 0 if not failed else 1


def run_parser_suite(compiler: pathlib.Path, args: argparse.Namespace) -> int:
    expectations = load_parser_expectations()
    cases = collect_error_cases(PARSER_ERROR_DIR, args.parser_case, expectations, "parser")
    if not cases:
        print("parser error cases: 0")
        return 0

    results = [run_error_case(compiler, path, expectations, output_arg="ca") for path in cases]
    passed = sum(1 for item in results if item.ok)
    failed = [item for item in results if not item.ok]

    print(f"parser error cases: {len(results)}")
    print(f"parser error passed: {passed}")
    print(f"parser error failed: {len(failed)}")

    for item in failed[: args.show_parser_details]:
        print()
        print(f"--- parser mismatch: {item.name} ---")
        print(f"expected return code: {item.expected_returncode}")
        print(f"actual return code:   {item.actual_returncode}")
        print("expected stderr:")
        print(item.expected_stderr)
        print("actual stderr:")
        print(item.actual_stderr)

    return 0 if not failed else 1


def main() -> int:
    args = parse_args()
    compiler = ensure_compiler(args.compiler)

    overall_status = 0

    if not args.skip_golden:
        print("== golden regression ==")
        overall_status = max(overall_status, run_golden_suite(compiler, args))
        print()

    if not args.skip_lexer_errors:
        print("== lexer error regression ==")
        overall_status = max(overall_status, run_lexer_suite(compiler, args))
        print()

    if not args.skip_parser_errors:
        print("== parser error regression ==")
        overall_status = max(overall_status, run_parser_suite(compiler, args))

    if args.skip_golden and args.skip_lexer_errors and args.skip_parser_errors:
        print("nothing selected")
        return 1

    return overall_status


if __name__ == "__main__":
    raise SystemExit(main())
