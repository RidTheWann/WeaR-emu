#pragma once

/**
 * @file WeaR_Style.h
 * @brief RPCS3-Style Dark Theme (Toolbar + Table + Log)
 * 
 * Professional flat design. No emojis.
 */

namespace WeaR {

// =============================================================================
// PROFESSIONAL QSS STYLESHEET
// =============================================================================
inline const char* getStyleSheet() {
    return R"(
/* ============================================================
   GLOBAL
   ============================================================ */
* {
    font-family: "Segoe UI", Arial, sans-serif;
}

QMainWindow, QWidget {
    background-color: #1E1E1E;
    color: #CCCCCC;
    font-size: 10pt;
}

/* ============================================================
   TOOLBAR (Top Bar)
   ============================================================ */
QToolBar {
    background-color: #252526;
    border: none;
    border-bottom: 1px solid #3E3E42;
    spacing: 4px;
    padding: 4px 8px;
}

QToolBar::separator {
    background-color: #3E3E42;
    width: 1px;
    margin: 4px 8px;
}

QToolButton {
    background-color: transparent;
    border: none;
    border-radius: 4px;
    color: #CCCCCC;
    font-size: 9pt;
    padding: 6px 12px;
}

QToolButton:hover {
    background-color: #3E3E42;
    color: #FFFFFF;
}

QToolButton:pressed {
    background-color: #0078D4;
}

/* ============================================================
   GAME TABLE
   ============================================================ */
QTableWidget {
    background-color: #1E1E1E;
    alternate-background-color: #252526;
    border: none;
    gridline-color: transparent;
    selection-background-color: #0078D4;
    selection-color: #FFFFFF;
    font-size: 10pt;
}

QTableWidget::item {
    padding: 8px 12px;
    border: none;
}

QTableWidget::item:selected {
    background-color: #0078D4;
    color: #FFFFFF;
}

QTableWidget::item:hover:!selected {
    background-color: #2D2D30;
}

QHeaderView::section {
    background-color: #252526;
    color: #CCCCCC;
    font-weight: 600;
    font-size: 9pt;
    padding: 8px 12px;
    border: none;
    border-bottom: 1px solid #3E3E42;
    border-right: 1px solid #3E3E42;
}

QHeaderView::section:last {
    border-right: none;
}

/* ============================================================
   DOCK WIDGET (Log)
   ============================================================ */
QDockWidget {
    background-color: #1E1E1E;
    border: 1px solid #3E3E42;
    titlebar-close-icon: url(none);
}

QDockWidget::title {
    background-color: #252526;
    color: #CCCCCC;
    font-size: 9pt;
    font-weight: 600;
    padding: 6px 12px;
    border-bottom: 1px solid #3E3E42;
}

QDockWidget::close-button, QDockWidget::float-button {
    background: transparent;
    border: none;
    padding: 2px;
}

QDockWidget::close-button:hover, QDockWidget::float-button:hover {
    background: #3E3E42;
}

/* ============================================================
   LOG CONSOLE
   ============================================================ */
#logConsole {
    background-color: #0D0D0D;
    border: none;
    font-family: "Cascadia Code", "Consolas", "Courier New", monospace;
    font-size: 9pt;
    color: #CCCCCC;
    padding: 8px;
    selection-background-color: #0078D4;
}

/* ============================================================
   STATUS BAR
   ============================================================ */
QStatusBar {
    background-color: #007ACC;
    border: none;
    color: #FFFFFF;
    font-size: 9pt;
    min-height: 22px;
}

QStatusBar::item {
    border: none;
}

QStatusBar QLabel {
    color: #FFFFFF;
    background: transparent;
    padding: 0 4px;
}

/* ============================================================
   BUTTONS
   ============================================================ */
QPushButton {
    background-color: #3E3E42;
    border: 1px solid #4E4E52;
    border-radius: 4px;
    color: #CCCCCC;
    font-size: 9pt;
    padding: 6px 14px;
    min-height: 24px;
}

QPushButton:hover {
    background-color: #4E4E52;
    color: #FFFFFF;
}

QPushButton:pressed {
    background-color: #0078D4;
    border-color: #0078D4;
}

QPushButton:disabled {
    background-color: #2D2D30;
    color: #5A5A5A;
}

/* ============================================================
   SETTINGS DIALOG
   ============================================================ */
QDialog {
    background-color: #1E1E1E;
}

QTabWidget::pane {
    background-color: #252526;
    border: 1px solid #3E3E42;
    border-top: none;
}

QTabBar::tab {
    background-color: #2D2D30;
    border: 1px solid #3E3E42;
    border-bottom: none;
    color: #808080;
    font-size: 9pt;
    padding: 8px 16px;
}

QTabBar::tab:selected {
    background-color: #252526;
    color: #FFFFFF;
}

QTabBar::tab:hover:!selected {
    background-color: #3E3E42;
}

QGroupBox {
    background-color: #252526;
    border: 1px solid #3E3E42;
    border-radius: 4px;
    font-weight: 600;
    color: #0078D4;
    margin-top: 12px;
    padding: 12px;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 4px;
}

/* ============================================================
   INPUTS
   ============================================================ */
QLineEdit, QSpinBox, QComboBox {
    background-color: #3C3C3C;
    border: 1px solid #3E3E42;
    border-radius: 2px;
    color: #CCCCCC;
    font-size: 9pt;
    padding: 5px 8px;
    min-height: 22px;
}

QLineEdit:focus, QComboBox:focus {
    border-color: #0078D4;
}

QCheckBox {
    color: #CCCCCC;
    font-size: 9pt;
    spacing: 6px;
}

QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid #3E3E42;
    border-radius: 2px;
    background-color: #3C3C3C;
}

QCheckBox::indicator:checked {
    background-color: #0078D4;
    border-color: #0078D4;
}

/* ============================================================
   SCROLLBARS
   ============================================================ */
QScrollBar:vertical {
    background: #1E1E1E;
    width: 10px;
}

QScrollBar::handle:vertical {
    background: #5A5A5A;
    min-height: 20px;
    border-radius: 5px;
    margin: 1px;
}

QScrollBar::handle:vertical:hover {
    background: #787878;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background: #1E1E1E;
    height: 10px;
}

QScrollBar::handle:horizontal {
    background: #5A5A5A;
    min-width: 20px;
    border-radius: 5px;
    margin: 1px;
}

/* ============================================================
   MENU
   ============================================================ */
QMenu {
    background-color: #252526;
    border: 1px solid #3E3E42;
    color: #CCCCCC;
    padding: 4px 0;
}

QMenu::item {
    padding: 6px 24px;
}

QMenu::item:selected {
    background-color: #094771;
}

QMenu::separator {
    background-color: #3E3E42;
    height: 1px;
    margin: 4px 8px;
}

/* ============================================================
   TOOLTIPS
   ============================================================ */
QToolTip {
    background-color: #2D2D30;
    border: 1px solid #3E3E42;
    color: #CCCCCC;
    font-size: 9pt;
    padding: 4px 8px;
}
)";
}

} // namespace WeaR
