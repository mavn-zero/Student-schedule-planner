#include "structures.h"
#include <stdio.h>
#include <stdlib.h>

void init_graph(SessionGraph *g, size_t initialCapacity) {
  if (initialCapacity == 0) {
    initialCapacity = 48;
  }
  g->capacity = initialCapacity;
  g->sessionCount = 0;
  g->sessions = (Session *)malloc(g->capacity * sizeof(Session));
  if (!g->sessions) {
    fprintf(stderr, "Error in allocating memory for SessionGraph\n");
    exit(1);
  }
}

size_t add_session(SessionGraph *g, const char *sectionName,
                   SubjectC *subject) {
  if (g->sessionCount >= g->capacity) {
    g->capacity *= 2;
    g->sessions =
        (Session *)realloc(g->sessions, g->capacity * sizeof(Session));
    if (!g->sessions) {
      fprintf(stderr, "Error in reallocating memory for SessionGraph\n");
      exit(1);
    }
  }

  size_t id = g->sessionCount;
  Session *newSession = &g->sessions[id];

  newSession->id = id;
  newSession->assignedSlot = -1;
  newSession->assignedRoomIndex = -1;
  newSession->subject = subject;
  newSession->conflicts = NULL;

  snprintf(newSession->sectionName, sizeof(newSession->sectionName), "%s",
           sectionName);

  g->sessionCount++;
  return id;
}

void add_conflict(SessionGraph *g, size_t sessionA_id, size_t sessionB_id) {
  if (sessionA_id == sessionB_id)
    return;
  if (sessionA_id >= g->sessionCount || sessionB_id >= g->sessionCount)
    return;

  Session *a = &g->sessions[sessionA_id];
  Session *b = &g->sessions[sessionB_id];

  Conflict *check = a->conflicts;
  while (check != NULL) {
    if (check->target == b) {
      return;
    }
    check = check->next;
  }

  Conflict *conflictA = (Conflict *)malloc(sizeof(Conflict));
  Conflict *conflictB = (Conflict *)malloc(sizeof(Conflict));
  if (!conflictA || !conflictB) {
    fprintf(stderr, "Error in allocating memory for Conflict\n");
    exit(1);
  }

  conflictA->target = b;
  conflictA->next = a->conflicts;
  a->conflicts = conflictA;

  conflictB->target = a;
  conflictB->next = b->conflicts;
  b->conflicts = conflictB;
}

void free_graph(SessionGraph *g) {
  for (size_t i = 0; i < g->sessionCount; i++) {
    Conflict *current = g->sessions[i].conflicts;
    while (current != NULL) {
      Conflict *temp = current;
      current = current->next;
      free(temp);
    }
  }

  free(g->sessions);
  g->sessions = NULL;
  g->sessionCount = 0;
  g->capacity = 0;
}
