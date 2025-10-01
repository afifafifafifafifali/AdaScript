from __future__ import annotations
import json
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Generator, List, Optional, Tuple


@dataclass
class Project:
    name: str
    root: Path
    adascript_path: Optional[Path] = None

    def iter_source_files(self) -> Generator[Path, None, None]:
        # Show .ad files under project root as sources
        for p in self.root.rglob("*.ad"):
            yield p


class ProjectManager:
    def __init__(self):
        self.current: Optional[Project] = None

    def create_project(self, name: str, location: Path, adascript_path: Optional[str] = None) -> Project:
        root = Path(location) / name
        root.mkdir(parents=True, exist_ok=True)
        # Create a minimal project structure (no builtins copied into project)
        (root / "src").mkdir(exist_ok=True)
        (root / "include").mkdir(exist_ok=True)
        # Copy headers from source tree (exclude executable and builtins)
        repo_root = Path.cwd()
        include_src = repo_root / "include"
        if include_src.exists():
            for f in include_src.glob("*.h"):
                shutil.copy2(f, root / "include" / f.name)
        # Optionally: copy AdaScript source examples
        src_hint = repo_root / "src" / "main.cpp"
        if src_hint.exists():
            (root / "src" / "main.cpp").write_text(src_hint.read_text(encoding="utf-8"), encoding="utf-8")
        # Create a minimal project marker with path to adascript
        meta = {"name": name}
        if adascript_path:
            meta["adascript_path"] = adascript_path
        (root / ".nukita.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
        return Project(name=name, root=root, adascript_path=Path(adascript_path) if adascript_path else None)

    def open_project(self, path: Path) -> Optional[Project]:
        path = Path(path)
        marker = path / ".nukita.json"
        if not marker.exists():
            return None
        try:
            data = json.loads(marker.read_text(encoding="utf-8"))
            name = data.get("name", path.name)
            adascript_path = data.get("adascript_path")
        except Exception:
            name = path.name
            adascript_path = None
        self.current = Project(name=name, root=path, adascript_path=Path(adascript_path) if adascript_path else None)
        return self.current

    def build_run_command(self, active_file: Optional[Path] = None) -> Optional[str]:
        r"""
        Build a command string to run inside Windows cmd, using project root as CWD with syntax:
        .\<adascript_exe_relative> <adascript_file_relative>
        If the executable is not under the project root, fall back to quoted absolute path.
        If no active_file provided, run --help.
        """
        proj = self.current
        if not proj:
            return None
        exe_path: Optional[Path] = None
        if proj.adascript_path:
            p = proj.adascript_path
            if not p.is_absolute():
                exe_path = (proj.root / p).resolve()
            else:
                exe_path = p
        else:
            # Look for packaged AdaScript binary in dist/windows, fallback to build.
            project_root = proj.root
            candidates = [
                project_root / "dist" / "windows" / "adascript.exe",
                project_root / "build" / "adascript.exe",
                project_root / "build-linux" / "adascript",
            ]
            exe_path = next((p for p in candidates if p.exists()), None)
        if not exe_path or not exe_path.exists():
            return None
        # Compute relative exe path if under project root
        try:
            rel_exe = exe_path.relative_to(proj.root)
            exe_str = f'.\\{str(rel_exe)}'
        except ValueError:
            exe_str = f'"{str(exe_path)}"'
        if not active_file:
            return f"{exe_str} --help"
        try:
            rel_file = Path(active_file).relative_to(proj.root)
        except ValueError:
            # If file is outside project, just quote it
            rel_file = Path(active_file)
        file_str = f'"{str(rel_file)}"' if ' ' in str(rel_file) else str(rel_file)
        return f"{exe_str} {file_str}"

    def check_syntax(self, source_text: str, path: Optional[Path]) -> List[Tuple[int, str]]:
        # Minimal heuristic checks: unmatched braces/parentheses and unfinished string
        errors: List[Tuple[int, str]] = []
        stack = []
        opens = {'(': ')', '{': '}', '[': ']'}
        closes = {')', '}', ']'}
        in_string = None
        for idx, line in enumerate(source_text.splitlines(), start=1):
            i = 0
            while i < len(line):
                ch = line[i]
                if in_string:
                    if ch == in_string:
                        in_string = None
                    elif ch == '\\':
                        i += 1  # skip escaped
                else:
                    if ch in ('"', "'"):
                        in_string = ch
                    elif ch in opens:
                        stack.append((opens[ch], idx))
                    elif ch in closes:
                        if not stack or stack[-1][0] != ch:
                            errors.append((idx, f"Unmatched closing '{ch}'"))
                        else:
                            stack.pop()
                i += 1
        if in_string:
            errors.append((idx, "Unterminated string literal"))
        for expected, line_no in [(s, l) for (s, l) in stack]:
            errors.append((line_no, f"Missing closing '{expected}'"))
        return errors
