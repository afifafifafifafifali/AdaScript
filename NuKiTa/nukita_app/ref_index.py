from __future__ import annotations
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from nukita_app.project import Project


def tokenize(line: str) -> List[str]:
    return re.findall(r"[A-Za-z_][A-Za-z0-9_]*", line)


class ReferenceIndex:
    def __init__(self):
        self._built = False
        self._defs: Dict[str, Tuple[Path, int]] = {}
        self._refs: Dict[str, List[Tuple[Path, int]]] = {}

    def build_index(self, project: Project) -> None:
        self._defs.clear()
        self._refs.clear()
        for file in project.iter_source_files():
            try:
                text = file.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue
            for i, line in enumerate(text.splitlines(), start=1):
                # Very simple function definition: fun name(
                m = re.match(r"\s*fun\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", line)
                if m:
                    name = m.group(1)
                    self._defs[name] = (file, i)
                # Record references
                for tok in tokenize(line):
                    self._refs.setdefault(tok, []).append((file, i))
        self._built = True

    def find_references(self, project: Project, symbol: str) -> List[str]:
        if not self._built:
            self.build_index(project)
        refs = self._refs.get(symbol, [])
        # Return as "file:line"
        return [f"{p}:{ln}" for (p, ln) in refs]

    def goto_definition(self, project: Project, symbol: str) -> Optional[Tuple[Path, int]]:
        if not self._built:
            self.build_index(project)
        return self._defs.get(symbol)
