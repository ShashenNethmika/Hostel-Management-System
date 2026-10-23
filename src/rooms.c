/* Original room allocation module: R.M.P.Prabodhini Rathnayake (TG IT 2025 2509).
 * Adapted for shared student records. */
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
#define TRANSACTION_FILE "room_transaction.txt"
#define TRANSACTION_HEADER "HOSTEL_ROOM_TRANSACTION_V1"
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
static int roomStateFailed = 0;

static int readRoomInteger(const char *prompt, int *number) {
    char buffer[128];
    if (!readConsoleLine(prompt, buffer, sizeof(buffer))) return -1;
    return parseInteger(buffer, INT_MIN, INT_MAX, number);
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
    if (!readConsoleLine("Student ID: ", buffer, sizeof(buffer))) return -1;
    if (!validStudentId(buffer)) return 0;
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

static int parseRoomRecord(char *line, Room *room) {
    char *fields[3];
    if (strchr(line, '|') != NULL || splitFields(line, fields, 3) != 3) return 0;
    return parseInteger(fields[0], 1, INT_MAX, &room->roomNo) &&
           parseInteger(fields[1], 1, INT_MAX, &room->capacity) &&
           parseInteger(fields[2], 0, room->capacity, &room->occupied);
}

static int parseAllocationRecord(char *line, Allocation *allocation) {
    char *fields[2];
    if (strchr(line, '|') != NULL || splitFields(line, fields, 2) != 2 ||
        !validStudentId(fields[0]) ||
        !parseInteger(fields[1], 1, INT_MAX, &allocation->roomNo)) return 0;
    strcpy(allocation->studentId, fields[0]);
    normalizeStudentId(allocation->studentId);
    return 1;
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

    int result;
    while ((result = readTextLine(fp, line, sizeof(line))) != TEXT_LINE_EOF) {
        Room r;
        char *p = line;
        lineNumber++;

        if (result != TEXT_LINE_OK) {
            printf("Error: invalid or unreadable rooms.txt line %d.\n", lineNumber);
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
        int result;
        while ((result = readTextLine(fp, line, sizeof(line))) != TEXT_LINE_EOF) {
            Allocation a;
            char *p = line;
            lineNumber++;

            if (result != TEXT_LINE_OK) {
                printf("Error: invalid or unreadable allocations.txt line %d.\n", lineNumber);
                goto fail;
            }
            while (isspace((unsigned char)*p)) {
                p++;
            }
            if (*p == '\0') {
                continue;
            }
            if (!parseAllocationRecord(p, &a) ||
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

static int saveRooms(void) {
    char temporary[RECORD_LINE_SIZE];
    FILE *file = createDataTempFile(ROOM_FILE, temporary);
    if (file == NULL) { printDataFileError(ROOM_FILE); return 0; }
    for (int i = 0; i < roomCount; i++) {
        if (fprintf(file, "%d %d %d\n", rooms[i].roomNo,
                    rooms[i].capacity, rooms[i].occupied) < 0) break;
    }
    if (commitDataTempFile(file, temporary, ROOM_FILE)) return 1;
    printDataFileError(ROOM_FILE);
    return 0;
}

static int saveAllocations(void) {
    char temporary[RECORD_LINE_SIZE];
    FILE *file = createDataTempFile(ALLOCATION_FILE, temporary);
    if (file == NULL) { printDataFileError(ALLOCATION_FILE); return 0; }
    for (int i = 0; i < allocationCount; i++) {
        if (fprintf(file, "%s %d\n", allocations[i].studentId,
                    allocations[i].roomNo) < 0) break;
    }
    if (commitDataTempFile(file, temporary, ALLOCATION_FILE)) return 1;
    printDataFileError(ALLOCATION_FILE);
    return 0;
}

static int readTransactionCount(FILE *file, const char *expected, int maximum, int *count) {
    char line[256], *fields[2];
    return readTextLine(file, line, sizeof(line)) == TEXT_LINE_OK &&
           strchr(line, '|') == NULL && splitFields(line, fields, 2) == 2 &&
           strcmp(fields[0], expected) == 0 && parseInteger(fields[1], 0, maximum, count);
}

/* The journal contains a complete validated target state. Replaying it is safe
 * even if the process previously stopped between the two file replacements. */
int recoverRoomTransaction(void) {
    FILE *file = fopen(TRANSACTION_FILE, "r");
    if (file == NULL) {
        if (errno == ENOENT) return 1;
        perror("Cannot read room transaction");
        return 0;
    }
    char line[256];
    int expectedRooms, expectedAllocations;
    roomCount = allocationCount = 0;
    if (readTextLine(file, line, sizeof(line)) != TEXT_LINE_OK ||
        strcmp(line, TRANSACTION_HEADER) != 0 ||
        !readTransactionCount(file, "ROOMS", MAX_ROOMS, &expectedRooms)) goto invalid;
    for (int i = 0; i < expectedRooms; i++) {
        Room room;
        if (readTextLine(file, line, sizeof(line)) != TEXT_LINE_OK ||
            !parseRoomRecord(line, &room) || findRoom(room.roomNo) != -1) goto invalid;
        rooms[roomCount++] = room;
    }
    if (!readTransactionCount(file, "ALLOCATIONS", MAX_ALLOCATIONS, &expectedAllocations)) goto invalid;
    for (int i = 0; i < expectedAllocations; i++) {
        Allocation allocation;
        if (readTextLine(file, line, sizeof(line)) != TEXT_LINE_OK ||
            !parseAllocationRecord(line, &allocation) || findRoom(allocation.roomNo) == -1 ||
            findAllocation(allocation.studentId) != -1) goto invalid;
        allocations[allocationCount++] = allocation;
    }
    if (readTextLine(file, line, sizeof(line)) != TEXT_LINE_OK || strcmp(line, "END") != 0 ||
        readTextLine(file, line, sizeof(line)) != TEXT_LINE_EOF) goto invalid;
    for (int i = 0; i < roomCount; i++) {
        if (allocatedCountInRoom(rooms[i].roomNo) > rooms[i].occupied) goto invalid;
    }
    if (fclose(file) != 0) { printDataFileError(TRANSACTION_FILE); return 0; }
    if (!saveRooms() || !saveAllocations() || remove(TRANSACTION_FILE) != 0) {
        fprintf(stderr, "Room transaction is pending. Resolve the file error and restart to recover.\n");
        return 0;
    }
    return 1;

invalid:
    fclose(file);
    roomCount = allocationCount = 0;
    fprintf(stderr, "Invalid room transaction journal. Original room data was not changed.\n");
    return 0;
}

static int saveState(void) {
    char temporary[RECORD_LINE_SIZE];
    FILE *file = createDataTempFile(TRANSACTION_FILE, temporary);
    if (file == NULL) {
        printDataFileError(TRANSACTION_FILE);
        roomStateFailed = 1;
        return 0;
    }
    fprintf(file, "%s\nROOMS %d\n", TRANSACTION_HEADER, roomCount);
    for (int i = 0; i < roomCount; i++)
        fprintf(file, "%d %d %d\n", rooms[i].roomNo, rooms[i].capacity, rooms[i].occupied);
    fprintf(file, "ALLOCATIONS %d\n", allocationCount);
    for (int i = 0; i < allocationCount; i++)
        fprintf(file, "%s %d\n", allocations[i].studentId, allocations[i].roomNo);
    fprintf(file, "END\n");
    if (!commitDataTempFile(file, temporary, TRANSACTION_FILE) || !recoverRoomTransaction()) {
        roomStateFailed = 1;
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
    if (!saveState()) {
        printf("Room update could not be completed. Restart to retry recovery.\n");
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

    rooms[roomIndex].occupied++;
    if (!saveState()) {
        printf("Student allocation could not be completed. Restart to retry recovery.\n");
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

    rooms[roomIndex].occupied--;
    if (!saveState()) {
        printf("Student removal could not be completed. Restart to retry recovery.\n");
        return;
    }
    printf("Student %s removed from room %d. Occupied: %d/%d.\n",
           removed.studentId, removed.roomNo, rooms[roomIndex].occupied,
           rooms[roomIndex].capacity);
}

void roomMenu(void) {
    int choice, result;
    roomStateFailed = 0;
    if (!recoverRoomTransaction() || !loadRooms() || !loadAllocations()) {
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
        if (roomStateFailed) return;
    }
}
