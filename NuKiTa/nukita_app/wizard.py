from __future__ import annotations
from typing import Dict

from PySide6.QtWidgets import QDialog, QFormLayout, QDialogButtonBox, QLineEdit, QFileDialog, QPushButton, QHBoxLayout


class ProjectWizard(QDialog):
    # Provide compatibility constant used in MainWindow
    Accepted = QDialog.Accepted

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("New Project")
        self._name = QLineEdit(self)
        self._location = QLineEdit(self)
        self._adascript = QLineEdit(self)
        layout = QFormLayout(self)
        layout.addRow("Name:", self._name)
        # Location with Browse button (native Explorer dialog)
        loc_row = QHBoxLayout()
        loc_row.addWidget(self._location)
        btn_browse_loc = QPushButton("Browse...")
        btn_browse_loc.clicked.connect(self._browse_location)
        loc_row.addWidget(btn_browse_loc)
        layout.addRow("Location:", loc_row)
        # Executable path with Browse button
        exe_row = QHBoxLayout()
        exe_row.addWidget(self._adascript)
        btn_browse_exe = QPushButton("Browse...")
        btn_browse_exe.clicked.connect(self._browse_adascript)
        exe_row.addWidget(btn_browse_exe)
        layout.addRow("AdaScript exe:", exe_row)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, self)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def result(self) -> Dict[str, str]:
        return {
            "name": self._name.text() or "NewProject",
            "location": self._location.text() or ".",
            "adascript_path": self._adascript.text() or "",
        }

    def _browse_location(self):
        path = QFileDialog.getExistingDirectory(self, "Select Project Location")
        if path:
            self._location.setText(path)

    def _browse_adascript(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select AdaScript Executable", "", "Executable (*.exe);;All Files (*.*)")
        if path:
            self._adascript.setText(path)
