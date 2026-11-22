#pragma once

#include <QButtonGroup>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QWidget>
#include <string>
#include <vector>

struct AppPaths {
  std::string inputDir;
  std::string outputDir;
  std::string backendBin;
  std::string checkerBin;
  std::string formatterBin;
};

// Abstract Base Page -> showcases Inheritance & Runtime Polymorphism
class BaseTab : public QWidget {
  Q_OBJECT
protected:
  const AppPaths &paths;

public:
  explicit BaseTab(const AppPaths &p, QWidget *parent = nullptr)
      : QWidget(parent), paths(p) {}
  virtual ~BaseTab() = default;

  virtual QString tabTitle() const = 0;
  virtual QString navLabel() const = 0;
  virtual void refreshData() = 0;
};

// Page 1: Timetable Card Grid Viewer (Custom QScrollArea + QGridLayout)
class TimetableGridTab : public BaseTab {
  Q_OBJECT
  QComboBox *filterMode;
  QComboBox *filterValue;
  QScrollArea *scrollArea;
  QWidget *gridContainer;
  QLabel *statusLabel;

  void populateFilterValues();
  void renderGrid();

public:
  explicit TimetableGridTab(const AppPaths &p, QWidget *parent = nullptr);
  QString tabTitle() const override { return "Timetable Schedule"; }
  QString navLabel() const override { return "  Schedule"; }
  void refreshData() override;
};

// Page 2: Input CSV Editor
class InputDataTab : public BaseTab {
  Q_OBJECT
  QComboBox *fileSelector;
  QTableWidget *table;

  void loadSelectedCSV();
  void saveSelectedCSV();

public:
  explicit InputDataTab(const AppPaths &p, QWidget *parent = nullptr);
  QString tabTitle() const override { return "Input Data Editor"; }
  QString navLabel() const override { return "  Input CSVs"; }
  void refreshData() override { loadSelectedCSV(); }
};

// Page 3: Pipeline Runner & Live Logs
class PipelineTab : public BaseTab {
  Q_OBJECT
  QPushButton *runBtn;
  QPlainTextEdit *logConsole;
  QProcess *process;

  void runPipeline();

signals:
  void pipelineFinished();

public:
  explicit PipelineTab(const AppPaths &p, QWidget *parent = nullptr);
  QString tabTitle() const override { return "Generate & Validate"; }
  QString navLabel() const override { return "  Run Engine"; }
  void refreshData() override {}
};

// Page 4: HTML Print Viewer
class HtmlViewerTab : public BaseTab {
  Q_OBJECT
  QComboBox *htmlSelector;
  QTextBrowser *viewer;

  void loadSelectedHtml();
  void openInBrowser();

public:
  explicit HtmlViewerTab(const AppPaths &p, QWidget *parent = nullptr);
  QString tabTitle() const override { return "HTML Print Preview"; }
  QString navLabel() const override { return "  Print View"; }
  void refreshData() override;
};

// Main Dashboard Window with Sidebar + QStackedWidget
class MainWindow : public QMainWindow {
  Q_OBJECT
  AppPaths paths;
  QStackedWidget *stack;
  QLabel *headerTitle;
  QButtonGroup *navGroup;
  std::vector<BaseTab *> tabList;

public:
  explicit MainWindow(const AppPaths &p, QWidget *parent = nullptr);
  void refreshAllTabs();
};
