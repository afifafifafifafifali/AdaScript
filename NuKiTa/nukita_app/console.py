from __future__ import annotations
from pathlib import Path
from typing import Optional

from PySide6.QtCore import QProcess, QByteArray, QEvent
from PySide6.QtGui import QTextCursor, QKeyEvent, Qt, QFont
from PySide6.QtWidgets import QWidget, QVBoxLayout, QTextEdit, QLineEdit, QLabel

# Try to use QTermWidget if available
try:
    from qtermwidget import QTermWidget  # type: ignore
except Exception:
    QTermWidget = None  # Fallback to QTextEdit-based shell


class ConsolePane(QWidget):
    def __init__(self, shell: str = "cmd", parent=None):
        super().__init__(parent)
        self.shell = shell
        self.proc: Optional[QProcess] = None
        self._use_qterm = QTermWidget is not None

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        if self._use_qterm:
            # Initialize QTermWidget terminal
            self.term = QTermWidget()
            # Configure shell program
            if Qt.platformName().lower().startswith("windows") or self.shell.lower().startswith("cmd"):
                try:
                    self.term.setShellProgram("cmd.exe")  # type: ignore[attr-defined]
                    # Some bindings support setArgs
                    if hasattr(self.term, "setArgs"):
                        self.term.setArgs(["/Q", "/K"])  # type: ignore[attr-defined]
                except Exception:
                    pass
            layout.addWidget(self.term)
            self._banner = None
        else:
            # Fallback: simple text console
            self.output = QTextEdit(self)
            self.output.setReadOnly(True)
            self.output.setFont(QFont("Consolas", 12))
            self.input = QLineEdit(self)
            self.input.setFont(QFont("Consolas", 12))
            self.input.setPlaceholderText("Enter command and press Enter...")
            layout.addWidget(self.output)
            layout.addWidget(self.input)
            # Simple command history
            self._history: list[str] = []
            self._hist_index: int = -1
            # Direct typing support: redirect keys/clicks on output to input
            self.output.installEventFilter(self)
            self.input.installEventFilter(self)
            self.input.returnPressed.connect(self._send_current_line)

    def start_shell(self, cwd: Optional[str] = None):
        if self._use_qterm:
            # Set working directory if API supports it
            try:
                if cwd and hasattr(self.term, "setWorkingDirectory"):
                    self.term.setWorkingDirectory(cwd)  # type: ignore[attr-defined]
            except Exception:
                pass
            # QTermWidget usually starts its shell automatically on creation
            return
        # Fallback path using QProcess
        if self.proc is not None:
            return
        self.proc = QProcess(self)
        if cwd:
            self.proc.setWorkingDirectory(cwd)
        # Always use Windows CMD for this IDE
        self.proc.start("cmd.exe", ["/Q", "/K"]) 
        self.proc.readyReadStandardOutput.connect(self._read_stdout)
        self.proc.readyReadStandardError.connect(self._read_stderr)
        self.proc.finished.connect(lambda *_: self.output.append("\n<process finished>"))
        self.output.append("Console started. Type commands below.\n")
        self.input.setFocus()

    def run_command(self, cmd: str, cwd: Optional[str] = None):
        if self._use_qterm:
            try:
                if cwd and hasattr(self.term, "setWorkingDirectory"):
                    self.term.setWorkingDirectory(cwd)  # type: ignore[attr-defined]
            except Exception:
                pass
            # Send the command to terminal
            try:
                if hasattr(self.term, "sendText"):
                    self.term.sendText(cmd + "\r")  # type: ignore[attr-defined]
                else:
                    # As a last resort, fall back to no-op if API missing
                    pass
            except Exception:
                pass
            return
        # Fallback to QProcess-based shell
        if self.proc is None:
            self.start_shell(cwd)
        if self.proc is not None:
            self.output.append(f"\n> {cmd}")
            self.proc.write((cmd + "\r\n").encode())
            self.input.setFocus()

    # Fallback helper methods (not used with QTermWidget)
    def _read_stdout(self):
        if not self.proc:
            return
        data = bytes(self.proc.readAllStandardOutput()).decode(errors="ignore")
        if data:
            self.output.moveCursor(QTextCursor.End)
            self.output.insertPlainText(data)
            self.output.moveCursor(QTextCursor.End)

    def _read_stderr(self):
        if not self.proc:
            return
        data = bytes(self.proc.readAllStandardError()).decode(errors="ignore")
        if data:
            self.output.moveCursor(QTextCursor.End)
            self.output.insertPlainText(data)
            self.output.moveCursor(QTextCursor.End)

    def _send_current_line(self):
        cmd = self.input.text().rstrip()
        if not cmd or not self.proc:
            return
        # Save to history
        if not self._history or self._history[-1] != cmd:
            self._history.append(cmd)
        self._hist_index = len(self._history)
        self.output.append(f"\n> {cmd}")
        self.proc.write((cmd + "\r\n").encode())
        self.input.clear()

    def focus_input(self):
        if self._use_qterm:
            # No direct input widget; do nothing
            return
        self.input.setFocus()

    def eventFilter(self, obj, event):
        if self._use_qterm:
            return super().eventFilter(obj, event)
        # Redirect typing and clicks on output into the input line
        if obj is self.output:
            if event.type() in (QEvent.MouseButtonPress, QEvent.MouseButtonDblClick):
                self.input.setFocus()
                return True
            if event.type() == QEvent.KeyPress:
                ke: QKeyEvent = event  # type: ignore
                text = ke.text()
                if text:
                    self.input.setFocus()
                    # Insert text into input and keep cursor at end
                    cur = self.input.cursorPosition()
                    t = self.input.text()
                    t = t[:cur] + text + t[cur:]
                    self.input.setText(t)
                    self.input.setCursorPosition(cur + len(text))
                    return True
        elif obj is self.input and event.type() == QEvent.KeyPress:
            ke: QKeyEvent = event  # type: ignore
            if ke.key() == Qt.Key_Up:
                if self._history and self._hist_index > 0:
                    self._hist_index -= 1
                    self.input.setText(self._history[self._hist_index])
                    self.input.setCursorPosition(len(self.input.text()))
                return True
            if ke.key() == Qt.Key_Down:
                if self._history and self._hist_index < len(self._history):
                    self._hist_index += 1
                    if self._hist_index == len(self._history):
                        self.input.clear()
                    else:
                        self.input.setText(self._history[self._hist_index])
                        self.input.setCursorPosition(len(self.input.text()))
                return True
        return super().eventFilter(obj, event)

    def closeEvent(self, event):
        if self._use_qterm:
            return super().closeEvent(event)
        if self.proc is not None:
            try:
                self.proc.write(b"exit\r\n")
                self.proc.waitForFinished(1000)
            except Exception:
                pass
            self.proc.kill()
            self.proc = None
        super().closeEvent(event)
