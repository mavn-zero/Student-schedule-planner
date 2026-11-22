#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
namespace fs = std::filesystem;

struct Section {
  string id, name, program;
  int studentCount, semester;
};
struct Faculty {
  string id, name, department;
  int maxSessions;
};
struct Room {
  string id, name, type;
  int capacity;
};
struct Subject {
  string id, name, sectionName, facultyName, roomType;
  int sessionsPerWeek;
};
struct FixedActivity {
  string day, start, end, activity, sectionName;
};
struct Slot {
  string day, start, end;
  int dayIndex, periodIndex;
};
struct Lesson {
  string sectionName, subjectName, facultyName, requiredRoomType;
  int sectionIndex, facultyIndex, subjectIndex;
  int assignedSlot, assignedRoom;
};

static vector<string> splitCSV(const string &line) {
  // Simple CSV parser. Input cells must not contain commas or quoted commas.
  vector<string> result;
  string cell;
  stringstream ss(line);
  while (getline(ss, cell, ','))
    result.push_back(cell);
  if (!line.empty() && line.back() == ',')
    result.push_back("");
  return result;
}
static vector<vector<string>> readCSV(const string &path) {
  ifstream in(path);
  if (!in)
    throw runtime_error("Cannot open file: " + path);
  vector<vector<string>> rows;
  string line;
  while (getline(in, line)) {
    if (!line.empty() && line.back() == char(13))
      line.pop_back();
    rows.push_back(splitCSV(line));
  }
  return rows;
}
static int toInt(const string &s, int fallback = 0) {
  try {
    return stoi(s);
  } catch (...) {
    return fallback;
  }
}
static string getCell(const vector<string> &r, size_t i) {
  return i < r.size() ? r[i] : "";
}
static int findHeader(const vector<string> &header, const string &key) {
  for (int i = 0; i < (int)header.size(); ++i)
    if (header[i] == key)
      return i;
  return -1;
}
static string cellByName(const vector<string> &row,
                         const vector<string> &header, const string &key) {
  int i = findHeader(header, key);
  if (i < 0)
    return "";
  return getCell(row, (size_t)i);
}
static vector<vector<string>> readTable(const string &path,
                                        const vector<string> &required) {
  auto rows = readCSV(path);
  if (rows.empty())
    throw runtime_error("Empty CSV: " + path);
  for (const auto &col : required)
    if (findHeader(rows[0], col) < 0)
      throw runtime_error(path + " missing column: " + col);
  return rows;
}
static const string DAYS[6] = {"Monday",   "Tuesday", "Wednesday",
                               "Thursday", "Friday",  "Saturday"};

class TimetableManager {
private:
  vector<Section> sections;
  vector<Faculty> faculty;
  vector<Room> rooms;
  vector<Subject> subjects;
  vector<FixedActivity> fixedActivities;
  vector<Slot> slots;
  vector<Lesson> lessons;

  // Timetable rules: max 6 periods/day; input slot list decides actual periods.
  // occupancy: [resource][slot]
  vector<vector<int>> sectionBusy, facultyBusy, roomBusy;
  vector<vector<string>> sectionFixed;

