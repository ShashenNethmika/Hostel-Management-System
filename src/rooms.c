/* Original room allocation module: Prabodhini. Adapted for shared student records. */
#include "rooms.h"
#include "student.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROOM_FILE "rooms.txt"
#define ALLOCATION_FILE "allocations.txt"
#define MAX_ROOMS 200
#define MAX_ALLOCATIONS 10000
#define STUDENT_ID_LENGTH (STUDENT_ID_SIZE - 1)

typedef struct {
    int roomNo;
    int capacity;
    int occupied;
} Room;

typedef struct {
    char studentId[STUDENT_ID_LENGTH + 1];
    int roomNo;
} Allocation;

static Room rooms[MAX_ROOMS];
static Allocation allocations[MAX_ALLOCATIONS];
static int roomCount = 0;
static int allocationCount = 0;

static void discardLine(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
}

static int readRoomInteger(const char *prompt, int *number) {
    char buffer[128];
    char *end;
    long parsed;

    printf("%s", prompt);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }
    if (strchr(buffer, '\n') == NULL && !feof(stdin)) {
        discardLine();
        return 0;
    }

    errno = 0;
    parsed = strtol(buffer, &end, 10);
    if (end == buffer || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        return 0;
    }

    *number = (int)parsed;
    return 1;
}

static int validStudentId(const char *id) {
    return isValidStudentIndex(id);
}

static void normalizeStudentId(char *id) {
    size_t i;
    for (i = 0; id[i] != '\0'; i++) {
        id[i] = (char)toupper((unsigned char)id[i]);
    }
}

