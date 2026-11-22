#include "MainWindow.h"
#include "Models.h"

#include <QDesktopServices>
#include <QDir>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>

namespace fs = std::filesystem;

static int timeToMinutes(const std::string &t) {
  int h = 0, m = 0;
  if (std::sscanf(t.c_str(), "%d:%d", &h, &m) != 2)
    return 0;
  return h * 60 + m;
}

struct SubjectPalette {
  QString bg;
  QString accent;
};

static SubjectPalette colorForSubject(const std::string &subjectName) {
  static const std::vector<SubjectPalette> palette = {
      {"rgba(235, 111, 146, 0.14)", "#eb6f92"},
      {"rgba(156, 207, 216, 0.14)", "#9ccfd8"},
      {"rgba(196, 167, 231, 0.14)", "#c4a7e7"},
      {"rgba(246, 193, 119, 0.14)", "#f6c177"},
      {"rgba(86, 159, 186, 0.16)", "#569fba"},
      {"rgba(235, 188, 186, 0.14)", "#ebbcba"},
      {"rgba(166, 227, 161, 0.14)", "#a6e3a1"},
      {"rgba(137, 180, 250, 0.14)", "#89b4fa"},
      {"rgba(250, 179, 135, 0.14)", "#fab387"},
      {"rgba(245, 194, 231, 0.14)", "#f5c2e7"}};
  size_t idx = std::hash<std::string>{}(subjectName) % palette.size();
  return palette[idx];
}

static void addCardShadow(QWidget *w) {
  auto *shadow = new QGraphicsDropShadowEffect(w);
  shadow->setBlurRadius(14);
  shadow->setOffset(0, 3);
  shadow->setColor(QColor(0, 0, 0, 75));
  w->setGraphicsEffect(shadow);
}

// =========================================================
// PAGE 1: CUSTOM CARD-BASED TIMETABLE GRID
// =========================================================
TimetableGridTab::TimetableGridTab(const AppPaths &p, QWidget *parent)
    : BaseTab(p, parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(14);

  // Filter Toolbar Card
  auto *toolbar = new QFrame(this);
  toolbar->setObjectName("toolbarCard");
  auto *barLayout = new QHBoxLayout(toolbar);
  barLayout->setContentsMargins(14, 10, 14, 10);
  barLayout->setSpacing(12);

  filterMode = new QComboBox(toolbar);
  filterMode->addItems({"Section", "Faculty", "Room"});
  filterMode->setMinimumWidth(120);

  filterValue = new QComboBox(toolbar);
  filterValue->setMinimumWidth(200);

  auto *reloadBtn = new QPushButton("Reload", toolbar);
  statusLabel = new QLabel("Ready", toolbar);
  statusLabel->setStyleSheet("color: #908caa; font-size: 12px;");

  barLayout->addWidget(new QLabel("Filter by:", toolbar));
  barLayout->addWidget(filterMode);
  barLayout->addWidget(filterValue);
  barLayout->addStretch();
  barLayout->addWidget(statusLabel);
  barLayout->addWidget(reloadBtn);

  // Scrollable Card Grid
  scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }");

  gridContainer = new QWidget();
  gridContainer->setStyleSheet("background: transparent;");
  scrollArea->setWidget(gridContainer);

  layout->addWidget(toolbar);
  layout->addWidget(scrollArea, 1);

  connect(filterMode, &QComboBox::currentTextChanged, this,
          [this]() { populateFilterValues(); });
  connect(filterValue, &QComboBox::currentTextChanged, this,
          [this]() { renderGrid(); });
  connect(reloadBtn, &QPushButton::clicked, this, [this]() { refreshData(); });

  refreshData();
}

void TimetableGridTab::refreshData() { populateFilterValues(); }

void TimetableGridTab::populateFilterValues() {
  filterValue->blockSignals(true);
  filterValue->clear();

  try {
    std::string csvPath =
        (fs::path(paths.outputDir) / "Timetable_Output.csv").string();
    auto entries = CSVTable<TimetableEntry>::load(csvPath);
    std::set<std::string> values;
    QString mode = filterMode->currentText();

    for (const auto &e : entries) {
      if (mode == "Section")
        values.insert(e.section);
      else if (mode == "Faculty")
        values.insert(e.faculty);
      else
        values.insert(e.room);
    }
    for (const auto &v : values)
      filterValue->addItem(QString::fromStdString(v));
    statusLabel->setText(QString::number(entries.size()) + " sessions loaded");
  } catch (const std::exception &e) {
    statusLabel->setText(QString::fromStdString(e.what()));
  }

  filterValue->blockSignals(false);
  renderGrid();
}

