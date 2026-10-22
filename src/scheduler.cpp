#include "structures.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

constexpr int DAYS = 6;
constexpr int SLOTS_PER_DAY = 8;
constexpr int TOTAL_SLOTS = DAYS * SLOTS_PER_DAY;

constexpr int LUNCH_SLOT = 4;
constexpr int ELECTIVE_SLOT = 5;
constexpr int DEFAULT_ELECTIVE_DAYS = 4;

const char *DAY_NAMES[DAYS] = {"Monday",   "Tuesday", "Wednesday",
                               "Thursday", "Friday",  "Saturday"};

const char *SLOT_TIMES[SLOTS_PER_DAY] = {
    "08:00-09:00", "09:00-10:00", "10:00-11:00", "11:00-12:00",
    "12:00-13:00", "13:00-14:00", "14:00-15:00", "15:00-16:00"};

struct SessionMeta { // session
  std::vector<int> sectionIndices;
  bool isLabPart1 = false;
  bool isLabPart2 = false;
  size_t pairedSessionId = 0;
  bool isCombinedLT = false;
};

// reading input
std::vector<std::string> parseCSVLine(const std::string &line) {
  std::vector<std::string> result;
  std::string current;
  bool inQuotes = false;
  for (char c : line) {
    if (c == '"') {
      inQuotes = !inQuotes;
    } else if (c == ',' && !inQuotes) {
      result.push_back(current);
      current.clear();
    } else if (c != '\r' && c != '\n') {
      current += c;
    }
  }
  result.push_back(current);
  return result;
}

std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

std::vector<std::string> splitList(const std::string &s,
                                   char delim = ',') { // For sections
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    std::string t = trim(item);
    if (!t.empty())
      elems.push_back(t);
  }
  return elems;
}