static int readStudentId(char *id) {
    char buffer[128];
    size_t len;

    printf("Student ID: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }
    if (strchr(buffer, '\n') == NULL && !feof(stdin)) {
        discardLine();
        return 0;
    }

    len = strcspn(buffer, "\r\n");
    buffer[len] = '\0';
    if (!validStudentId(buffer)) {
        return 0;
    }
    strcpy(id, buffer);
    normalizeStudentId(id);
    return 1;
}

static int findRoom(int roomNo) {
    int i;
    for (i = 0; i < roomCount; i++) {
        if (rooms[i].roomNo == roomNo) {
            return i;
        }
    }
    return -1;
}

static int findAllocation(const char *studentId) {
    int i;
    for (i = 0; i < allocationCount; i++) {
        if (strcmp(allocations[i].studentId, studentId) == 0) {
            return i;
        }
    }
    return -1;
}

static int allocatedCountInRoom(int roomNo) {
    int i, total = 0;
    for (i = 0; i < allocationCount; i++) {
        if (allocations[i].roomNo == roomNo) {
            total++;
        }
    }
    return total;
}

static int parseRoomRecord(const char *line, Room *r) {
    char trailing;
    return sscanf(line, "%d %d %d %c", &r->roomNo, &r->capacity,
                  &r->occupied, &trailing) == 3;
}

static int loadRooms(void) {
    FILE *fp = fopen(ROOM_FILE, "r");
    char line[256];
    int lineNumber = 0;
    roomCount = 0;

    if (fp == NULL) {
        if (errno == ENOENT) {
            return 1;
        }
        perror("Cannot open rooms.txt");
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        Room r;
        char *p = line;
        lineNumber++;

        if (strchr(line, '\n') == NULL && !feof(fp)) {
            printf("Error: rooms.txt line %d is too long.\n", lineNumber);
            goto fail;
        }
        while (isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            continue;
        }
        if (!parseRoomRecord(p, &r) || r.roomNo <= 0 || r.capacity <= 0 ||
            r.occupied < 0 || r.occupied > r.capacity ||
            findRoom(r.roomNo) != -1) {
            printf("Error: invalid or duplicate room on line %d of rooms.txt.\n",
                   lineNumber);
            goto fail;
        }
        if (roomCount >= MAX_ROOMS) {
            printf("Error: at most %d rooms are supported.\n", MAX_ROOMS);
            goto fail;
        }
        rooms[roomCount++] = r;
    }
    if (ferror(fp)) {
        printf("Error while reading rooms.txt.\n");
        goto fail;
    }
    fclose(fp);
    return 1;

fail:
    fclose(fp);
    roomCount = 0;
    return 0;
}

static int loadAllocations(void) {
    FILE *fp = fopen(ALLOCATION_FILE, "r");
    char line[256];
    int lineNumber = 0;
    int i;
    allocationCount = 0;

    if (fp == NULL) {
        if (errno != ENOENT) {
            perror("Cannot open allocations.txt");
            return 0;
        }
    } else {
        while (fgets(line, sizeof(line), fp) != NULL) {
            Allocation a;
            char extra;
            char *p = line;
            lineNumber++;

            if (strchr(line, '\n') == NULL && !feof(fp)) {
                printf("Error: allocations.txt line %d is too long.\n", lineNumber);
                goto fail;
            }
            while (isspace((unsigned char)*p)) {
                p++;
            }
            if (*p == '\0') {
                continue;
            }
            if (sscanf(p, "%29s %d %c", a.studentId, &a.roomNo, &extra) != 2 ||
                !validStudentId(a.studentId) || a.roomNo <= 0 ||
                findRoom(a.roomNo) == -1) {
                printf("Error: invalid allocation on line %d of allocations.txt.\n",
                       lineNumber);
                goto fail;
            }
            normalizeStudentId(a.studentId);
            if (findAllocation(a.studentId) != -1) {
                printf("Error: Student ID %s is allocated more than once.\n",
                       a.studentId);
                goto fail;
            }
            if (allocationCount >= MAX_ALLOCATIONS) {
                printf("Error: too many student allocations.\n");
                goto fail;
            }
            allocations[allocationCount++] = a;
        }
        if (ferror(fp)) {
            printf("Error while reading allocations.txt.\n");
            goto fail;
        }
        fclose(fp);
    }

    for (i = 0; i < roomCount; i++) {
        int tracked = allocatedCountInRoom(rooms[i].roomNo);
        if (tracked > rooms[i].occupied) {
            printf("Error: room %d has %d tracked students but occupied is %d.\n",
                   rooms[i].roomNo, tracked, rooms[i].occupied);
            printf("Fix rooms.txt and allocations.txt before making changes.\n");
            allocationCount = 0;
            return 0;
        }
        if (tracked < rooms[i].occupied) {
            printf("Note: room %d has %d older occupant(s) without Student IDs.\n",
                   rooms[i].roomNo, rooms[i].occupied - tracked);
            printf("They cannot be removed by ID until their real allocations\n"
                   "are recorded in allocations.txt.\n");
        }
    }
    return 1;

fail:
    fclose(fp);
    allocationCount = 0;
    return 0;
}

static int commitTempFile(const char *temporary, const char *destination) {
    char backup[128];
    FILE *original;

    if (rename(temporary, destination) == 0) {
        return 1;
    }

    original = fopen(destination, "r");
    if (original == NULL) {
        remove(temporary);
        return 0;
    }
    fclose(original);

    if (snprintf(backup, sizeof(backup), "%s.bak", destination) >=
        (int)sizeof(backup)) {
        remove(temporary);
        return 0;
    }
    if (remove(backup) != 0 && errno != ENOENT) {
        remove(temporary);
        return 0;
    }
    if (rename(destination, backup) != 0) {
        remove(temporary);
        return 0;
    }
    if (rename(temporary, destination) == 0) {
        remove(backup);
        return 1;
    }
    if (rename(backup, destination) != 0) {
        printf("CRITICAL: restore %s from %s manually.\n", destination, backup);
    }
    remove(temporary);
    return 0;
}

static int saveRooms(void) {
    FILE *fp = fopen("rooms.txt.tmp", "w");
    int i;
    if (fp == NULL) {
        perror("Cannot create rooms.txt.tmp");
        return 0;
    }
    for (i = 0; i < roomCount; i++) {
        if (fprintf(fp, "%d %d %d\n", rooms[i].roomNo,
                    rooms[i].capacity, rooms[i].occupied) < 0) {
            fclose(fp);
            remove("rooms.txt.tmp");
            return 0;
        }
    }
    if (fclose(fp) != 0) {
        remove("rooms.txt.tmp");
        return 0;
    }
    if (!commitTempFile("rooms.txt.tmp", ROOM_FILE)) {
        printf("Error: rooms.txt could not be saved.\n");
        return 0;
    }
    return 1;
}

static int saveAllocations(void) {
    FILE *fp = fopen("allocations.txt.tmp", "w");
    int i;
    if (fp == NULL) {
        perror("Cannot create allocations.txt.tmp");
        return 0;
    }
    for (i = 0; i < allocationCount; i++) {
        if (fprintf(fp, "%s %d\n", allocations[i].studentId,
                    allocations[i].roomNo) < 0) {
            fclose(fp);
            remove("allocations.txt.tmp");
            return 0;
        }
    }
    if (fclose(fp) != 0) {
        remove("allocations.txt.tmp");
        return 0;
    }
    if (!commitTempFile("allocations.txt.tmp", ALLOCATION_FILE)) {
        printf("Error: allocations.txt could not be saved.\n");
        return 0;
    }
    return 1;
}

static void viewRooms(void) {
    int i;
    printf("\n%-8s %-10s %-10s %-10s\n",
           "Room", "Capacity", "Occupied", "Status");
    printf("----------------------------------------\n");
    for (i = 0; i < roomCount; i++) {
        printf("%-8d %-10d %-10d %-10s\n", rooms[i].roomNo,
               rooms[i].capacity, rooms[i].occupied,
               rooms[i].occupied == rooms[i].capacity ? "FULL" : "Available");
    }
    if (roomCount == 0) {
        printf("No rooms found.\n");
    }
}

static void viewVacantRooms(void) {
    int i, found = 0;
    printf("\nRooms with free beds:\n");
    for (i = 0; i < roomCount; i++) {
        if (rooms[i].occupied < rooms[i].capacity) {
            printf("Room %d: %d bed(s) free\n", rooms[i].roomNo,
                   rooms[i].capacity - rooms[i].occupied);
            found = 1;
        }
    }
    if (!found) {
        printf("No rooms with free beds.\n");
    }
}

static void viewAllocations(void) {
    int i;
    printf("\n%-32s %-10s\n", "Student ID", "Room");
    printf("--------------------------------------------\n");
    for (i = 0; i < allocationCount; i++) {
        printf("%-32s %-10d\n", allocations[i].studentId,
               allocations[i].roomNo);
    }
    if (allocationCount == 0) {
        printf("No identified students have been allocated.\n");
    }
}

static void addRoom(void) {
    Room r;
    int result;
    if (roomCount >= MAX_ROOMS) {
        printf("Room list is full.\n");
        return;
    }
    result = readRoomInteger("Room number: ", &r.roomNo);
    if (result != 1 || r.roomNo <= 0) {
        if (result != -1) printf("Invalid room number.\n");
        return;
    }
    if (findRoom(r.roomNo) != -1) {
        printf("Room already exists.\n");
        return;
    }
    result = readRoomInteger("Capacity: ", &r.capacity);
    if (result != 1 || r.capacity <= 0) {
        if (result != -1) printf("Invalid capacity.\n");
        return;
    }
    r.occupied = 0;
    rooms[roomCount++] = r;
    if (!saveRooms()) {
        roomCount--;
        printf("Room was NOT saved.\n");
        return;
    }
    printf("Room %d added successfully.\n", r.roomNo);
}

static void allocateStudent(void) {
    char studentId[STUDENT_ID_LENGTH + 1];
    int roomNo, roomIndex, result;

    if (allocationCount >= MAX_ALLOCATIONS) {
        printf("Student allocation list is full.\n");
        return;
    }
    result = readStudentId(studentId);
    if (result != 1) {
        if (result != -1) printf("Invalid Student ID (TG prefix; max 29 characters).\n");
        return;
    }
    struct Student student;
    if (lookupStudent(studentId, &student) != 1) {
        printf("Student not registered. Register through Student Management first.\n");
        return;
    }
    if (findAllocation(studentId) != -1) {
        printf("Student %s is already allocated to room %d.\n",
               studentId, allocations[findAllocation(studentId)].roomNo);
        return;
    }

    result = readRoomInteger("Room number: ", &roomNo);
    if (result != 1 || roomNo <= 0) {
        if (result != -1) printf("Invalid room number.\n");
        return;
    }
    roomIndex = findRoom(roomNo);
    if (roomIndex == -1) {
        printf("Room not found.\n");
        return;
    }
    if (rooms[roomIndex].occupied >= rooms[roomIndex].capacity) {
        printf("Room %d is FULL. Student was not allocated.\n", roomNo);
        return;
    }

    strcpy(allocations[allocationCount].studentId, studentId);
    allocations[allocationCount].roomNo = roomNo;
    allocationCount++;

    if (!saveAllocations()) {
        allocationCount--;
        printf("Student allocation was NOT saved.\n");
        return;
    }

    rooms[roomIndex].occupied++;
    if (!saveRooms()) {
        rooms[roomIndex].occupied--;
        allocationCount--;
        if (!saveAllocations()) {
            printf("CRITICAL: allocations.txt may now disagree with rooms.txt.\n");
            printf("Check both files before using this module again.\n");
        }
        printf("Student allocation could not be completed.\n");
        return;
    }
    printf("Student %s allocated to room %d. Occupied: %d/%d.\n",
           studentId, roomNo, rooms[roomIndex].occupied,
           rooms[roomIndex].capacity);
}

static void removeStudent(void) {
    char studentId[STUDENT_ID_LENGTH + 1];
    Allocation removed;
    int allocationIndex, roomIndex, lastIndex, result;

    result = readStudentId(studentId);
    if (result != 1) {
        if (result != -1) printf("Invalid Student ID.\n");
        return;
    }
    allocationIndex = findAllocation(studentId);
    if (allocationIndex == -1) {
        printf("Student %s has no recorded room allocation.\n", studentId);
        return;
    }

    removed = allocations[allocationIndex];
    roomIndex = findRoom(removed.roomNo);
    if (roomIndex == -1 || rooms[roomIndex].occupied <= 0) {
        printf("Room records do not match this allocation. No changes made.\n");
        return;
    }
    printf("Student %s is allocated to room %d.\n",
           removed.studentId, removed.roomNo);

    lastIndex = allocationCount - 1;
    allocations[allocationIndex] = allocations[lastIndex];
    allocationCount--;

    if (!saveAllocations()) {
        allocationCount++;
        allocations[lastIndex] = allocations[allocationIndex];
        allocations[allocationIndex] = removed;
        printf("Student removal was NOT saved.\n");
        return;
    }

    rooms[roomIndex].occupied--;
    if (!saveRooms()) {
        rooms[roomIndex].occupied++;
        allocationCount++;
        allocations[lastIndex] = allocations[allocationIndex];
        allocations[allocationIndex] = removed;
        if (!saveAllocations()) {
            printf("CRITICAL: allocations.txt may now disagree with rooms.txt.\n");
            printf("Check both files before using this module again.\n");
        }
        printf("Student removal could not be completed.\n");
        return;
    }
    printf("Student %s removed from room %d. Occupied: %d/%d.\n",
           removed.studentId, removed.roomNo, rooms[roomIndex].occupied,
           rooms[roomIndex].capacity);
}

void roomMenu(void) {
    int choice, result;
    if (!loadRooms() || !loadAllocations()) {
        printf("Fix room data files before using the Room Allocation menu.\n");
        return;
    }
    for (;;) {
        printf("\n===== Room Allocation & Management =====\n");
        printf("1. View All Rooms\n");
        printf("2. View Vacant Rooms\n");
        printf("3. Add Room\n");
        printf("4. Allocate Student to Room (by Student ID)\n");
        printf("5. Remove Student from Room (by Student ID)\n");
        printf("6. View Student Allocations\n");
        printf("0. Back\n");

        result = readRoomInteger("Choice: ", &choice);
        if (result == -1) return;
        if (result == 0) {
            printf("Invalid choice.\n");
            continue;
        }
        switch (choice) {
            case 1: viewRooms(); break;
            case 2: viewVacantRooms(); break;
            case 3: addRoom(); break;
            case 4: allocateStudent(); break;
            case 5: removeStudent(); break;
            case 6: viewAllocations(); break;
            case 0: return;
            default: printf("Invalid choice.\n");
        }
    }
}
