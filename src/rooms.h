#ifndef ROOMS_H
#define ROOMS_H

/* Recover a pending two-file update before any room data is read. */
int recoverRoomTransaction(void);
/* Load/recover once before querying a batch of authoritative allocations. */
int loadRoomState(void);
/* In-memory O(log N) lookup; requires a successful loadRoomState(). */
int lookupStudentRoom(const char *id, int *roomNo);
void roomMenu(void);

#endif
