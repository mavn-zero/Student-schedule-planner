#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char name[64];
  uint32_t semester;
  uint32_t numSections;
} BranchC;

typedef struct {
  char type[32];
  uint32_t count;
} RoomCategoryC;

typedef struct {
  char name[32];
  char type[32];
} RoomC;

typedef struct {
  char name[128];
  char code[32];
  char branchName[64];
  char roomType[32];
  uint32_t semester;
  uint32_t sessionsPerWeek;
} SubjectC;

struct Session;

typedef struct Conflict {
  struct Session *target;
  struct Conflict *next;
} Conflict;

typedef struct Session {
  size_t id;
  int32_t assignedSlot;
  int32_t assignedRoomIndex;
  char sectionName[128];
  SubjectC *subject;
  Conflict *conflicts;
} Session;

typedef struct {
  Session *sessions;
  size_t sessionCount;
  size_t capacity;
} SessionGraph;

void init_graph(SessionGraph *g, size_t initialCapacity);
size_t add_session(SessionGraph *g, const char *sectionName, SubjectC *subject);
void add_conflict(SessionGraph *g, size_t sessionA_id, size_t sessionB_id);
void free_graph(SessionGraph *g);

#ifdef __cplusplus
}
#endif

#endif
