from __future__ import annotations
from pathlib import Path
from typing import List

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel, QListWidget, QPushButton, QHBoxLayout


class WelcomeWidget(QWidget):
    openProjectRequested = Signal(Path)
    openProjectDialogRequested = Signal()
    createProjectRequested = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._recent: List[str] = []

        layout = QVBoxLayout(self)
        title = QLabel("Welcome to NuKiTa - AdaScript IDE")
        title.setStyleSheet("font-size: 20px; font-weight: bold;")
        layout.addWidget(title)

        # Quick actions
        btn_row = QHBoxLayout()
        self.btn_new = QPushButton("Create New Project")
        self.btn_open = QPushButton("Open Project...")
        btn_row.addWidget(self.btn_new)
        btn_row.addWidget(self.btn_open)
        layout.addLayout(btn_row)

        layout.addWidget(QLabel("Recent Projects:"))
        self.recent_list = QListWidget(self)
        layout.addWidget(self.recent_list)

        self.btn_new.clicked.connect(self._emit_create)
        self.btn_open.clicked.connect(self._emit_open_dialog)
        self.recent_list.itemActivated.connect(self._open_selected)

    def set_recent(self, items: List[str]):
        self._recent = items
        self.recent_list.clear()
        for p in items:
            self.recent_list.addItem(p)

    def _emit_create(self):
        self.createProjectRequested.emit()

    def _emit_open_dialog(self):
        # Ask MainWindow to present native folder selection
        self.openProjectDialogRequested.emit()

    def _open_selected(self, item):
        path = Path(item.text())
        self.openProjectRequested.emit(path)