void TimetableGridTab::renderGrid() {
  // Rebuild container cleanly
  auto *newContainer = new QWidget();
  newContainer->setStyleSheet("background: transparent;");
  auto *grid = new QGridLayout(newContainer);
  grid->setSpacing(10);
  grid->setContentsMargins(4, 4, 8, 8);

  if (filterValue->count() == 0) {
    auto *empty = new QLabel("No timetable data found. Run the engine first.",
                             newContainer);
    empty->setAlignment(Qt::AlignCenter);
    empty->setStyleSheet("color: #6e6a86; font-size: 15px; padding: 40px;");
    grid->addWidget(empty, 0, 0);
    scrollArea->setWidget(newContainer);
    gridContainer = newContainer;
    return;
  }

  try {
    std::string csvPath =
        (fs::path(paths.outputDir) / "Timetable_Output.csv").string();
    auto entries = CSVTable<TimetableEntry>::load(csvPath);

    std::map<std::string, std::string> endAt;
    for (const auto &e : entries)
      endAt[e.start] = e.end;

    std::vector<std::string> times;
    for (const auto &kv : endAt)
      times.push_back(kv.first);
    std::sort(times.begin(), times.end(),
              [](const std::string &a, const std::string &b) {
                return timeToMinutes(a) < timeToMinutes(b);
              });

    const std::vector<std::string> days = {"Monday",   "Tuesday", "Wednesday",
                                           "Thursday", "Friday",  "Saturday"};

    // Top-left corner pill
    auto *corner = new QLabel("DAY / TIME", newContainer);
    corner->setAlignment(Qt::AlignCenter);
    corner->setStyleSheet("color: #6e6a86; font-weight: 700; font-size: 11px; "
                          "letter-spacing: 1px;");
    grid->addWidget(corner, 0, 0);

    // Time column headers
    for (size_t c = 0; c < times.size(); ++c) {
      auto *hdr =
          new QLabel(QString::fromStdString(times[c] + " – " + endAt[times[c]]),
                     newContainer);
      hdr->setAlignment(Qt::AlignCenter);
      hdr->setStyleSheet(
          "background-color: #26233a; color: #9ccfd8; border-radius: 8px;"
          "padding: 8px 6px; font-weight: 700; font-size: 12px;");
      grid->addWidget(hdr, 0, static_cast<int>(c + 1));
      grid->setColumnStretch(static_cast<int>(c + 1), 1);
    }

    QString mode = filterMode->currentText();
    std::string target = filterValue->currentText().toStdString();

    for (size_t r = 0; r < days.size(); ++r) {
      auto *dayHdr = new QLabel(QString::fromStdString(days[r].substr(0, 3)),
                                newContainer);
      dayHdr->setAlignment(Qt::AlignCenter);
      dayHdr->setFixedWidth(72);
      dayHdr->setStyleSheet(
          "background-color: #26233a; color: #ebbcba; border-radius: 8px;"
          "padding: 10px; font-weight: 700; font-size: 13px;");
      grid->addWidget(dayHdr, static_cast<int>(r + 1), 0);

      for (size_t c = 0; c < times.size(); ++c) {
        std::vector<TimetableEntry> cellEntries;
        for (const auto &e : entries) {
          bool match = (mode == "Section" && e.section == target) ||
                       (mode == "Faculty" && e.faculty == target) ||
                       (mode == "Room" && e.room == target);
          if (match && e.day == days[r] && e.start == times[c]) {
            cellEntries.push_back(e);
          }
        }

        if (cellEntries.empty()) {
          auto *emptyCard = new QFrame(newContainer);
          emptyCard->setMinimumHeight(86);
          emptyCard->setStyleSheet(
              "QFrame { background-color: rgba(31, 29, 46, 0.45);"
              "border: 1px dashed #26233a; border-radius: 10px; }");
          grid->addWidget(emptyCard, static_cast<int>(r + 1),
                          static_cast<int>(c + 1));
          continue;
        }

        const auto &e = cellEntries[0];
        auto colors = colorForSubject(e.subject);
        bool isLab = (e.subject.find("Lab") != std::string::npos ||
                      e.room.find("Lab") != std::string::npos);

        auto *card = new QFrame(newContainer);
        card->setMinimumHeight(86);
        card->setStyleSheet(
            QString("QFrame { background-color: %1; border-left: 4px solid %2;"
                    "border-radius: 10px; }")
                .arg(colors.bg, colors.accent));
        addCardShadow(card);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(4);

        auto *topRow = new QHBoxLayout();
        topRow->setSpacing(6);
        auto *titleLbl = new QLabel(QString::fromStdString(e.subject), card);
        titleLbl->setWordWrap(true);
        titleLbl->setStyleSheet(
            QString("font-weight: 700; font-size: 12px; color: %1; border: "
                    "none; background: transparent;")
                .arg(colors.accent));
        topRow->addWidget(titleLbl, 1);

        if (isLab) {
          auto *badge = new QLabel("LAB", card);
          badge->setStyleSheet(
              QString(
                  "background-color: %1; color: #191724; border-radius: 4px;"
                  "padding: 2px 5px; font-size: 9px; font-weight: 800; border: "
                  "none;")
                  .arg(colors.accent));
          badge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
          topRow->addWidget(badge, 0, Qt::AlignTop);
        }

        QString metaText = QString::fromStdString(e.faculty + "  •  " + e.room);
        if (mode != "Section")
          metaText += QString::fromStdString("  •  " + e.section);
        auto *metaLbl = new QLabel(metaText, card);
        metaLbl->setWordWrap(true);
        metaLbl->setStyleSheet("font-size: 11px; color: #e0def4; border: none; "
                               "background: transparent;");

        cardLayout->addLayout(topRow);
        cardLayout->addWidget(metaLbl);
        cardLayout->addStretch();

        card->setToolTip(QString::fromStdString(
            "Subject: " + e.subject + "\nFaculty: " + e.faculty +
            "\nRoom: " + e.room + "\nSection: " + e.section +
            "\nTime: " + e.day + " " + e.start + " - " + e.end));

        grid->addWidget(card, static_cast<int>(r + 1), static_cast<int>(c + 1));
      }
    }
    grid->setRowStretch(static_cast<int>(days.size() + 1), 1);
  } catch (const std::exception &e) {
    statusLabel->setText(QString::fromStdString(e.what()));
  }

  scrollArea->setWidget(newContainer);
  gridContainer = newContainer;
}