  int findSection(const string &name) const {
    for (int i = 0; i < (int)sections.size(); ++i)
      if (sections[i].name == name)
        return i;
    return -1;
  }
  int findFaculty(const string &name) const {
    for (int i = 0; i < (int)faculty.size(); ++i)
      if (faculty[i].name == name)
        return i;
    return -1;
  }
  bool fixedForSection(int sec, int slot) const {
    for (const auto &a : fixedActivities) {
      if (a.day == slots[slot].day &&
          (a.sectionName == "ALL" || a.sectionName == sections[sec].name) &&
          a.start == slots[slot].start)
        return true;
    }
    return false;
  }
  bool roomMatches(const Room &r, const string &need, int students) const {
    bool typeOk = (need.empty() || need == "Any" || r.type == need);
    return typeOk && r.capacity >= students;
  }
  bool solve(int task) {
    if (task == (int)lessons.size())
      return true;
    Lesson &l = lessons[task];
    const Section &sec = sections[l.sectionIndex];
    const Subject &sub = subjects[l.subjectIndex];

    // Try the most constrained/earliest slots first.
    for (int sl = 0; sl < (int)slots.size(); ++sl) {
      if (sectionBusy[l.sectionIndex][sl] || facultyBusy[l.facultyIndex][sl])
        continue;
      if (fixedForSection(l.sectionIndex, sl))
        continue;

      // A fixed activity marked ALL blocks all sections; the activity slot
      // is omitted from generated class slots when represented in Slots.csv.
      for (int rm = 0; rm < (int)rooms.size(); ++rm) {
        if (roomBusy[rm][sl])
          continue;
        if (!roomMatches(rooms[rm], sub.roomType, sec.studentCount))
          continue;

        sectionBusy[l.sectionIndex][sl] = 1;
        facultyBusy[l.facultyIndex][sl] = 1;
        roomBusy[rm][sl] = 1;
        l.assignedSlot = sl;
        l.assignedRoom = rm;
        if (solve(task + 1))
          return true;
        sectionBusy[l.sectionIndex][sl] = 0;
        facultyBusy[l.facultyIndex][sl] = 0;
        roomBusy[rm][sl] = 0;
        l.assignedSlot = -1;
        l.assignedRoom = -1;
      }
    }
    return false;
  }

public:
  void load(const string &inputDir) {
    auto s = readTable(
        (fs::path(inputDir) / "Sections.csv").string(),
        {"SectionID", "SectionName", "StudentCount", "Program", "Semester"});
    for (size_t i = 1; i < s.size(); ++i)
      if (s[i].size() > 1)
        sections.push_back({getCell(s[i], 0), getCell(s[i], 1),
                            getCell(s[i], 3), toInt(getCell(s[i], 2)),
                            toInt(getCell(s[i], 4))});

    auto f = readTable(
        (fs::path(inputDir) / "Faculty.csv").string(),
        {"FacultyID", "FacultyName", "Department", "MaxSessionsPerWeek"});
    for (size_t i = 1; i < f.size(); ++i)
      if (f[i].size() > 1)
        faculty.push_back({getCell(f[i], 0), getCell(f[i], 1), getCell(f[i], 2),
                           toInt(getCell(f[i], 3), 30)});

    auto r = readTable((fs::path(inputDir) / "Rooms.csv").string(),
                       {"RoomID", "RoomName", "RoomType", "Capacity"});
    for (size_t i = 1; i < r.size(); ++i)
      if (r[i].size() > 1)
        rooms.push_back({getCell(r[i], 0), getCell(r[i], 1), getCell(r[i], 2),
                         toInt(getCell(r[i], 3))});

    auto u = readTable((fs::path(inputDir) / "Subjects.csv").string(),
                       {"SubjectID", "SubjectName", "SectionName",
                        "FacultyName", "SessionsPerWeek", "RoomTypeRequired"});
    for (size_t i = 1; i < u.size(); ++i)
      if (u[i].size() > 1)
        subjects.push_back({getCell(u[i], 0), getCell(u[i], 1),
                            getCell(u[i], 2), getCell(u[i], 3),
                            getCell(u[i], 5), toInt(getCell(u[i], 4))});

    // Slots.csv defines available class periods only. Lunch/break/activity
    // periods are intentionally not included as teaching slots.
    auto t = readTable((fs::path(inputDir) / "Slots.csv").string(),
                       {"Day", "Start", "End", "Period"});
    for (size_t i = 1; i < t.size(); ++i) {
      int d = -1;
      for (int k = 0; k < 6; k++)
        if (getCell(t[i], 0) == DAYS[k])
          d = k;
      if (d >= 0)
        slots.push_back({getCell(t[i], 0), getCell(t[i], 1), getCell(t[i], 2),
                         d, toInt(getCell(t[i], 3))});
    }

    fs::path fixedPath = fs::path(inputDir) / "FixedActivities.csv";
    if (fs::exists(fixedPath)) {
      auto a = readTable(fixedPath.string(),
                         {"Day", "Start", "End", "Activity", "SectionName"});
      for (size_t i = 1; i < a.size(); ++i)
        fixedActivities.push_back({getCell(a[i], 0), getCell(a[i], 1),
                                   getCell(a[i], 2), getCell(a[i], 3),
                                   getCell(a[i], 4)});
    }
    if (sections.empty() || faculty.empty() || rooms.empty() ||
        subjects.empty() || slots.empty())
      throw runtime_error("Sections, Faculty, Rooms, Subjects and Slots must "
                          "contain data rows.");

    for (const auto &sub : subjects) {
      if (findSection(sub.sectionName) < 0)
        throw runtime_error("Unknown section in Subjects.csv: " +
                            sub.sectionName);
      if (findFaculty(sub.facultyName) < 0)
        throw runtime_error("Unknown faculty in Subjects.csv: " +
                            sub.facultyName);
      if (sub.sessionsPerWeek < 0)
        throw runtime_error("Negative SessionsPerWeek for " + sub.name);
    }
    for (int i = 0; i < (int)sections.size(); i++)
      sectionBusy.push_back(vector<int>(slots.size(), 0));
    for (int i = 0; i < (int)faculty.size(); i++)
      facultyBusy.push_back(vector<int>(slots.size(), 0));
    for (int i = 0; i < (int)rooms.size(); i++)
      roomBusy.push_back(vector<int>(slots.size(), 0));
  }

