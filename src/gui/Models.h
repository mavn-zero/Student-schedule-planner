#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class GuiException : public std::runtime_error {
public:
  explicit GuiException(const std::string &msg) : std::runtime_error(msg) {}
};

// Quote-aware CSV utilities
inline std::vector<std::string> parseCSVLine(const std::string &line) {
  std::vector<std::string> cells;
  std::string cell;
  bool inQuotes = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '"') {
      if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
        cell += '"';
        ++i;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (c == ',' && !inQuotes) {
      cells.push_back(cell);
      cell.clear();
    } else {
      cell += c;
    }
  }
  cells.push_back(cell);
  return cells;
}

inline std::vector<std::vector<std::string>>
readRawCSV(const std::string &path) {
  std::ifstream in(path);
  if (!in)
    throw GuiException("Cannot open file: " + path);
  std::vector<std::vector<std::string>> rows;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (!line.empty())
      rows.push_back(parseCSVLine(line));
  }
  return rows;
}

// Abstract Base Class (Abstraction + Polymorphism + Operator Overloading)
class AcademicEntity {
protected:
  std::string id;
  std::string name;

public:
  AcademicEntity(std::string id, std::string name)
      : id(std::move(id)), name(std::move(name)) {}
  virtual ~AcademicEntity() = default;

  virtual std::string formatSummary() const = 0;

  friend std::ostream &operator<<(std::ostream &os, const AcademicEntity &e) {
    return os << e.formatSummary();
  }
};

class SectionEntity : public AcademicEntity {
  int studentCount;
  std::string program;
  int semester;

public:
  SectionEntity(std::string id, std::string name, int count, std::string prog,
                int sem)
      : AcademicEntity(std::move(id), std::move(name)), studentCount(count),
        program(std::move(prog)), semester(sem) {}

  std::string formatSummary() const override {
    return id + " | " + name + " (" + program + ", Sem " +
           std::to_string(semester) + ")";
  }

  static SectionEntity fromRow(const std::vector<std::string> &r) {
    return SectionEntity(r[0], r[1], std::stoi(r[2]), r[3], std::stoi(r[4]));
  }
};

// Represents one row of Timetable_Output.csv
struct TimetableEntry {
  std::string day, start, end, section, subject, faculty, room, period;

  static TimetableEntry fromRow(const std::vector<std::string> &r) {
    if (r.size() < 8)
      throw GuiException("Malformed Timetable_Output.csv row.");
    return {r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]};
  }
};

// Generic Template Loader
template <typename T> class CSVTable {
public:
  static std::vector<T> load(const std::string &path) {
    auto rows = readRawCSV(path);
    std::vector<T> items;
    for (size_t i = 1; i < rows.size(); ++i) {
      if (rows[i].size() >= 4)
        items.push_back(T::fromRow(rows[i]));
    }
    return items;
  }
};