int main() {
  std::vector<BranchC> branches;
  std::vector<std::string> allSections;
  std::map<std::string, std::vector<int>> branchToSections;

  {
    std::ifstream file("input/Branches.csv");
    if (!file.is_open()) {
      std::cerr << "Error: Could not open input/Branches.csv\n";
      return 1;
    }
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
      if (trim(line).empty())
        continue;
      auto cols = parseCSVLine(line);
      if (cols.size() < 3)
        continue;

      BranchC b{};
      std::snprintf(b.name, sizeof(b.name), "%s", trim(cols[0]).c_str());
      b.semester = std::stoul(trim(cols[1]));
      b.numSections = std::stoul(trim(cols[2]));
      branches.push_back(b);

      for (uint32_t i = 1; i <= b.numSections; ++i) {
        std::string secName = std::string(b.name) + "-" + std::to_string(i);
        int secIdx = static_cast<int>(allSections.size());
        allSections.push_back(secName);
        branchToSections[b.name].push_back(secIdx);
      }
    }
  }

  std::vector<RoomCategoryC> roomCategories;
  std::vector<RoomC> rooms;
  int totalLabRooms = 0;

  {
    std::ifstream file("input/Rooms.csv");
    if (!file.is_open())
      file.open("input/rooms.csv");
    if (!file.is_open()) {
      std::cerr << "Error: Could not open input/Rooms.csv\n";
      return 1;
    }
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
      if (trim(line).empty())
        continue;
      auto cols = parseCSVLine(line);
      if (cols.size() < 2)
        continue;

      RoomCategoryC rc{};
      std::string rType = trim(cols[0]);
      uint32_t rCount = std::stoul(trim(cols[1]));
      std::snprintf(rc.type, sizeof(rc.type), "%s", rType.c_str());
      rc.count = rCount;
      roomCategories.push_back(rc);

      if (rType == "Lab") {
        totalLabRooms += static_cast<int>(rCount);
      }

      for (uint32_t i = 1; i <= rCount; ++i) {
        RoomC r{};
        std::snprintf(r.type, sizeof(r.type), "%s", rType.c_str());
        std::string prefix = (rType == "Classroom")        ? "CR-"
                             : (rType == "Lab")            ? "LAB-"
                             : (rType == "LectureTheater") ? "LT-"
                                                           : (rType + "-");
        std::snprintf(r.name, sizeof(r.name), "%s%u", prefix.c_str(), i);
        rooms.push_back(r);
      }
    }
  }

  std::vector<SubjectC> subjects;
  {
    std::ifstream file("input/Subjects.csv");
    if (!file.is_open()) {
      std::cerr << "Error: Could not open input/Subjects.csv\n";
      return 1;
    }
    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
      if (trim(line).empty())
        continue;
      auto cols = parseCSVLine(line);
      if (cols.size() < 6)
        continue;

      SubjectC sub{};
      std::snprintf(sub.code, sizeof(sub.code), "%s", trim(cols[0]).c_str());
      std::snprintf(sub.name, sizeof(sub.name), "%s", trim(cols[1]).c_str());
      std::snprintf(sub.branchName, sizeof(sub.branchName), "%s",
                    trim(cols[2]).c_str());
      sub.semester = std::stoul(trim(cols[3]));
      std::snprintf(sub.roomType, sizeof(sub.roomType), "%s",
                    trim(cols[4]).c_str());
      sub.sessionsPerWeek = std::stoul(trim(cols[5]));
      subjects.push_back(sub);
    }
  }

  std::string electiveLabel = "ELECTIVE";
  int electiveDaysPerWeek = DEFAULT_ELECTIVE_DAYS;
  {
    std::ifstream eFile("input/Electives.csv");
    if (eFile.is_open()) {
      std::string line;
      std::getline(eFile, line);
      std::vector<std::string> eCodes;
      int maxCount = 0;
      while (std::getline(eFile, line)) {
        if (trim(line).empty())
          continue;
        auto cols = parseCSVLine(line);
        if (!cols.empty() && !trim(cols[0]).empty()) {
          eCodes.push_back(trim(cols[0]));
          if (cols.size() >= 6) {
            try {
              int c = std::stoi(trim(cols[5]));
              if (c > maxCount)
                maxCount = c;
            } catch (...) {
            }
          }
        }
      }
      if (!eCodes.empty()) {
        electiveLabel = "ELECTIVE (";
        for (size_t i = 0; i < eCodes.size(); ++i) {
          electiveLabel += eCodes[i] + (i + 1 < eCodes.size() ? "/" : "");
        }
        electiveLabel += ")";
        if (maxCount > 0 && maxCount <= DAYS) {
          electiveDaysPerWeek = maxCount;
        }
      }
    }
  }

  // Populating Graph
  SessionGraph graph;
  init_graph(&graph, 512);
  std::vector<SessionMeta> meta;

  int numSections = static_cast<int>(allSections.size());
  std::vector<int> sectionRequiredHours(numSections, 0);
  int totalLabBlocksRequested = 0;

  for (size_t sIdx = 0; sIdx < subjects.size(); ++sIdx) {
    SubjectC *subPtr = &subjects[sIdx];
    auto targetBranches = splitList(subPtr->branchName, ',');
    std::string rType = subPtr->roomType;
    uint32_t count = subPtr->sessionsPerWeek;

    if (rType == "Lab") {
      uint32_t blocks = count / 2;
      uint32_t rem = count % 2;

      for (const auto &bName : targetBranches) {
        if (!branchToSections.count(bName))
          continue;
        for (int secId : branchToSections[bName]) {
          sectionRequiredHours[secId] += static_cast<int>(count);
          totalLabBlocksRequested += static_cast<int>(blocks);

          for (uint32_t b = 0; b < blocks; ++b) {
            size_t id1 =
                add_session(&graph, allSections[secId].c_str(), subPtr);
            size_t id2 =
                add_session(&graph, allSections[secId].c_str(), subPtr);

            SessionMeta m1, m2;
            m1.sectionIndices = {secId};
            m1.isLabPart1 = true;
            m1.pairedSessionId = id2;

            m2.sectionIndices = {secId};
            m2.isLabPart2 = true;
            m2.pairedSessionId = id1;

            meta.push_back(m1);
            meta.push_back(m2);
          }
          for (uint32_t r = 0; r < rem; ++r) {
            add_session(&graph, allSections[secId].c_str(), subPtr);
            SessionMeta m;
            m.sectionIndices = {secId};
            meta.push_back(m);
          }
        }
      }
    } else {
      // Pair sections for LTs
      for (const auto &bName : targetBranches) {
        if (!branchToSections.count(bName))
          continue;
        const auto &secList = branchToSections[bName];

        for (int secId : secList) {
          sectionRequiredHours[secId] += static_cast<int>(count);
        }

        for (size_t i = 0; i < secList.size(); i += 2) {
          if (i + 1 < secList.size()) {
            int secA = secList[i];
            int secB = secList[i + 1];
            std::string combinedName =
                allSections[secA] + "+" + allSections[secB];

            for (uint32_t k = 0; k < count; ++k) {
              add_session(&graph, combinedName.c_str(), subPtr);
              SessionMeta m;
              m.sectionIndices = {secA, secB};
              m.isCombinedLT = true;
              meta.push_back(m);
            }
          } else {
            int secA = secList[i];
            for (uint32_t k = 0; k < count; ++k) {
              add_session(&graph, allSections[secA].c_str(), subPtr);
              SessionMeta m;
              m.sectionIndices = {secA};
              m.isCombinedLT = false;
              meta.push_back(m);
            }
          }
        }
      }
    }
  }

  int maxAvailableRegularSlots = TOTAL_SLOTS - DAYS - electiveDaysPerWeek;
  for (int sec = 0; sec < numSections; ++sec) {
    if (sectionRequiredHours[sec] > maxAvailableRegularSlots) {
      std::cerr << "[WARNING] Section " << allSections[sec] << " requests "
                << sectionRequiredHours[sec]
                << " hrs/week, which exceeds the maximum "
                << maxAvailableRegularSlots
                << " regular slots available in the 08:00-16:00 grid!\n";
    }
  }

  int maxWeeklyLabBlocks = totalLabRooms * 3 * DAYS; // 3 two-hour windows/day
  if (totalLabBlocksRequested > maxWeeklyLabBlocks) {
    std::cerr << "[WARNING] Total 2-hour lab blocks requested ("
              << totalLabBlocksRequested
              << ") exceeds total Lab room capacity (" << maxWeeklyLabBlocks
              << " blocks/week)!\n";
  }

  // Conflict graph
  for (size_t i = 0; i < graph.sessionCount; ++i) {
    for (size_t j = i + 1; j < graph.sessionCount; ++j) {
      bool sharesSection = false;
      for (int sA : meta[i].sectionIndices) {
        for (int sB : meta[j].sectionIndices) {
          if (sA == sB) {
            sharesSection = true;
            break;
          }
        }
        if (sharesSection)
          break;
      }
      if (sharesSection) {
        add_conflict(&graph, i, j);
      }
    }
  }

  // Constraints
  int numRooms = static_cast<int>(rooms.size());

  std::vector<std::vector<bool>> sectionBusy(
      numSections, std::vector<bool>(TOTAL_SLOTS, false));
  std::vector<std::vector<bool>> roomBusy(
      numRooms, std::vector<bool>(TOTAL_SLOTS, false));

  // Lunch :) and elective :(
  for (int sec = 0; sec < numSections; ++sec) {
    for (int d = 0; d < DAYS; ++d) {
      sectionBusy[sec][d * SLOTS_PER_DAY + LUNCH_SLOT] = true;
      if (d < electiveDaysPerWeek) {
        sectionBusy[sec][d * SLOTS_PER_DAY + ELECTIVE_SLOT] = true;
      }
    }
  }

  std::vector<int> conflictDegree(graph.sessionCount, 0);
  for (size_t i = 0; i < graph.sessionCount; ++i) {
    Conflict *c = graph.sessions[i].conflicts;
    while (c != NULL) {
      conflictDegree[i]++;
      c = c->next;
    }
  }

  std::vector<size_t> taskOrder;
  for (size_t i = 0; i < graph.sessionCount; ++i) {
    if (!meta[i].isLabPart2) {
      taskOrder.push_back(i);
    }
  }

  // Priority order: 2-hour Labs -> Combined 2-Section LT classes ->
  // Single-Section classes
  std::sort(taskOrder.begin(), taskOrder.end(), [&](size_t a, size_t b) {
    if (meta[a].isLabPart1 != meta[b].isLabPart1)
      return meta[a].isLabPart1 > meta[b].isLabPart1;
    if (meta[a].isCombinedLT != meta[b].isCombinedLT)
      return meta[a].isCombinedLT > meta[b].isCombinedLT;
    if (conflictDegree[a] != conflictDegree[b])
      return conflictDegree[a] > conflictDegree[b];
    return a < b;
  });

  auto hasGraphConflict = [&](size_t sessId, int slot) {
    Conflict *curr = graph.sessions[sessId].conflicts;
    while (curr != NULL) {
      if (curr->target->assignedSlot == slot)
        return true;
      curr = curr->next;
    }
    return false;
  };

  auto hasSubjectOnDay = [&](size_t sessId, int day) {
    SubjectC *sub = graph.sessions[sessId].subject;
    int startSlot = day * SLOTS_PER_DAY;
    int endSlot = startSlot + SLOTS_PER_DAY;

    Conflict *curr = graph.sessions[sessId].conflicts;
    while (curr != NULL) {
      int s = curr->target->assignedSlot;
      if (s >= startSlot && s < endSlot && curr->target->subject == sub) {
        return true;
      }
      curr = curr->next;
    }
    return false;
  };

  auto findRoom = [&](size_t sessId, int slot1, int slot2,
                      bool allowFallback) -> int {
    std::string subRoomType = graph.sessions[sessId].subject->roomType;
    if (subRoomType == "Lab") {
      for (int r = 0; r < numRooms; ++r) {
        if (std::string(rooms[r].type) == "Lab" && !roomBusy[r][slot1] &&
            (slot2 == -1 || !roomBusy[r][slot2])) {
          return r;
        }
      }
      return -1;
    }

    if (meta[sessId].isCombinedLT) {
      for (int r = 0; r < numRooms; ++r) {
        if (std::string(rooms[r].type) == "LectureTheater" &&
            !roomBusy[r][slot1]) {
          return r;
        }
      }
      return -1;
    } else {
      for (int r = 0; r < numRooms; ++r) {
        if (std::string(rooms[r].type) == "Classroom" && !roomBusy[r][slot1]) {
          return r;
        }
      }
      if (allowFallback) {
        for (int r = 0; r < numRooms; ++r) {
          if (std::string(rooms[r].type) == "LectureTheater" &&
              !roomBusy[r][slot1]) {
            return r;
          }
        }
      }
      return -1;
    }
  };

  int unscheduledCount = 0;
  for (size_t sessId : taskOrder) {
    bool placed = false;

    // Order days by lightest current load for the section(s)
    std::vector<int> days = {0, 1, 2, 3, 4, 5};
    std::sort(days.begin(), days.end(), [&](int d1, int d2) {
      int load1 = 0, load2 = 0;
      for (int sec : meta[sessId].sectionIndices) {
        for (int h = 0; h < SLOTS_PER_DAY; ++h) {
          if (sectionBusy[sec][d1 * SLOTS_PER_DAY + h])
            load1++;
          if (sectionBusy[sec][d2 * SLOTS_PER_DAY + h])
            load2++;
        }
      }
      return load1 != load2 ? load1 < load2 : d1 < d2;
    });

    if (meta[sessId].isLabPart1) {
      size_t part2Id = meta[sessId].pairedSessionId;
      const int validStarts[] = {6, 0, 2};

      for (int pass = 0; pass < 2 && !placed; ++pass) {
        bool relaxSameDay = (pass == 1);
        for (int d : days) {
          if (!relaxSameDay && hasSubjectOnDay(sessId, d))
            continue;

          for (int h : validStarts) {
            int slot1 = d * SLOTS_PER_DAY + h;
            int slot2 = slot1 + 1;

            int sec = meta[sessId].sectionIndices[0];
            if (sectionBusy[sec][slot1] || sectionBusy[sec][slot2])
              continue;
            if (hasGraphConflict(sessId, slot1) ||
                hasGraphConflict(part2Id, slot2))
              continue;

            int roomIdx = findRoom(sessId, slot1, slot2, false);
            if (roomIdx != -1) {
              graph.sessions[sessId].assignedSlot = slot1;
              graph.sessions[sessId].assignedRoomIndex = roomIdx;
              graph.sessions[part2Id].assignedSlot = slot2;
              graph.sessions[part2Id].assignedRoomIndex = roomIdx;

              sectionBusy[sec][slot1] = true;
              sectionBusy[sec][slot2] = true;
              roomBusy[roomIdx][slot1] = true;
              roomBusy[roomIdx][slot2] = true;
              placed = true;
              break;
            }
          }
          if (placed)
            break;
        }
      }
    } else {
      for (int pass = 0; pass < 4 && !placed; ++pass) {
        bool allowFallback = (pass >= 1);
        bool relaxSameDay = (pass >= 2);

        for (int d : days) {
          if (!relaxSameDay && hasSubjectOnDay(sessId, d))
            continue;

          for (int h = 0; h < SLOTS_PER_DAY; ++h) {
            if (h == LUNCH_SLOT ||
                (h == ELECTIVE_SLOT && d < electiveDaysPerWeek))
              continue;
            int slot = d * SLOTS_PER_DAY + h;

            bool secConflict = false;
            for (int sec : meta[sessId].sectionIndices) {
              if (sectionBusy[sec][slot]) {
                secConflict = true;
                break;
              }
            }
            if (secConflict || hasGraphConflict(sessId, slot))
              continue;

            int roomIdx = findRoom(sessId, slot, -1, allowFallback);
            if (roomIdx != -1) {
              graph.sessions[sessId].assignedSlot = slot;
              graph.sessions[sessId].assignedRoomIndex = roomIdx;

              for (int sec : meta[sessId].sectionIndices) {
                sectionBusy[sec][slot] = true;
              }
              roomBusy[roomIdx][slot] = true;
              placed = true;
              break;
            }
          }
          if (placed)
            break;
        }
      }
    }

    if (!placed) {
      unscheduledCount += (meta[sessId].isLabPart1 ? 2 : 1);
      std::cerr << "[WARNING] Unscheduled Session: Could not place "
                << graph.sessions[sessId].subject->code << " ("
                << graph.sessions[sessId].subject->roomType << ") for section "
                << graph.sessions[sessId].sectionName
                << " within the 08:00-16:00 grid.\n";
    }
  }

  // Exporting
  fs::create_directories("output");

  for (int sec = 0; sec < numSections; ++sec) {
    std::string outPath = "output/" + allSections[sec] + ".csv";
    std::ofstream out(outPath);

    out << "Day";
    for (int h = 0; h < SLOTS_PER_DAY; ++h) {
      out << "," << SLOT_TIMES[h];
    }
    out << "\n";

    for (int d = 0; d < DAYS; ++d) {
      out << DAY_NAMES[d];
      for (int h = 0; h < SLOTS_PER_DAY; ++h) {
        if (h == LUNCH_SLOT) {
          out << ",LUNCH BREAK";
          continue;
        }
        if (h == ELECTIVE_SLOT && d < electiveDaysPerWeek) {
          out << ",\"" << electiveLabel << "\"";
          continue;
        }

        int slot = d * SLOTS_PER_DAY + h;
        std::string cell = "FREE";

        for (size_t i = 0; i < graph.sessionCount; ++i) {
          if (graph.sessions[i].assignedSlot == slot) {
            for (int sId : meta[i].sectionIndices) {
              if (sId == sec) {
                int rIdx = graph.sessions[i].assignedRoomIndex;
                std::string rName = (rIdx >= 0) ? rooms[rIdx].name : "TBA";
                cell = std::string(graph.sessions[i].subject->code) + " (" +
                       rName + ")";
                if (meta[i].sectionIndices.size() > 1) {
                  cell +=
                      " [" + std::string(graph.sessions[i].sectionName) + "]";
                }
                break;
              }
            }
          }
        }
        out << ",\"" << cell << "\"";
      }
      out << "\n";
    }
  }

  {
    std::ofstream masterOut("output/master_schedule.csv");
    masterOut << "SessionID,Section,SubjectCode,SubjectName,Day,TimeSlot,Room,"
                 "RoomType\n";
    for (size_t i = 0; i < graph.sessionCount; ++i) {
      const Session &s = graph.sessions[i];
      if (s.assignedSlot < 0)
        continue;
      int d = s.assignedSlot / SLOTS_PER_DAY;
      int h = s.assignedSlot % SLOTS_PER_DAY;
      const RoomC &rm = rooms[s.assignedRoomIndex];

      masterOut << s.id << ",\"" << s.sectionName << "\",\"" << s.subject->code
                << "\",\"" << s.subject->name << "\"," << DAY_NAMES[d] << ","
                << SLOT_TIMES[h] << "," << rm.name << "," << rm.type << "\n";
    }
  }

  std::cout << "Timetable generation complete (08:00 - 16:00 Grid, "
            << electiveDaysPerWeek << " Elective Days)!\n"
            << "Total C Graph Nodes Scheduled: "
            << (graph.sessionCount - unscheduledCount) << " / "
            << graph.sessionCount << "\n";
  if (unscheduledCount > 0) {
    std::cerr << "[WARNING] " << unscheduledCount
              << " session node(s) could not be scheduled due to slot/room "
                 "constraints.\n";
  } else {
    std::cout << "Status: 100% of sessions scheduled with zero conflicts.\n";
  }
  std::cout << "Generated " << numSections
            << " section CSVs and output/master_schedule.csv.\n";

  free_graph(&graph);
  return 0;
}