// =========================================================
// PAGE 2: INPUT CSV EDITOR
// =========================================================
InputDataTab::InputDataTab(const AppPaths &p, QWidget *parent)
    : BaseTab(p, parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(14);

  auto *toolbar = new QFrame(this);
  toolbar->setObjectName("toolbarCard");
  auto *topBar = new QHBoxLayout(toolbar);
  topBar->setContentsMargins(14, 10, 14, 10);

  fileSelector = new QComboBox(toolbar);
  fileSelector->addItems({"Sections.csv", "Faculty.csv", "Rooms.csv",
                          "Subjects.csv", "Slots.csv", "FixedActivities.csv"});

  auto *addRowBtn = new QPushButton("+ Add Row", toolbar);
  auto *saveBtn = new QPushButton("Save Changes", toolbar);
  saveBtn->setObjectName("primaryBtn");

  topBar->addWidget(new QLabel("File:", toolbar));
  topBar->addWidget(fileSelector, 1);
  topBar->addWidget(addRowBtn);
  topBar->addWidget(saveBtn);

  table = new QTableWidget(this);
  table->setAlternatingRowColors(true);
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  layout->addWidget(toolbar);
  layout->addWidget(table, 1);

  connect(fileSelector, &QComboBox::currentTextChanged, this,
          [this]() { loadSelectedCSV(); });
  connect(addRowBtn, &QPushButton::clicked, this,
          [this]() { table->insertRow(table->rowCount()); });
  connect(saveBtn, &QPushButton::clicked, this,
          [this]() { saveSelectedCSV(); });

  loadSelectedCSV();
}

void InputDataTab::loadSelectedCSV() {
  try {
    std::string path =
        (fs::path(paths.inputDir) / fileSelector->currentText().toStdString())
            .string();
    auto rows = readRawCSV(path);
    table->clear();
    if (rows.empty())
      return;

    int cols = static_cast<int>(rows[0].size());
    table->setColumnCount(cols);
    QStringList headers;
    for (const auto &h : rows[0])
      headers << QString::fromStdString(h);
    table->setHorizontalHeaderLabels(headers);

    table->setRowCount(static_cast<int>(rows.size()) - 1);
    for (size_t r = 1; r < rows.size(); ++r) {
      for (int c = 0; c < cols; ++c) {
        std::string val =
            (c < static_cast<int>(rows[r].size())) ? rows[r][c] : "";
        table->setItem(static_cast<int>(r - 1), c,
                       new QTableWidgetItem(QString::fromStdString(val)));
      }
    }
  } catch (const std::exception &e) {
    table->clear();
    table->setRowCount(0);
    table->setColumnCount(1);
    table->setHorizontalHeaderLabels({QString::fromStdString(e.what())});
  }
}

