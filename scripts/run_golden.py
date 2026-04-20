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
IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")
CONTROL_KEYWORDS = {"if", "while", "for", "switch"}
TYPE_KEYWORDS = {
    "char",
    "const",
    "double",
    "float",
    "int",
    "long",
    "short",
    "signed",
    "unsigned",
    "void",
}


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
    lines = [line for line in text.splitlines() if not line.lstrip().startswith("#")]
    tokens = TOKEN_RE.findall("\n".join(lines))
    tokens = [canonicalize_token(token) for token in tokens]
    tokens = expand_simple_declarations(tokens)
    tokens = normalize_empty_control_blocks(tokens)
    tokens = normalize_single_statement_control_blocks(tokens)
    tokens = sort_simple_block_declarations(tokens)
    tokens = strip_redundant_grouping_parentheses(tokens)
    return "\n".join(tokens)


def canonicalize_token(token: str) -> str:
    if token.startswith(("'", '"')):
        return token
    if IDENT_RE.fullmatch(token):
        return token.lower()
    return token


def strip_redundant_grouping_parentheses(tokens: list[str]) -> list[str]:
    normalized = tokens[:]
    while True:
        changed = False
        result: list[str] = []
        index = 0
        while index < len(normalized):
            token = normalized[index]
            if token != "(":
                result.append(token)
                index += 1
                continue

            match_index = find_matching_paren(normalized, index)
            if match_index is None or not can_drop_parentheses(normalized, index, match_index):
                result.append(token)
                index += 1
                continue

            result.extend(normalized[index + 1 : match_index])
            index = match_index + 1
            changed = True

        if not changed:
            return normalized
        normalized = result


def find_matching_paren(tokens: list[str], start: int) -> int | None:
    depth = 0
    for index in range(start, len(tokens)):
        token = tokens[index]
        if token == "(":
            depth += 1
        elif token == ")":
            depth -= 1
            if depth == 0:
                return index
    return None


def find_matching_brace(tokens: list[str], start: int) -> int | None:
    depth = 0
    for index in range(start, len(tokens)):
        token = tokens[index]
        if token == "{":
            depth += 1
        elif token == "}":
            depth -= 1
            if depth == 0:
                return index
    return None


def can_drop_parentheses(tokens: list[str], start: int, end: int) -> bool:
    prev_token = tokens[start - 1] if start > 0 else None
    if prev_token in CONTROL_KEYWORDS:
        return False
    if prev_token is not None and (IDENT_RE.fullmatch(prev_token) or prev_token in {"]", ")"}):
        return False

    inner = tokens[start + 1 : end]
    if not inner:
        return False

    depth = 0
    for token in inner:
        if token == "(":
            depth += 1
        elif token == ")":
            depth -= 1
        elif token == "," and depth == 0:
            return False

    next_token = tokens[end + 1] if end + 1 < len(tokens) else None
    if next_token == "(":
        return False

    return True


def flatten_single_statement_block(body: list[str]) -> list[str] | None:
    if not body or body[0] in TYPE_KEYWORDS or "{" in body or "}" in body:
        return None
    if body.count(";") != 1 or body[-1] != ";":
        return None
    return body


def expand_simple_declarations(tokens: list[str]) -> list[str]:
    expanded: list[str] = []
    index = 0
    while index < len(tokens):
        parsed = parse_simple_declaration(tokens, index)
        if parsed is None:
            expanded.append(tokens[index])
            index += 1
            continue

        type_tokens, declarators, end = parsed
        for declarator in declarators:
            expanded.extend(type_tokens)
            expanded.extend(declarator)
            expanded.append(";")
        index = end

    return expanded


def normalize_empty_control_blocks(tokens: list[str]) -> list[str]:
    normalized: list[str] = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token in CONTROL_KEYWORDS and index + 1 < len(tokens) and tokens[index + 1] == "(":
            end = find_matching_paren(tokens, index + 1)
            if end is not None and end + 2 < len(tokens) and tokens[end + 1] == "{" and tokens[end + 2] == "}":
                normalized.extend(tokens[index : end + 1])
                normalized.append(";")
                index = end + 3
                continue
        if token == "else" and index + 2 < len(tokens) and tokens[index + 1] == "{" and tokens[index + 2] == "}":
            normalized.extend(["else", ";"])
            index += 3
            continue

        normalized.append(token)
        index += 1

    return normalized


def normalize_single_statement_control_blocks(tokens: list[str]) -> list[str]:
    normalized: list[str] = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token in CONTROL_KEYWORDS and index + 1 < len(tokens) and tokens[index + 1] == "(":
            end = find_matching_paren(tokens, index + 1)
            if end is not None and end + 1 < len(tokens) and tokens[end + 1] == "{":
                block_end = find_matching_brace(tokens, end + 1)
                if block_end is not None:
                    body = tokens[end + 2 : block_end]
                    flattened = flatten_single_statement_block(body)
                    if flattened is not None:
                        normalized.extend(tokens[index : end + 1])
                        normalized.extend(flattened)
                        index = block_end + 1
                        continue
        if token == "else" and index + 1 < len(tokens) and tokens[index + 1] == "{":
            block_end = find_matching_brace(tokens, index + 1)
            if block_end is not None:
                body = tokens[index + 2 : block_end]
                flattened = flatten_single_statement_block(body)
                if flattened is not None:
                    normalized.append("else")
                    normalized.extend(flattened)
                    index = block_end + 1
                    continue

        normalized.append(token)
        index += 1

    return normalized


def sort_simple_block_declarations(tokens: list[str]) -> list[str]:
    result: list[str] = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        result.append(token)
        index += 1
        if token != "{":
            continue

        declarations: list[list[str]] = []
        while True:
            decl_end = find_simple_declaration_end(tokens, index)
            if decl_end is None:
                break
            declarations.append(tokens[index:decl_end])
            index = decl_end

        declarations.sort(key=lambda item: tuple(item))
        for declaration in declarations:
            result.extend(declaration)

    return result


def find_simple_declaration_end(tokens: list[str], start: int) -> int | None:
    parsed = parse_simple_declaration(tokens, start)
    return None if parsed is None else parsed[2]


def parse_simple_declaration(tokens: list[str], start: int) -> tuple[list[str], list[list[str]], int] | None:
    if start >= len(tokens) or tokens[start] not in TYPE_KEYWORDS:
        return None

    index = start
    type_tokens: list[str] = []
    while index < len(tokens) and tokens[index] in TYPE_KEYWORDS:
        type_tokens.append(tokens[index])
        index += 1

    declarators: list[list[str]] = []
    current: list[str] = []
    bracket_depth = 0
    saw_identifier = False

    while index < len(tokens):
        token = tokens[index]
        if token in {"=", "{", "}", "(", ")"}:
            return None
        if token == "[":
            bracket_depth += 1
            current.append(token)
            index += 1
            continue
        if token == "]":
            bracket_depth -= 1
            if bracket_depth < 0:
                return None
            current.append(token)
            index += 1
            continue
        if token == "," and bracket_depth == 0:
            if not current or not saw_identifier:
                return None
            declarators.append(current)
            current = []
            saw_identifier = False
            index += 1
            continue
        if token == ";" and bracket_depth == 0:
            if not current or not saw_identifier:
                return None
            declarators.append(current)
            return type_tokens, declarators, index + 1

        if IDENT_RE.fullmatch(token):
            saw_identifier = True
        current.append(token)
        index += 1

    return None


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
