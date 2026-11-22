#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_ROWS 10000
#define LINE_SIZE 2048
typedef struct {
    char day[32];
    char start[32];
    char end[32];
    char section[128];
    char subject[128];
    char faculty[128];
    char room[128];
    char period[16];
} TimetableRow;
// Graph edge node
typedef struct Edge {
    int vertex;
    unsigned char types;
    struct Edge *next;
} Edge;
// Conflict types
#define SECTION_CLASH 1
#define FACULTY_CLASH 2
#define ROOM_CLASH 4
// Copy string safely
void copy_field(char *dst, size_t size, const char *src) {
    if (src == NULL) src = "";
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}
// Convert time into minutes
int to_minutes(const char *time) {
    int hour, minute;
    char extra;
    if (sscanf(time, "%d:%d%c", &hour, &minute, &extra) != 2)
        return -1;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
        return -1;

    return hour * 60 + minute;
}
// Check time overlap
int time_overlaps(const TimetableRow *a, const TimetableRow *b) {
    int startA = to_minutes(a->start);
    int endA = to_minutes(a->end);
    int startB = to_minutes(b->start);
    int endB = to_minutes(b->end);

    if (startA < 0 || endA < 0 || startB < 0 || endB < 0)
        return 0;

    if (endA <= startA || endB <= startB)
        return 0;

    return startA < endB && startB < endA;
}
// Add an edge to adjacency list
void add_edge(Edge **graph, int u, int v, unsigned char types) {
    Edge *newNode = malloc(sizeof(Edge));

    if (newNode == NULL) {
        fprintf(stderr, "Graph memory allocation failed.\n");
        exit(2);
    }
    newNode->vertex = v;
    newNode->types = types;
    newNode->next = graph[u];
    graph[u] = newNode;
    Edge *reverseNode = malloc(sizeof(Edge));
    if (reverseNode == NULL) {
        fprintf(stderr, "Graph memory allocation failed.\n");
        exit(2);
    }
    reverseNode->vertex = u;
    reverseNode->types = types;
    reverseNode->next = graph[v];
    graph[v] = reverseNode;
}
// Free graph memory
void free_graph(Edge **graph, int n) {
    for (int i = 0; i < n; i++) {
        Edge *current = graph[i];
        while (current != NULL) {
            Edge *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(graph);
}
// Display conflict type
void display_conflict(unsigned char types,
                      const TimetableRow *row) {
    if (types & SECTION_CLASH) {
        printf("SECTION CLASH: %s on %s, %s-%s\n",
               row->section, row->day,
               row->start, row->end);
    }
    if (types & FACULTY_CLASH) {
        printf("FACULTY CLASH: %s on %s, %s-%s\n",
               row->faculty, row->day,
               row->start, row->end);
    }
    if (types & ROOM_CLASH) {
        printf("ROOM CLASH: %s on %s, %s-%s\n",
               row->room, row->day,
               row->start, row->end);
    }
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage: conflict_check.exe <Timetable_Output.csv>\n");
        return 2;
    }
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("Cannot open timetable CSV");
        return 2;
    }
    TimetableRow *rows =
        malloc(MAX_ROWS * sizeof(TimetableRow));
    if (rows == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(fp);
        return 2;
    }
    char line[LINE_SIZE];
    int n = 0;
    int lineNumber = 0;
    // Skip CSV header
    if (fgets(line, sizeof(line), fp) == NULL) {
        fprintf(stderr, "Empty timetable file.\n");
        free(rows);
        fclose(fp);
        return 2;
    }
    lineNumber++;
    // Read timetable records
    while (fgets(line, sizeof(line), fp) != NULL) {
        lineNumber++;
        if (n >= MAX_ROWS) {
            fprintf(stderr, "Maximum of %d sessions exceeded.\n",
                    MAX_ROWS);
            free(rows);
            fclose(fp);
            return 2;
        }
        char *fields[8];
        int fieldCount = 0;
        fields[fieldCount++] = strtok(line, ",\r\n");
        while (fieldCount < 8 &&
               (fields[fieldCount] = strtok(NULL, ",\r\n")) != NULL) {
            fieldCount++;
        }
        if (fieldCount != 8) {
            fprintf(stderr, "Invalid CSV row at line %d.\n",
                    lineNumber);
            free(rows);
            fclose(fp);
            return 2;
        }
        char *destinations[8] = {
            rows[n].day,
            rows[n].start,
            rows[n].end,
            rows[n].section,
            rows[n].subject,
            rows[n].faculty,
            rows[n].room,
            rows[n].period
        };
        size_t sizes[8] = {
            sizeof(rows[n].day),
            sizeof(rows[n].start),
            sizeof(rows[n].end),
            sizeof(rows[n].section),
            sizeof(rows[n].subject),
            sizeof(rows[n].faculty),
            sizeof(rows[n].room),
            sizeof(rows[n].period)
        };
        for (int k = 0; k < 8; k++) {

            if (strlen(fields[k]) >= sizes[k]) {
                fprintf(stderr,
                        "Field too long at CSV line %d.\n",
                        lineNumber);
                free(rows);
                fclose(fp);
                return 2;
            }
            copy_field(destinations[k], sizes[k], fields[k]);
        }
        int start = to_minutes(rows[n].start);
        int end = to_minutes(rows[n].end);
        if (start < 0 || end < 0 || end <= start) {
            fprintf(stderr, "Invalid time at CSV line %d.\n",
                    lineNumber);
            free(rows);
            fclose(fp);
            return 2;
        }
        n++;
    }
    fclose(fp);
    if (n == 0) {
        fprintf(stderr, "No timetable sessions found.\n");
        free(rows);
        return 2;
    }
    // Create graph with n vertices
    Edge **graph = calloc(n, sizeof(Edge *));
    if (graph == NULL) {
        fprintf(stderr, "Graph allocation failed.\n");
        free(rows);
        return 2;
    }
    int conflicts = 0;
    int edgeCount = 0;
    // Build undirected conflict graph
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (strcmp(rows[i].day, rows[j].day) != 0)
                continue;
            if (!time_overlaps(&rows[i], &rows[j]))
                continue;
            unsigned char types = 0;
            if (strcmp(rows[i].section, rows[j].section) == 0)
                types |= SECTION_CLASH;
            if (strcmp(rows[i].faculty, rows[j].faculty) == 0)
                types |= FACULTY_CLASH;
            if (strcmp(rows[i].room, rows[j].room) == 0)
                types |= ROOM_CLASH;
            // Add edge if any conflict exists
            if (types != 0) {
                add_edge(graph, i, j, types);
                edgeCount++;
            }
        }
    }
    printf("\n----- CONFLICT GRAPH -----\n");
    printf("Vertices (sessions): %d\n", n);
    printf("Edges (conflicting pairs): %d\n\n", edgeCount);
    // Traverse adjacency list and report conflicts
    for (int i = 0; i < n; i++) {
        printf("Session %d: ", i + 1);
        if (graph[i] == NULL) {
            printf("No conflicts\n");
            continue;
        }
        Edge *current = graph[i];
        while (current != NULL) {
            if (current->vertex > i) {
                printf("Connected to Session %d\n",
                       current->vertex + 1);
                display_conflict(current->types, &rows[i]);
                // Count each reported clash type
                if (current->types & SECTION_CLASH)
                    conflicts++;
                if (current->types & FACULTY_CLASH)
                    conflicts++;
                if (current->types & ROOM_CLASH)
                    conflicts++;
            }
            current = current->next;
        }
    }
    // Calculate vertex degrees
    printf("\n----- VERTEX DEGREES -----\n");
    for (int i = 0; i < n; i++) {
        int degree = 0;
        Edge *current = graph[i];
        while (current != NULL) {
            degree++;
            current = current->next;
        }
        printf("Session %d degree: %d\n", i + 1, degree);
    }
    if (conflicts == 0) {
        printf("\nC DSA validation passed: %d sessions; "
               "no section, faculty or room clashes.\n", n);
    } else {
        printf("\nValidation failed: %d clash report(s).\n",
               conflicts);
    }
    free_graph(graph, n);
    free(rows);
    return conflicts == 0 ? 0 : 1;
}
