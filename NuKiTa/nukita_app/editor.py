from __future__ import annotations
from pathlib import Path
from typing import Iterable, List, Optional

from PySide6.QtWidgets import QPlainTextEdit
from PySide6.QtGui import QFont

from nukita_app.highlighter import AdaHighlighter


class Diagnostic:
    def __init__(self, line: int, message: str):
        self.line = line
        self.message = message

    def __str__(self) -> str:
        return f"Line {self.line}: {self.message}"


class CodeEditor(QPlainTextEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.path: Optional[Path] = None
        # Monospace readable defaults
        self.setFont(QFont("Consolas", 13))
        self.setTabStopDistance(4 * self.fontMetrics().horizontalAdvance(' '))

    def load_file(self, path: Path) -> None:
        self.path = Path(path)
        try:
            text = self.path.read_text(encoding="utf-8")
        except Exception:
            text = ""
        self.setPlainText(text)
        # Attach syntax highlighter to this document
        AdaHighlighter(self.document())

    def save_file(self, path: Optional[Path] = None) -> None:
        if path is not None:
            self.path = Path(path)
        if not self.path:
            return
        self.path.write_text(self.toPlainText(), encoding="utf-8")

    def show_diagnostics(self, errors: Iterable[Diagnostic]) -> None:
        # Minimal stub: could underline lines; for now, no-op.
        pass

    def symbol_under_cursor(self) -> Optional[str]:
        tc = self.textCursor()
        if not tc:
            return None
        tc.select(tc.WordUnderCursor)
        s = tc.selectedText()
        return s or None

    def goto_line(self, line: int) -> None:
        # Move cursor to the specified 1-based line
        line = max(1, line)
        doc = self.document()
        if not doc:
            return
        if line > doc.blockCount():
            line = doc.blockCount()
        block = doc.findBlockByNumber(line - 1)
        if block.isValid():
            tc = self.textCursor()
            tc.setPosition(block.position())
            self.setTextCursor(tc)
