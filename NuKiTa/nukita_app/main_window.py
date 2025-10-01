from __future__ import annotations
import os
from pathlib import Path
from typing import Optional

from PySide6.QtCore import Qt, QTimer, QSettings
from PySide6.QtGui import QAction, QKeySequence
from PySide6.QtWidgets import (
    QMainWindow, QFileDialog, QDockWidget, QListWidget, QMessageBox,
    QToolBar, QTabWidget, QTreeView, QMenu, QInputDialog, QTabBar
)

from nukita_app.editor import CodeEditor
from nukita_app.project import ProjectManager, Project
from nukita_app.wizard import ProjectWizard
from nukita_app.console import ConsolePane
from nukita_app.ref_index import ReferenceIndex
from nukita_app.welcome import WelcomeWidget


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("NuKiTa - AdaScript IDE")
        self.resize(1200, 800)

        self.project_manager = ProjectManager()
        self.ref_index = ReferenceIndex()

        self._setup_ui()
        self._setup_actions()
        self._setup_connections()

        # If launched with a project path via argv
        if len(os.sys.argv) > 1:
            candidate = Path(os.sys.argv[1])
            if candidate.exists():
                self.open_project(candidate)

    def _setup_ui(self):
        self.toolbar = QToolBar("Main")
        self.toolbar.setIconSize(self.toolbar.iconSize() * 1.2)
        self.addToolBar(self.toolbar)

        self.tabs = QTabWidget()
        # We'll manage close buttons per-tab manually
        self.tabs.setTabsClosable(False)
        self.setCentralWidget(self.tabs)

        # Welcome tab with recent projects
        self.welcome = WelcomeWidget(self)
        self.tabs.addTab(self.welcome, "Welcome")

        # Project Explorer (directory tree)
        self.explorer = QTreeView(self)
        self.explorer.setContextMenuPolicy(Qt.CustomContextMenu)
        # Slightly larger row height for readability
        self.explorer.setStyleSheet("QTreeView { font-size: 12pt; } QTreeView::item { padding: 4px 2px; }")
        self.explorer_dock = QDockWidget("Project Explorer", self)
        self.explorer_dock.setWidget(self.explorer)
        self.addDockWidget(Qt.LeftDockWidgetArea, self.explorer_dock)

        # Terminal tab (embedded CMD like VSCode)
        self.console = ConsolePane(shell="cmd")
        self.tabs.addTab(self.console, "Terminal")

        # Status
        self.status = self.statusBar()

    def _setup_actions(self):
        # File menu
        file_menu = self.menuBar().addMenu("&File")
        self.act_new_project = QAction("New Project", self)
        self.act_open_project = QAction("Open Project", self)
        self.act_new_file = QAction("New File", self)
        self.act_new_file.setShortcut(QKeySequence.New)
        self.act_open_file = QAction("Open File", self)
        self.act_save_file = QAction("Save", self)
        self.act_save_file.setShortcut(QKeySequence.Save)
        self.act_save_as = QAction("Save As...", self)
        self.act_exit = QAction("Exit", self)

        file_menu.addAction(self.act_new_project)
        file_menu.addAction(self.act_open_project)
        file_menu.addSeparator()
        file_menu.addAction(self.act_new_file)
        file_menu.addAction(self.act_open_file)
        file_menu.addAction(self.act_save_file)
        file_menu.addAction(self.act_save_as)
        file_menu.addSeparator()
        file_menu.addAction(self.act_exit)

        # Navigate menu
        nav_menu = self.menuBar().addMenu("&Navigate")
        self.act_find_refs = QAction("Find References", self)
        self.act_goto_def = QAction("Go to Definition", self)
        nav_menu.addAction(self.act_find_refs)
        nav_menu.addAction(self.act_goto_def)

        # Run toolbar
        self.act_run = QAction("Run", self)
        self.toolbar.addAction(self.act_run)

    def _setup_connections(self):
        self.act_new_project.triggered.connect(self.new_project)
        self.act_open_project.triggered.connect(self.choose_project_folder)
        self.act_new_file.triggered.connect(self.new_file)
        self.act_open_file.triggered.connect(self.open_file_dialog)
        self.act_save_file.triggered.connect(self.save_current_file)
        self.act_save_as.triggered.connect(self.save_current_file_as)
        self.act_exit.triggered.connect(self.close)

        self.act_run.triggered.connect(self.run_project)
        self.act_find_refs.triggered.connect(self.find_references)
        self.act_goto_def.triggered.connect(self.goto_definition)

        self.explorer.doubleClicked.connect(self._open_from_tree)
        self.explorer.customContextMenuRequested.connect(self._explorer_context_menu)
        # Welcome interactions
        self.welcome.openProjectRequested.connect(self.open_project)
        self.welcome.openProjectDialogRequested.connect(self.choose_project_folder)
        self.welcome.createProjectRequested.connect(self.new_project)

    # Project handling
    def new_project(self):
        wiz = ProjectWizard(self)
        if wiz.exec() == wiz.Accepted:
            result = wiz.result()
            try:
                project = self.project_manager.create_project(**result)
            except Exception as e:
                QMessageBox.critical(self, "Project Creation Failed", str(e))
                return
            self.open_project(project.root)

    def choose_project_folder(self):
        path = QFileDialog.getExistingDirectory(self, "Open Project Directory")
        if path:
            self.open_project(Path(path))

    def open_project(self, path: Path):
        proj = self.project_manager.open_project(path)
        if not proj:
            QMessageBox.warning(self, "Invalid Project", "No .nukita.json found at selected path.")
            return
        self.setWindowTitle(f"NuKiTa - {proj.name}")
        self._populate_explorer(proj)
        # Switch to Welcome and update recent list
        self._remember_recent_project(proj.root)
        self._show_welcome()
        # Start embedded terminal in project directory
        self.console.start_shell(cwd=str(proj.root))
        # Build index initially
        self._reindex_project()

    def _populate_explorer(self, project: Project):
        # Use QFileSystemModel bound to project root
        from PySide6.QtWidgets import QFileSystemModel
        self.fs_model = QFileSystemModel(self)
        self.fs_model.setRootPath(str(project.root))
        self.fs_model.setReadOnly(False)
        self.explorer.setModel(self.fs_model)
        self.explorer.setRootIndex(self.fs_model.index(str(project.root)))
        # Hide columns except Name and Size
        for col in range(2, 4):
            self.explorer.hideColumn(col)

    # File operations
    def new_file(self):
        proj = self.project_manager.current
        initial_dir = str(proj.root) if proj else ""
        path, _ = QFileDialog.getSaveFileName(self, "Create New File", initial_dir, "AdaScript (*.ad);;All Files (*.*)")
        if path:
            p = Path(path)
            try:
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text("", encoding="utf-8")
            except Exception as e:
                QMessageBox.critical(self, "Create File Failed", str(e))
                return
            self.open_file(p)

    def open_file_dialog(self):
        if not self.project_manager.current:
            path, _ = QFileDialog.getOpenFileName(self, "Open File")
        else:
            root = str(self.project_manager.current.root)
            path, _ = QFileDialog.getOpenFileName(self, "Open File", root)
        if path:
            self.open_file(Path(path))

    def _open_from_tree(self, index):
        proj = self.project_manager.current
        if not proj:
            return
        model = self.explorer.model()
        path = Path(model.filePath(index))  # type: ignore[attr-defined]
        if path.is_file():
            self.open_file(path)

    def open_file(self, path: Path):
        # If already open
        for i in range(self.tabs.count()):
            w: CodeEditor = self.tabs.widget(i)  # type: ignore
            if getattr(w, "path", None) == path:
                self.tabs.setCurrentIndex(i)
                return
        editor = CodeEditor(self)
        editor.load_file(path)
        editor.textChanged.connect(self._on_editor_changed)
        self.tabs.addTab(editor, path.name)
        idx = self.tabs.indexOf(editor)
        self._set_tab_clean(idx)
        self.tabs.setCurrentWidget(editor)
        # If Welcome is visible only, keep it as first tab
        self._maybe_hide_welcome()

    def current_editor(self) -> Optional[CodeEditor]:
        w = self.tabs.currentWidget()
        if isinstance(w, CodeEditor):
            return w
        return None

    def save_current_file(self):
        ed = self.current_editor()
        if not ed:
            return
        if not ed.path:
            self.save_current_file_as()
        else:
            ed.save_file()
            # Clear modified dot and show close button
            idx = self.tabs.indexOf(ed)
            if idx != -1:
                base = self.tabs.tabText(idx).lstrip("• ")
                self.tabs.setTabText(idx, base)
                self._set_tab_clean(idx)
            self.status.showMessage("Saved", 2000)
            self._reindex_project(debounce=True)

    def save_current_file_as(self):
        ed = self.current_editor()
        if not ed:
            return
        path, _ = QFileDialog.getSaveFileName(self, "Save As")
        if path:
            ed.save_file(Path(path))
            idx = self.tabs.indexOf(ed)
            if idx != -1:
                self.tabs.setTabText(idx, Path(path).name)
                self._set_tab_clean(idx)
            self._reindex_project(debounce=True)

    # Syntax and references
    def _on_editor_changed(self):
        ed = self.current_editor()
        if not ed:
            return
        # Mark tab with a dot and hide close button while editing
        idx = self.tabs.indexOf(ed)
        if idx != -1:
            base = self.tabs.tabText(idx).lstrip("• ")
            self.tabs.setTabText(idx, f"• {base}")
            self._set_tab_modified(idx)
        self.status.showMessage("Modified", 1000)
        # Debounced syntax check
        QTimer.singleShot(500, self._check_syntax_current)

    def _check_syntax_current(self):
        ed = self.current_editor()
        proj = self.project_manager.current
        if not ed or not proj:
            return
        errors = self.project_manager.check_syntax(ed.toPlainText(), ed.path)
        ed.show_diagnostics(errors)

    def _reindex_project(self, debounce: bool = False):
        proj = self.project_manager.current
        if not proj:
            return
        def do_index():
            self.ref_index.build_index(proj)
        if debounce:
            QTimer.singleShot(500, do_index)
        else:
            do_index()

    def find_references(self):
        ed = self.current_editor()
        proj = self.project_manager.current
        if not ed or not proj:
            return
        symbol = ed.symbol_under_cursor()
        if not symbol:
            self.status.showMessage("No symbol under cursor", 2000)
            return
        refs = self.ref_index.find_references(proj, symbol)
        QMessageBox.information(self, "References", "\n".join(str(r) for r in refs) or "None")

    def goto_definition(self):
        ed = self.current_editor()
        proj = self.project_manager.current
        if not ed or not proj:
            return
        symbol = ed.symbol_under_cursor()
        if not symbol:
            return
        loc = self.ref_index.goto_definition(proj, symbol)
        if loc:
            file, line = loc
            self.open_file(file)
            # Move cursor to line
            new_ed = self.current_editor()
            if new_ed:
                new_ed.goto_line(line)
        else:
            self.status.showMessage("Definition not found", 2000)

    def _show_welcome(self):
        # Ensure welcome tab is present and refreshed
        idx = self.tabs.indexOf(self.welcome)
        if idx == -1:
            self.tabs.insertTab(0, self.welcome, "Welcome")
            idx = 0
        self._update_welcome_recent()
        self.tabs.setCurrentIndex(idx)

    def _maybe_hide_welcome(self):
        # Keep welcome tab present; do nothing. Could hide if desired.
        pass

    def _recent_settings(self) -> QSettings:
        return QSettings("NuKiTa", "AdaScriptIDE")

    def _remember_recent_project(self, path: Path):
        s = self._recent_settings()
        items = s.value("recentProjects", [])
        if not isinstance(items, list):
            items = []
        spath = str(path)
        if spath in items:
            items.remove(spath)
        items.insert(0, spath)
        items = items[:10]
        s.setValue("recentProjects", items)
        self._update_welcome_recent()

    def _load_recent_projects(self) -> list[str]:
        s = self._recent_settings()
        items = s.value("recentProjects", [])
        if isinstance(items, list):
            return [str(x) for x in items]
        return []

    def _update_welcome_recent(self):
        self.welcome.set_recent(self._load_recent_projects())

    def _open_cmd_window(self, cwd: Path):
        # No-op: terminal is embedded now
        pass

    # Tab close button management
    def _set_tab_modified(self, index: int):
        # Hide close button for modified tab
        bar = self.tabs.tabBar()
        bar.setTabButton(index, QTabBar.ButtonPosition.RightSide, None)

    def _set_tab_clean(self, index: int):
        # Show close button for clean tab
        from PySide6.QtWidgets import QStyle, QToolButton
        bar = self.tabs.tabBar()
        btn = QToolButton()
        btn.setAutoRaise(True)
        btn.setIcon(self.style().standardIcon(QStyle.SP_TitleBarCloseButton))
        # Capture index at creation time
        def do_close(i=index):
            self._close_tab(i)
        btn.clicked.connect(do_close)
        bar.setTabButton(index, QTabBar.ButtonPosition.RightSide, btn)

    def _close_tab(self, index: int):
        w = self.tabs.widget(index)
        if isinstance(w, CodeEditor):
            # Optional: prompt to save if needed in the future
            pass
        self.tabs.removeTab(index)

    def _explorer_context_menu(self, pos):
        proj = self.project_manager.current
        if not proj:
            return
        index = self.explorer.indexAt(pos)
        model = self.explorer.model()
        base_path = Path(model.filePath(index)) if index.isValid() else proj.root  # type: ignore[attr-defined]
        if base_path.is_file():
            base_path = base_path.parent
        menu = QMenu(self)
        act_new_file = menu.addAction("New File")
        act_new_folder = menu.addAction("New Folder")
        act_open = menu.addAction("Open")
        act_reveal = menu.addAction("Open in Explorer")
        chosen = menu.exec(self.explorer.viewport().mapToGlobal(pos))
        if chosen == act_new_file:
            name, ok = QInputDialog.getText(self, "New File", "File name:")
            if ok and name:
                p = base_path / name
                try:
                    p.parent.mkdir(parents=True, exist_ok=True)
                    p.write_text("", encoding="utf-8")
                    self.open_file(p)
                except Exception as e:
                    QMessageBox.critical(self, "Create File Failed", str(e))
        elif chosen == act_new_folder:
            name, ok = QInputDialog.getText(self, "New Folder", "Folder name:")
            if ok and name:
                p = base_path / name
                try:
                    p.mkdir(parents=True, exist_ok=True)
                except Exception as e:
                    QMessageBox.critical(self, "Create Folder Failed", str(e))
        elif chosen == act_open and index.isValid():
            p = Path(model.filePath(index))  # type: ignore[attr-defined]
            if p.is_file():
                self.open_file(p)
        elif chosen == act_reveal:
            target = base_path
            # Open Windows Explorer at the folder or select the file
            try:
                import subprocess
                if target.is_file():
                    subprocess.Popen(["explorer", "/select,", str(target)])
                else:
                    subprocess.Popen(["explorer", str(target)])
            except Exception as e:
                QMessageBox.critical(self, "Explorer Error", str(e))

    # Run
    def run_project(self):
        proj = self.project_manager.current
        if not proj:
            QMessageBox.warning(self, "No Project", "Open a project first.")
            return
        # Use current editor file relative to project
        ed = self.current_editor()
        active = ed.path if ed and ed.path else None
        cmd = self.project_manager.build_run_command(active)
        if not cmd:
            QMessageBox.warning(self, "Run Error", "AdaScript executable not configured or no file selected.")
            return
        # Switch to Terminal tab and run inside embedded CMD
        term_idx = self.tabs.indexOf(self.console)
        if term_idx != -1:
            self.tabs.setCurrentIndex(term_idx)
        self.console.run_command(cmd, cwd=str(proj.root))
        self.status.showMessage("Running...", 2000)

    # Helpers
    def closeEvent(self, event):
        # TODO: Prompt to save modified editors
        super().closeEvent(event)