void InputDataTab::saveSelectedCSV() {
  std::string path =
      (fs::path(paths.inputDir) / fileSelector->currentText().toStdString())
          .string();
  std::ofstream out(path);
  if (!out) {
    QMessageBox::critical(this, "Save Error",
                          "Could not write to " + QString::fromStdString(path));
    return;
  }

  int cols = table->columnCount();
  for (int c = 0; c < cols; ++c) {
    if (c > 0)
      out << ",";
    out << table->horizontalHeaderItem(c)->text().toStdString();
  }
  out << "\n";

  for (int r = 0; r < table->rowCount(); ++r) {
    bool rowEmpty = true;
    std::string line;
    for (int c = 0; c < cols; ++c) {
      if (c > 0)
        line += ",";
      auto *item = table->item(r, c);
      std::string val = item ? item->text().toStdString() : "";
      if (!val.empty())
        rowEmpty = false;
      line += val;
    }
    if (!rowEmpty)
      out << line << "\n";
  }
  QMessageBox::information(this, "Saved",
                           "Updated " + fileSelector->currentText());
}

// =========================================================
// PAGE 3: PIPELINE RUNNER
// =========================================================
PipelineTab::PipelineTab(const AppPaths &p, QWidget *parent)
    : BaseTab(p, parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(14);

  auto *toolbar = new QFrame(this);
  toolbar->setObjectName("toolbarCard");
  auto *barLayout = new QHBoxLayout(toolbar);
  barLayout->setContentsMargins(14, 12, 14, 12);

  auto *desc = new QLabel("Runs C++ Backtracking Solver  →  C Graph Conflict "
                          "Checker  →  HTML Formatter",
                          toolbar);
  desc->setStyleSheet("color: #908caa;");

  runBtn = new QPushButton("Run Full Pipeline", toolbar);
  runBtn->setObjectName("primaryBtn");
  runBtn->setMinimumHeight(38);

  barLayout->addWidget(desc, 1);
  barLayout->addWidget(runBtn);

  logConsole = new QPlainTextEdit(this);
  logConsole->setReadOnly(true);
  logConsole->setStyleSheet(
      "font-family: 'JetBrains Mono', monospace; font-size: 12px;");

  layout->addWidget(toolbar);
  layout->addWidget(logConsole, 1);

  process = new QProcess(this);
  process->setProcessChannelMode(QProcess::MergedChannels);

  connect(runBtn, &QPushButton::clicked, this, [this]() { runPipeline(); });
  connect(process, &QProcess::readyReadStandardOutput, this, [this]() {
    logConsole->appendPlainText(
        QString::fromUtf8(process->readAllStandardOutput()));
  });
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this](int code, QProcess::ExitStatus) {
            runBtn->setEnabled(true);
            logConsole->appendPlainText(
                "\n[GUI] Pipeline finished with exit code " +
                QString::number(code));
            emit pipelineFinished();
          });
}

void PipelineTab::runPipeline() {
  logConsole->clear();
  runBtn->setEnabled(false);

  QString program = QString::fromStdString(paths.backendBin);
  QStringList args = {QString::fromStdString(paths.inputDir),
                      QString::fromStdString(paths.outputDir),
                      QString::fromStdString(paths.checkerBin),
                      QString::fromStdString(paths.formatterBin)};
  logConsole->appendPlainText("[GUI] Executing: " + program + " " +
                              args.join(" ") + "\n");
  process->start(program, args);
}

// =========================================================
// PAGE 4: HTML PRINT VIEWER
// =========================================================
HtmlViewerTab::HtmlViewerTab(const AppPaths &p, QWidget *parent)
    : BaseTab(p, parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(14);

  auto *toolbar = new QFrame(this);
  toolbar->setObjectName("toolbarCard");
  auto *bar = new QHBoxLayout(toolbar);
  bar->setContentsMargins(14, 10, 14, 10);

  htmlSelector = new QComboBox(toolbar);
  auto *openBtn = new QPushButton("Open in Browser (Print / PDF)", toolbar);
  openBtn->setObjectName("primaryBtn");

  bar->addWidget(new QLabel("Generated HTML:", toolbar));
  bar->addWidget(htmlSelector, 1);
  bar->addWidget(openBtn);

  viewer = new QTextBrowser(this);
  viewer->setOpenExternalLinks(true);

  layout->addWidget(toolbar);
  layout->addWidget(viewer, 1);

  connect(htmlSelector, &QComboBox::currentTextChanged, this,
          [this]() { loadSelectedHtml(); });
  connect(openBtn, &QPushButton::clicked, this, [this]() { openInBrowser(); });

  refreshData();
}

