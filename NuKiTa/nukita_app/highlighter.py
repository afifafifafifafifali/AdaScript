from __future__ import annotations
import re
from typing import List

from PySide6.QtCore import QRegularExpression
from PySide6.QtGui import QColor, QTextCharFormat, QFont, QSyntaxHighlighter, QTextDocument


class AdaHighlighter(QSyntaxHighlighter):
    def __init__(self, parent: QTextDocument):
        super().__init__(parent)
        self.highlighting_rules = []
        
        # Define text formats
        keyword_format = QTextCharFormat()
        keyword_format.setForeground(QColor(128, 0, 255))  # Purple
        keyword_format.setFontWeight(QFont.Bold)
        
        # AdaScript keywords
        keywords = [
            'fun', 'var', 'if', 'else', 'while', 'for', 'return', 'true', 'false',
            'null', 'and', 'or', 'not', 'in', 'import', 'from', 'as', 'class',
            'try', 'catch', 'throw', 'break', 'continue', 'switch', 'case', 'default',
            'const', 'let', 'async', 'await', 'yield', 'with', 'using', 'match'
        ]
        
        for keyword in keywords:
            pattern = QRegularExpression(rf'\b{keyword}\b')
            self.highlighting_rules.append((pattern, keyword_format))
        
        # Built-in functions and types
        builtin_format = QTextCharFormat()
        builtin_format.setForeground(QColor(0, 128, 128))  # Teal
        builtin_format.setFontWeight(QFont.Bold)
        
        builtins = [
            'print', 'println', 'input', 'len', 'type', 'str', 'int', 'float',
            'bool', 'list', 'dict', 'set', 'tuple', 'range', 'enumerate',
            'map', 'filter', 'reduce', 'zip', 'sort', 'reverse', 'max', 'min',
            'sum', 'abs', 'round', 'pow', 'sqrt', 'sin', 'cos', 'tan',
            'log', 'exp', 'random', 'time', 'sleep'
        ]
        
        for builtin in builtins:
            pattern = QRegularExpression(rf'\b{builtin}\b')
            self.highlighting_rules.append((pattern, builtin_format))
        
        # Numbers
        number_format = QTextCharFormat()
        number_format.setForeground(QColor(0, 128, 0))  # Green
        pattern = QRegularExpression(r'\b\d+(\.\d+)?\b')
        self.highlighting_rules.append((pattern, number_format))
        
        # Strings
        string_format = QTextCharFormat()
        string_format.setForeground(QColor(255, 128, 0))  # Orange
        # Double quoted strings
        pattern = QRegularExpression('".*"')
        self.highlighting_rules.append((pattern, string_format))
        # Single quoted strings
        pattern = QRegularExpression("'.*'")
        self.highlighting_rules.append((pattern, string_format))
        
        # Comments
        comment_format = QTextCharFormat()
        comment_format.setForeground(QColor(128, 128, 128))  # Gray
        comment_format.setFontItalic(True)
        # Single line comments
        pattern = QRegularExpression('//.*')
        self.highlighting_rules.append((pattern, comment_format))
        # Multi-line comments start
        pattern = QRegularExpression('/\\*')
        self.highlighting_rules.append((pattern, comment_format))
        
        # Functions
        function_format = QTextCharFormat()
        function_format.setForeground(QColor(0, 0, 255))  # Blue
        function_format.setFontWeight(QFont.Bold)
        pattern = QRegularExpression(r'\b[A-Za-z_][A-Za-z0-9_]*(?=\()')
        self.highlighting_rules.append((pattern, function_format))
        
        # Operators
        operator_format = QTextCharFormat()
        operator_format.setForeground(QColor(255, 0, 0))  # Red
        operators = ['=', '==', '!=', '<', '<=', '>', '>=', '\\+', '-', '\\*', '/', '%', '\\^']
        for op in operators:
            pattern = QRegularExpression(op)
            self.highlighting_rules.append((pattern, operator_format))
    
    def highlightBlock(self, text: str):
        # Apply all highlighting rules
        for pattern, format_obj in self.highlighting_rules:
            expression = pattern
            match_iterator = expression.globalMatch(text)
            while match_iterator.hasNext():
                match = match_iterator.next()
                self.setFormat(match.capturedStart(), match.capturedLength(), format_obj)
        
        # Handle multi-line comments
        self.setCurrentBlockState(0)
        
        start_expression = QRegularExpression('/\\*')
        end_expression = QRegularExpression('\\*/')
        
        comment_format = QTextCharFormat()
        comment_format.setForeground(QColor(128, 128, 128))
        comment_format.setFontItalic(True)
        
        if self.previousBlockState() != 1:
            start_match = start_expression.match(text)
            start_index = start_match.capturedStart() if start_match.hasMatch() else -1
        else:
            start_index = 0
        
        while start_index >= 0:
            end_match = end_expression.match(text, start_index)
            end_index = end_match.capturedStart() if end_match.hasMatch() else -1
            
            if end_index == -1:
                self.setCurrentBlockState(1)
                comment_length = len(text) - start_index
            else:
                comment_length = end_index - start_index + end_match.capturedLength()
            
            self.setFormat(start_index, comment_length, comment_format)
            
            start_match = start_expression.match(text, start_index + comment_length)
            start_index = start_match.capturedStart() if start_match.hasMatch() else -1
