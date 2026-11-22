
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <cstdio>
#include <stdexcept>

using namespace std;
namespace fs = std::filesystem;

struct Entry {
    string day, start, end, section;
    string subject, faculty, room, period;
};

struct SubjectInfo {
    string code, name, faculty;
};

// Remove extra spaces
string trim(string s) {
    size_t first = s.find_first_not_of(" \r\n\t");

    if (first == string::npos)
        return "";

    size_t last = s.find_last_not_of(" \r\n\t");

    return s.substr(first, last - first + 1);
}

// Read CSV including quoted fields
vector<string> parseCSV(const string& line) {
    vector<string> cells;
    string cell;
    bool quoted = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];

        if (c == '"') {
            if (quoted && i + 1 < line.size()
                && line[i + 1] == '"') {
                cell += '"';
                i++;
            }
            else {
                quoted = !quoted;
            }
        }
        else if (c == ',' && !quoted) {
            cells.push_back(trim(cell));
            cell.clear();
        }
        else {
            cell += c;
        }
    }

    cells.push_back(trim(cell));

    return cells;
}

// HTML special character handling
string escapeHTML(const string& s) {
    string result;

    for (char c : s) {
        if (c == '&')
            result += "&amp;";
        else if (c == '<')
            result += "&lt;";
        else if (c == '>')
            result += "&gt;";
        else if (c == '"')
            result += "&quot;";
        else
            result += c;
    }

    return result;
}

// Safe filename
string safeName(string s) {
    for (char& c : s) {
        if (!isalnum((unsigned char)c) &&
            c != '-' && c != '_') {
            c = '_';
        }
    }

    return s;
}

// Convert time into minutes for sorting
int toMinutes(const string& time) {
    int h, m;

    if (sscanf(time.c_str(), "%d:%d", &h, &m) != 2)
        return -1;

    return h * 60 + m;
}

// Read generated timetable CSV
vector<Entry> readTimetable(const string& path) {
    ifstream in(path);

    if (!in)
        throw runtime_error(
            "Cannot open timetable CSV: " + path
        );

    string line;

    if (!getline(in, line))
        throw runtime_error("Timetable CSV is empty.");

    vector<Entry> rows;

    while (getline(in, line)) {
        if (trim(line).empty())
            continue;

        vector<string> c = parseCSV(line);

        if (c.size() < 8)
            continue;

        rows.push_back({
            c[0], c[1], c[2], c[3],
            c[4], c[5], c[6], c[7]
        });
    }

    return rows;
}

// Read subject code, subject name and faculty
vector<SubjectInfo> readSubjects(
    const string& path,
    const string& section
) {
    ifstream in(path);

    if (!in)
        throw runtime_error(
            "Cannot open Subjects.csv: " + path
        );

    string line;

    if (!getline(in, line))
        throw runtime_error("Subjects.csv is empty.");

    vector<string> header = parseCSV(line);

    map<string, int> col;

    for (int i = 0; i < (int)header.size(); i++) {
        col[header[i]] = i;
    }

    if (!col.count("SubjectID") ||
        !col.count("SubjectName") ||
        !col.count("SectionName") ||
        !col.count("FacultyName")) {

        throw runtime_error(
            "Subjects.csv must contain SubjectID, "
            "SubjectName, SectionName and FacultyName."
        );
    }

    vector<SubjectInfo> subjects;
    set<string> used;

    while (getline(in, line)) {
        if (trim(line).empty())
            continue;

        vector<string> c = parseCSV(line);

        auto get = [&](const string& key) -> string {
            int i = col[key];

            if (i < (int)c.size())
                return c[i];

            return "";
        };

        if (get("SectionName") != section)
            continue;

        string code = get("SubjectID");
        string name = get("SubjectName");
        string faculty = get("FacultyName");

        string key = code + "|" + name + "|" + faculty;

        if (used.insert(key).second) {
            subjects.push_back({
                code, name, faculty
            });
        }
    }

    return subjects;
}