  void generate(const string &outputDir) {
    lessons.clear();
    for (int si = 0; si < (int)sections.size(); ++si) {
      for (int ui = 0; ui < (int)subjects.size(); ++ui)
        if (subjects[ui].sectionName == sections[si].name) {
          int fi = findFaculty(subjects[ui].facultyName);
          for (int n = 0; n < subjects[ui].sessionsPerWeek; n++)
            lessons.push_back({sections[si].name, subjects[ui].name,
                               subjects[ui].facultyName, subjects[ui].roomType,
                               si, fi, ui, -1, -1});
        }
    }
    if (lessons.empty())
      throw runtime_error("No class sessions were requested in Subjects.csv.");
    // Basic backtracking. For large college datasets this may take a long time;
    // the first valid timetable is returned.
    if (!solve(0))
      throw runtime_error("No valid timetable found. Check slots, room "
                          "capacity/type, faculty load and fixed activities.");
    fs::create_directories(outputDir);
    ofstream out(fs::path(outputDir) / "Timetable_Output.csv");
    out << "Day,Start,End,Section,Subject,Faculty,Room,Period\n";
    for (const auto &l : lessons) {
      const Slot &sl = slots[l.assignedSlot];
      const Room &rm = rooms[l.assignedRoom];
      out << sl.day << "," << sl.start << "," << sl.end << "," << l.sectionName
          << "," << l.subjectName << "," << l.facultyName << "," << rm.name
          << "," << sl.periodIndex << "\n";
    }
    cout << "Generated " << lessons.size() << " class sessions.\n";
  }
};

int main(int argc, char *argv[]) {
  if (argc < 5) {
    cout << "Usage:\n";
    cout << "timetable_backend <input_folder> "
         << "<output_folder> "
         << "<path_to_c_checker> "
         << "<path_to_formatter>\n";
    return 2;
  }
  try {
    TimetableManager manager;
    string inputDir = argv[1];
    string outputDir = argv[2];
    string checker = argv[3];
    string formatter = argv[4];
    // STEP 1: Generate timetable
    cout << "\nSTEP 1: Generating timetable...\n";
    manager.load(inputDir);
    manager.generate(outputDir);
    string csvPath = (fs::path(outputDir) / "Timetable_Output.csv").string();
    // STEP 2: Run C graph conflict checker
    cout << "\nSTEP 2: Checking timetable conflicts...\n";
#ifdef _WIN32
    string checkCommand = "cmd /c \"\"" + checker + "\" \"" + csvPath + "\"\"";
#else
    string checkCommand = "\"" + checker + "\" \"" + csvPath + "\"";
#endif
    int checkResult = system(checkCommand.c_str());
    if (checkResult != 0) {
      cerr << "\nConflict checking failed. "
           << "HTML generation stopped.\n";
      return 1;
    }
    // STEP 3: Generate HTML timetable
    cout << "\nSTEP 3: Generating HTML timetable...\n";
    string subjectsPath = (fs::path(inputDir) / "Subjects.csv").string();
#ifdef _WIN32
    string formatCommand = "cmd /c \"\"" + formatter + "\" \"" + csvPath +
                           "\" \"" + outputDir + "\" \"" + subjectsPath +
                           "\"\"";
#else
    string formatCommand = "\"" + formatter + "\" \"" + csvPath + "\" \"" +
                           outputDir + "\" \"" + subjectsPath + "\"";
#endif
    int formatResult = system(formatCommand.c_str());
    if (formatResult != 0) {
      cerr << "\nHTML formatting failed.\n";
      return 1;
    }
    cout << "\n================================\n";
    cout << "PROJECT EXECUTION COMPLETED!\n";
    cout << "1. Timetable generated.\n";
    cout << "2. Graph conflict checking completed.\n";
    cout << "3. HTML timetable generated.\n";
    cout << "================================\n";
  } catch (const exception &e) {
    cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
