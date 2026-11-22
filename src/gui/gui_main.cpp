#include "MainWindow.h"
#include <QApplication>
#include <QLoggingCategory>
#include <QStyleFactory>

static const char *APP_STYLESHEET = R"(
    QMainWindow, QWidget {
        background-color: #191724;
        color: #e0def4;
        font-family: "Inter", "Noto Sans", "Segoe UI", sans-serif;
        font-size: 13px;
    }

    /* Sidebar */
    #sidebar {
        background-color: #14121d;
        border-right: 1px solid #26233a;
    }
    #navBtn {
        background-color: transparent;
        color: #908caa;
        border: none;
        border-radius: 10px;
        padding: 12px 14px;
        text-align: left;
        font-size: 14px;
        font-weight: 600;
    }
    #navBtn:hover {
        background-color: #1f1d2e;
        color: #e0def4;
    }
    #navBtn:checked {
        background-color: #26233a;
        color: #ebbcba;
        border-left: 3px solid #ebbcba;
    }

    /* Toolbar Cards */
    #toolbarCard {
        background-color: #1f1d2e;
        border: 1px solid #26233a;
        border-radius: 12px;
    }

    /* Buttons */
    QPushButton {
        background-color: #26233a;
        color: #e0def4;
        border: 1px solid #403d52;
        border-radius: 8px;
        padding: 8px 16px;
        font-weight: 600;
    }
    QPushButton:hover {
        background-color: #312e47;
        border-color: #c4a7e7;
    }
    #primaryBtn {
        background-color: #ebbcba;
        color: #191724;
        border: none;
        font-weight: 700;
    }
    #primaryBtn:hover {
        background-color: #f3cbc9;
    }
    #primaryBtn:disabled {
        background-color: #403d52;
        color: #908caa;
    }

    /* ComboBox */
    QComboBox {
        background-color: #26233a;
        color: #e0def4;
        border: 1px solid #403d52;
        border-radius: 8px;
        padding: 6px 12px;
    }
    QComboBox:hover { border-color: #9ccfd8; }
    QComboBox::drop-down { border: none; width: 24px; }
    QComboBox QAbstractItemView {
        background-color: #1f1d2e;
        color: #e0def4;
        selection-background-color: #403d52;
        border: 1px solid #403d52;
    }

    /* Tables & Logs */
    QTableWidget, QPlainTextEdit, QTextBrowser {
        background-color: #1f1d2e;
        alternate-background-color: #191724;
        gridline-color: #26233a;
        border: 1px solid #26233a;
        border-radius: 12px;
        padding: 6px;
    }
    QHeaderView::section {
        background-color: #26233a;
        color: #9ccfd8;
        padding: 8px;
        border: none;
        font-weight: 700;
    }

    /* Modern Scrollbars */
    QScrollBar:vertical, QScrollBar:horizontal {
        background: #14121d;
        width: 8px;
        height: 8px;
        border-radius: 4px;
    }
    QScrollBar::handle {
        background: #403d52;
        border-radius: 4px;
    }
    QScrollBar::handle:hover { background: #6e6a86; }
    QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)";

int main(int argc, char *argv[]) {
  QLoggingCategory::setFilterRules("qt.qpa.wayland.textinput=false");

  QApplication app(argc, argv);
  app.setStyle(QStyleFactory::create("Fusion"));
  app.setStyleSheet(APP_STYLESHEET);

  AppPaths paths;
  paths.inputDir = (argc > 1) ? argv[1] : "input";
  paths.outputDir = (argc > 2) ? argv[2] : "output";
#ifdef _WIN32
  paths.backendBin = (argc > 3) ? argv[3] : "build\\timetable_backend.exe";
  paths.checkerBin = (argc > 4) ? argv[4] : "build\\conflict_check.exe";
  paths.formatterBin = (argc > 5) ? argv[5] : "build\\timetable_formatter.exe";
#else
  paths.backendBin = (argc > 3) ? argv[3] : "./build/timetable_backend";
  paths.checkerBin = (argc > 4) ? argv[4] : "./build/conflict_check";
  paths.formatterBin = (argc > 5) ? argv[5] : "./build/timetable_formatter";
#endif

  MainWindow window(paths);
  window.show();
  return app.exec();
}