int main(int argc, char* argv[]) {

    if (argc < 4) {
        cout << "Usage:\n";

        cout << "timetable_formatter "
             << "Timetable_Output.csv "
             << "output_folder "
             << "Subjects.csv "
             << "[section] [academic_year] [effective_date]\n";

        return 1;
    }

    try {

        // Read timetable
        vector<Entry> rows = readTimetable(argv[1]);

        fs::path output = argv[2];

        fs::create_directories(output);

        string subjectFile = argv[3];

        string sectionArg =
            argc > 4 ? argv[4] : "";

        string year =
            argc > 5 ? argv[5] : "2026-27";

        string effective =
            argc > 6 ? argv[6] : "13 July 2026";

        vector<string> days = {
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday"
        };

        set<string> sectionSet;

        map<string, string> endAt;

        // Collect sections and time slots
        for (const auto& r : rows) {
            sectionSet.insert(r.section);
            endAt[r.start] = r.end;
        }

        vector<string> times;

        for (const auto& item : endAt) {
            times.push_back(item.first);
        }

        // Sort time slots
        sort(times.begin(), times.end(),
            [](const string& a, const string& b) {
                return toMinutes(a) < toMinutes(b);
            }
        );

        int generated = 0;

        // Generate timetable for each section
        for (const string& section : sectionSet) {

            if (!sectionArg.empty() &&
                section != sectionArg) {
                continue;
            }

            vector<SubjectInfo> subjects =
                readSubjects(subjectFile, section);

            string filename =
                safeName(section) +
                "_College_Timetable.html";

            ofstream h(output / filename);

            if (!h)
                throw runtime_error(
                    "Cannot create HTML file."
                );

            // HTML header and CSS
            h << R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">

<title>College Timetable</title>

<style>

@page {
    size: A3 landscape;
    margin: 8mm;
}

body {
    font-family: "Times New Roman", serif;
    color: black;
    background: white;
    margin: 10px;
}

h2, h3, p {
    text-align: center;
    margin: 3px;
}

table {
    border-collapse: collapse;
    width: 100%;
    table-layout: fixed;
    margin-top: 8px;
}

th, td {
    border: 1px solid black;
    text-align: center;
    vertical-align: middle;
    padding: 4px;
    overflow-wrap: anywhere;
    font-size: 11px;
}

th {
    font-weight: bold;
}

.day {
    width: 6%;
    font-weight: bold;
}

thead th {
    height: 35px;
}

tbody td {
    height: 65px;
}

.heading {
    font-size: 17px;
    font-weight: bold;
}

.subheading {
    font-size: 14px;
    font-weight: bold;
}

.class-code {
    font-weight: bold;
}

.subject-table {
    width: 85%;
    margin: 15px auto;
}

.subject-table th,
.subject-table td {
    text-align: left;
    padding: 4px 6px;
    font-size: 11px;
}

.subject-table th:first-child {
    width: 18%;
}

.subject-table th:nth-child(2) {
    width: 52%;
}

.subject-table th:nth-child(3) {
    width: 30%;
}

@media print {
    body {
        margin: 0;
    }

    table {
        page-break-inside: avoid;
    }
}

</style>
</head>

<body>
)";

            // University heading
            h << "<h2>GRAPHIC ERA DEEMED TO BE UNIVERSITY</h2>";

            h << "<h3 class='heading'>"
              << "TIME TABLE OF B.TECH. III SEMESTER - "
              << "COMPUTER SCIENCE &amp; ENGINEERING "
              << "(Section: "
              << escapeHTML(section)
              << ")</h3>";

            h << "<h3>ACADEMIC YEAR "
              << escapeHTML(year)
              << "</h3>";

            h << "<p class='subheading'>W.E.F: "
              << escapeHTML(effective)
              << "</p>";

            // MAIN TIMETABLE FIRST
            h << "<table>";

            h << "<thead><tr>";

            h << "<th class='day'>DAY</th>";

            for (const string& t : times) {
                h << "<th>"
                  << escapeHTML(t)
                  << "-"
                  << escapeHTML(endAt[t])
                  << "</th>";
            }

            h << "</tr></thead><tbody>";

            // Monday to Saturday
            for (const string& day : days) {

                h << "<tr>";

                h << "<td class='day'>"
                  << day.substr(0, 3)
                  << "</td>";

                for (const string& t : times) {

                    h << "<td>";

                    bool first = true;

                    for (const auto& r : rows) {

                        if (r.section == section &&
                            r.day == day &&
                            r.start == t) {

                            if (!first)
                                h << "<hr>";

                            first = false;

                            // Find subject code
                            string code = "";

                            for (const auto& s : subjects) {
                                if (s.name == r.subject) {
                                    code = s.code;
                                    break;
                                }
                            }

                            if (!code.empty()) {
                                h << "<span class='class-code'>"
                                  << escapeHTML(code)
                                  << "</span><br>";
                            }

                            h << escapeHTML(r.subject)
                              << "<br>";

                            if (!r.faculty.empty()) {
                                h << escapeHTML(r.faculty)
                                  << "<br>";
                            }

                            if (!r.room.empty()) {
                                h << escapeHTML(r.room);
                            }
                        }
                    }

                    h << "</td>";
                }

                h << "</tr>";
            }

            h << "</tbody></table>";

            // SUBJECT DETAILS TABLE AFTER MAIN TIMETABLE
            h << "<table class='subject-table'>";

            h << "<thead><tr>"
              << "<th>Subject Code</th>"
              << "<th>Subject Name</th>"
              << "<th>Faculty</th>"
              << "</tr></thead><tbody>";

            for (const auto& s : subjects) {

                h << "<tr><td>"
                  << escapeHTML(s.code)
                  << "</td><td>"
                  << escapeHTML(s.name)
                  << "</td><td>"
                  << escapeHTML(s.faculty)
                  << "</td></tr>";
            }

            h << "</tbody></table>";

            // Footer
            h << "<p style='margin-top:8px;font-size:10px'>"
              << "Generated College Timetable"
              << "</p>";

            h << "</body></html>";

            generated++;
        }

        if (generated == 0) {
            throw runtime_error(
                "No matching section found."
            );
        }

        cout << "Successfully generated "
             << generated
             << " HTML timetable(s).\n";

        cout << "Open HTML in browser and press "
             << "Ctrl + P to save as PDF.\n";

    }
    catch (const exception& e) {

        cerr << "ERROR: "
             << e.what()
             << "\n";

        return 1;
    }

    return 0;
}
