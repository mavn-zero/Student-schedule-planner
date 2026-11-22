# Timetable Studio

A hybrid **C / C++17 / Qt 6** college timetable generation, validation, and visualization suite. It combines a C++ backtracking CSP solver, a pure C adjacency-list conflict graph validator, an HTML A3 print formatter, and a modern dark-mode Qt 6 desktop dashboard.

---

## Architecture & Pipeline

The project compiles into **4 executables** orchestrated by CMake:

1. **`timetable_backend` (`src/main.cpp`)** — Loads CSV blueprints (`Sections.csv`, `Faculty.csv`, `Rooms.csv`, `Subjects.csv`, `Slots.csv`, `FixedActivities.csv`), runs a backtracking solver to assign conflict-free slots and rooms, writes `output/Timetable_Output.csv`, and invokes the validator and formatter.
2. **`conflict_check` (`src/conflict_check.c`)** — Pure C DSA validator that builds an undirected adjacency-list conflict graph from `Timetable_Output.csv`, checks for section, faculty, and room overlaps using bitmask flags, and reports vertex degrees.
3. **`timetable_formatter` (`src/timetable_formatter.cpp`)** — Generates print-ready, A3 landscape HTML timetables (`*_College_Timetable.html`) with subject/faculty legend tables for every section.
4. **`timetable_gui` (`src/gui/`)** — Qt 6 desktop dashboard (**Timetable Studio**) with a left navigation sidebar and 4 views:
   - **Schedule:** Custom `QScrollArea` + `QGridLayout` card grid with deterministic subject color-coding, `[LAB]` badges, hover tooltips, and Section/Faculty/Room filtering.
   - **Input CSVs:** Built-in spreadsheet editor for modifying and saving input CSV files.
   - **Run Engine:** Executes the full C++/C pipeline asynchronously via `QProcess` and streams live logs.
   - **Print View:** Renders generated HTML timetables in-app with one-click browser export for PDF printing.

---

## C++ OOP Concepts Demonstrated

- **Abstraction & Inheritance:** Abstract base classes `AcademicEntity` (`Models.h`) and `BaseTab` (`MainWindow.h`) define pure virtual interfaces (`= 0`) inherited by concrete entities and UI pages.
- **Runtime Polymorphism:** `MainWindow` stores pages as `std::vector<BaseTab*>` and dynamically dispatches `tab->refreshData()` across all views when the pipeline finishes.
- **Encapsulation:** Entity fields and UI states are kept `private`/`protected` behind controlled accessors.
- **Templates:** Generic `CSVTable<T>` template parses any CSV file into typed C++ structs.
- **Operator Overloading:** `operator<<` is overloaded on `AcademicEntity` to stream formatted summaries.
- **Exception Handling:** Custom `GuiException` (inheriting from `std::runtime_error`) handles file I/O and malformed CSV errors cleanly.
- **Observer Pattern:** Qt Signals & Slots connect UI actions and asynchronous `QProcess` output without blocking the main thread.

---

## Project Structure

```text
.
├── CMakeLists.txt
├── flake.nix
├── input/
│   ├── Faculty.csv
│   ├── FixedActivities.csv
│   ├── Rooms.csv
│   ├── Sections.csv
│   ├── Slots.csv
│   └── Subjects.csv
├── output/
│   ├── Timetable_Output.csv
│   └── *_College_Timetable.html
└── src/
    ├── conflict_check.c
    ├── main.cpp
    ├── timetable_formatter.cpp
    └── gui/
        ├── gui_main.cpp
        ├── MainWindow.cpp
        ├── MainWindow.h
        └── Models.h