void HtmlViewerTab::refreshData() {
  htmlSelector->blockSignals(true);
  htmlSelector->clear();

  QDir dir(QString::fromStdString(paths.outputDir));
  QStringList files = dir.entryList({"*.html"}, QDir::Files, QDir::Name);
  htmlSelector->addItems(files);

  htmlSelector->blockSignals(false);
  loadSelectedHtml();
}

void HtmlViewerTab::loadSelectedHtml() {
  if (htmlSelector->count() == 0) {
    viewer->setHtml("<h3 style='color:#908caa; padding:20px;'>No HTML "
                    "timetables found in output folder yet.</h3>");
    return;
  }
  std::string filePath =
      (fs::path(paths.outputDir) / htmlSelector->currentText().toStdString())
          .string();
  std::ifstream in(filePath);
  if (!in)
    return;
  std::ostringstream ss;
  ss << in.rdbuf();
  viewer->setHtml(QString::fromStdString(ss.str()));
}

void HtmlViewerTab::openInBrowser() {
  if (htmlSelector->count() == 0)
    return;
  QString fullPath = QString::fromStdString(
      fs::absolute(fs::path(paths.outputDir) /
                   htmlSelector->currentText().toStdString())
          .string());
  QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
}

// =========================================================
// MAIN DASHBOARD WINDOW (SIDEBAR + STACKED PAGES)
// =========================================================
MainWindow::MainWindow(const AppPaths &p, QWidget *parent)
    : QMainWindow(parent), paths(p) {
  setWindowTitle("Timetable Studio");
  resize(1240, 760);

  auto *central = new QWidget(this);
  setCentralWidget(central);

  auto *rootLayout = new QHBoxLayout(central);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  // Left Sidebar
  auto *sidebar = new QFrame(this);
  sidebar->setObjectName("sidebar");
  sidebar->setFixedWidth(220);
  auto *sideLayout = new QVBoxLayout(sidebar);
  sideLayout->setContentsMargins(14, 22, 14, 18);
  sideLayout->setSpacing(8);

  auto *brand = new QLabel("TIMETABLE\nSTUDIO", sidebar);
  brand->setStyleSheet("font-size: 18px; font-weight: 800; color: #ebbcba; "
                       "letter-spacing: 1.5px; padding: 0 8px 16px 8px;");
  sideLayout->addWidget(brand);

  navGroup = new QButtonGroup(this);
  navGroup->setExclusive(true);

  // Right Content Area
  auto *contentArea = new QWidget(this);
  auto *contentLayout = new QVBoxLayout(contentArea);
  contentLayout->setContentsMargins(24, 20, 24, 20);
  contentLayout->setSpacing(16);

  headerTitle = new QLabel("Timetable Schedule", contentArea);
  headerTitle->setStyleSheet(
      "font-size: 22px; font-weight: 800; color: #e0def4;");

  stack = new QStackedWidget(contentArea);

  auto *gridTab = new TimetableGridTab(paths, this);
  auto *inputTab = new InputDataTab(paths, this);
  auto *pipeTab = new PipelineTab(paths, this);
  auto *htmlTab = new HtmlViewerTab(paths, this);

  tabList = {gridTab, inputTab, pipeTab, htmlTab};

  for (size_t i = 0; i < tabList.size(); ++i) {
    stack->addWidget(tabList[i]);

    auto *btn = new QPushButton(tabList[i]->navLabel(), sidebar);
    btn->setObjectName("navBtn");
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    if (i == 0)
      btn->setChecked(true);

    navGroup->addButton(btn, static_cast<int>(i));
    sideLayout->addWidget(btn);
  }

  sideLayout->addStretch();
  auto *footer = new QLabel("C / C++17 • Qt 6", sidebar);
  footer->setStyleSheet("color: #6e6a86; font-size: 11px; padding-left: 8px;");
  sideLayout->addWidget(footer);

  contentLayout->addWidget(headerTitle);
  contentLayout->addWidget(stack, 1);

  rootLayout->addWidget(sidebar);
  rootLayout->addWidget(contentArea, 1);

  connect(navGroup, &QButtonGroup::idClicked, this, [this](int idx) {
    stack->setCurrentIndex(idx);
    headerTitle->setText(tabList[idx]->tabTitle());
  });

  connect(pipeTab, &PipelineTab::pipelineFinished, this,
          &MainWindow::refreshAllTabs);
}

void MainWindow::refreshAllTabs() {
  for (BaseTab *t : tabList) {
    t->refreshData();
  }
}
