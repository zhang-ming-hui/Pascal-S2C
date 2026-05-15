import argparse
import json
import os
import pathlib
import shutil
import subprocess
import sys
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert Pascal-S2C AST JSON into Graphviz DOT, and optionally render images."
    )
    parser.add_argument("input", type=pathlib.Path, help="Path to .ast.json")
    parser.add_argument(
        "output",
        nargs="?",
        type=pathlib.Path,
        help="Optional output .dot path; defaults to <input>.dot when rendering, otherwise stdout",
    )
    parser.add_argument(
        "--render",
        choices=["svg", "png"],
        help="Render an image after writing the DOT file",
    )
    parser.add_argument(
        "--render-output",
        type=pathlib.Path,
        help="Optional render output path; defaults to <dot>.svg or <dot>.png",
    )
    parser.add_argument(
        "--dot-bin",
        type=pathlib.Path,
        help="Explicit path to dot executable",
    )
    parser.add_argument(
        "--view",
        action="store_true",
        help="Open the rendered image after generation",
    )
    return parser.parse_args()


def is_scalar(value: Any) -> bool:
    return value is None or isinstance(value, (str, int, float, bool))


def is_location_dict(value: Any) -> bool:
    return (
        isinstance(value, dict)
        and set(value.keys()) == {"line", "column"}
        and isinstance(value["line"], int)
        and isinstance(value["column"], int)
    )


def format_scalar(value: Any) -> str:
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "true" if value else "false"
    return str(value)


def escape_dot_label(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


class DotGraph:
    def __init__(self) -> None:
        self._next_id = 0
        self._lines = [
            "digraph AST {",
            '  rankdir=TB;',
            '  graph [fontname="Consolas"];',
            '  node [shape=box, fontname="Consolas"];',
            '  edge [fontname="Consolas"];',
        ]

    def add_node(self, label: str, shape: str = "box") -> str:
        node_id = f"n{self._next_id}"
        self._next_id += 1
        self._lines.append(
            f'  {node_id} [shape={shape}, label="{escape_dot_label(label)}"];'
        )
        return node_id

    def add_edge(self, src: str, dst: str, label: str | None = None) -> None:
        if label is None:
            self._lines.append(f"  {src} -> {dst};")
            return
        self._lines.append(
            f'  {src} -> {dst} [label="{escape_dot_label(label)}"];'
        )

    def render(self) -> str:
        return "\n".join([*self._lines, "}"]) + "\n"


def build_label(obj: dict[str, Any], fallback: str) -> str:
    if "node_type" in obj:
        title = str(obj["node_type"])
    elif obj.get("phase") == "PARSE" and "ast" in obj:
        title = "ParseResult"
    else:
        title = fallback
    lines = [title]

    loc = obj.get("loc")
    if is_location_dict(loc):
        lines.append(f'loc={loc["line"]}:{loc["column"]}')

    for key, value in obj.items():
        if key in {"node_type", "loc"}:
            continue
        if is_scalar(value):
            lines.append(f"{key}={format_scalar(value)}")
            continue
        if isinstance(value, list) and all(is_scalar(item) for item in value):
            rendered = ", ".join(format_scalar(item) for item in value)
            lines.append(f"{key}=[{rendered}]")

    return "\n".join(lines)


def emit_value(graph: DotGraph, value: Any, fallback: str) -> str:
    if isinstance(value, dict):
        return emit_dict(graph, value, fallback)
    if isinstance(value, list):
        return emit_list(graph, value, fallback)
    return graph.add_node(format_scalar(value), shape="plaintext")


def emit_dict(graph: DotGraph, obj: dict[str, Any], fallback: str) -> str:
    node_id = graph.add_node(build_label(obj, fallback))

    for key, value in obj.items():
        if key == "node_type":
            continue
        if key == "loc" and is_location_dict(value):
            continue
        if is_scalar(value):
            continue
        if isinstance(value, list) and all(is_scalar(item) for item in value):
            continue

        child_id = emit_value(graph, value, key)
        graph.add_edge(node_id, child_id, key)

    return node_id


def emit_list(graph: DotGraph, items: list[Any], fallback: str) -> str:
    node_id = graph.add_node(f"{fallback}[{len(items)}]", shape="ellipse")
    for index, item in enumerate(items):
        child_id = emit_value(graph, item, f"{fallback}_{index}")
        graph.add_edge(node_id, child_id, str(index))
    return node_id


def infer_dot_output(input_path: pathlib.Path) -> pathlib.Path:
    if input_path.suffixes[-2:] == [".ast", ".json"]:
        return input_path.with_suffix("").with_suffix(".dot")
    return input_path.with_suffix(".dot")


def infer_render_output(dot_path: pathlib.Path, render_format: str) -> pathlib.Path:
    return dot_path.with_suffix(f".{render_format}")


def find_dot(dot_bin: pathlib.Path | None) -> pathlib.Path | None:
    if dot_bin is not None:
        return dot_bin

    system_dot = shutil.which("dot")
    if system_dot:
        return pathlib.Path(system_dot)

    conda_prefix = pathlib.Path(sys.prefix)
    candidates = [
        conda_prefix / "Library" / "bin" / "dot.exe",
        pathlib.Path("D:/anaconda/Library/bin/dot.exe"),
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def render_graph(dot_exe: pathlib.Path, dot_path: pathlib.Path, output_path: pathlib.Path) -> None:
    format_name = output_path.suffix.lstrip(".").lower()
    if format_name not in {"svg", "png"}:
        raise SystemExit(f"unsupported render output suffix: {output_path.suffix}")

    subprocess.run(
        [str(dot_exe), f"-T{format_name}", str(dot_path), "-o", str(output_path)],
        check=True,
    )


def maybe_view(path: pathlib.Path) -> None:
    if sys.platform != "win32":
        return
    os_startfile = getattr(os, "startfile", None)
    if os_startfile is not None:
        os_startfile(str(path))


def main() -> int:
    args = parse_args()
    data = json.loads(args.input.read_text(encoding="utf-8"))
    graph = DotGraph()
    emit_value(graph, data, "root")
    dot_text = graph.render()

    dot_path = args.output
    if args.render and dot_path is None:
        dot_path = infer_dot_output(args.input)

    if dot_path is None:
        print(dot_text, end="")
        return 0

    dot_path.parent.mkdir(parents=True, exist_ok=True)
    dot_path.write_text(dot_text, encoding="utf-8")

    if not args.render:
        return 0

    dot_exe = find_dot(args.dot_bin)
    if dot_exe is None:
        raise SystemExit(
            "dot executable not found; install Graphviz or pass --dot-bin explicitly"
        )

    render_output = args.render_output or infer_render_output(dot_path, args.render)
    render_output.parent.mkdir(parents=True, exist_ok=True)
    render_graph(dot_exe, dot_path, render_output)

    if args.view:
        maybe_view(render_output)

    print(f"dot: {dot_path}")
    print(f"rendered: {render_output}")
    print(f"dot_exe: {dot_exe}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
