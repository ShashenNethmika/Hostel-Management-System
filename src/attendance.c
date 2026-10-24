/* Original attendance module: S.G.A.T.V Subhawickrama (TG IT 2025 2518).
 * Adapted for integration and overdue leave.
 * Last record per student is current. */
#include "attendance.h"
#include "common.h"
#include "student.h"

#include <stdlib.h>
#include <string.h>

struct Attendance {
    char id[STUDENT_ID_SIZE];
    char name[STUDENT_NAME_SIZE];
    int status;
    char date[11];
    char expectedReturn[11];
};

static int readAttendance(FILE *file, struct Attendance *record, size_t *lineNumber) {
    char line[RECORD_LINE_SIZE];
    char today[11];
    currentDate(today);
    while (readDataLine(file, "attendance.txt", line, sizeof(line), lineNumber)) {
        char *fields[5];
        int pipeSeparated = strchr(line, '|') != NULL;
        int count = splitFields(line, fields, 5);
        if ((pipeSeparated ? (count != 4 && count != 5) : count != 2) ||
            !isValidStudentIndex(fields[0])) {
            warnInvalidRecord("attendance.txt", *lineNumber);
            continue;
        }
        /* Only ID Status is a supported whitespace legacy format; history uses pipes. */
        const char *status = count == 2 ? fields[1] : fields[2];
        if (!parseInteger(status, 0, 1, &record->status) ||
            (count >= 4 && (!validStudentName(fields[1]) || !validDate(fields[3]) ||
                            strcmp(fields[3], today) > 0)) ||
            (count == 5 && strcmp(fields[4], "-") != 0 &&
             (!validDate(fields[4]) || strcmp(fields[4], fields[3]) < 0))) {
            warnInvalidRecord("attendance.txt", *lineNumber);
            continue;
        }
        strcpy(record->id, fields[0]);
        normalizeStudentIndex(record->id);
        strcpy(record->name, count == 2 ? "(legacy record)" : fields[1]);
        strcpy(record->date, count == 2 ? "-" : fields[3]);
        strcpy(record->expectedReturn, count == 5 ? fields[4] : "-");
        return 1;
    }
    return 0;
}

static void markAttendance(void) {
    struct Attendance record;
    struct Student student;
    if (!readStudentIndex("Student ID: ", record.id)) return;
    int found = lookupStudent(record.id, &student);
    if (found < 0) return;
    if (found == 0) {
        fprintf(stderr, "Student not registered. Register through Student Management first.\n");
        return;
    }
    strcpy(record.name, student.name);
    printf("Student Name: %s\nStatus (1 = In Hostel, 0 = Went Home)\n", record.name);
    if (!readMenuChoice(0, 1, &record.status)) return;
    currentDate(record.date);
    strcpy(record.expectedReturn, "-");
    if (record.status == 0) {
        char line[64];
        for (;;) {
            if (!readConsoleLine("Expected return date (YYYY-MM-DD): ", line, sizeof(line))) return;
            if (validDate(line) && strcmp(line, record.date) >= 0) {
                strcpy(record.expectedReturn, line);
                break;
            }
            fprintf(stderr, "Invalid return date. Use today or a later calendar date.\n");
        }
    }
    char line[RECORD_LINE_SIZE];
    int length = snprintf(line, sizeof(line), "%s|%s|%d|%s|%s\n", record.id,
                          record.name, record.status, record.date, record.expectedReturn);
    if (length < 0 || (size_t)length >= sizeof(line) ||
        !appendDataRecord("attendance.txt", line)) {
        printDataFileError("attendance.txt");
        return;
    }
    printf("Attendance saved for %s (%s).\n", record.name, record.status ? "In Hostel" : "Went Home");
}

static void viewAll(void) {
    FILE *file = fopen("attendance.txt", "r");
    if (file == NULL) { printDataFileError("attendance.txt"); return; }
    struct Attendance record;
    size_t lineNumber = 0;
    printf("\n%-30s %-25s %-12s %-12s %s\n", "ID", "Name", "Status", "Date", "Expected return");
    while (readAttendance(file, &record, &lineNumber)) {
        printf("%-30s %-25s %-12s %-12s %s\n", record.id, record.name,
               record.status ? "In Hostel" : "Went Home", record.date, record.expectedReturn);
    }
    if (ferror(file)) printDataFileError("attendance.txt");
    fclose(file);
}

void displayLeaveReport(int overdueOnly) {
    FILE *file = fopen("attendance.txt", "r");
    if (file == NULL) { printDataFileError("attendance.txt"); return; }
    struct Attendance *latest = NULL, record;
    size_t count = 0, capacity = 0, lineNumber = 0;
    int failed = 0;
    while (readAttendance(file, &record, &lineNumber)) {
        size_t index;
        for (index = 0; index < count; index++)
            if (strcmp(latest[index].id, record.id) == 0) break;
        if (index == count) {
            if (count == capacity) {
                size_t nextCapacity = capacity == 0 ? 16 : capacity * 2;
                if (nextCapacity < capacity || nextCapacity > (size_t)-1 / sizeof(*latest)) {
                    failed = 1; break;
                }
                struct Attendance *grown = realloc(latest, nextCapacity * sizeof(*latest));
                if (grown == NULL) { failed = 1; break; }
                latest = grown;
                capacity = nextCapacity;
            }
            count++;
        }
        latest[index] = record;
    }
    if (ferror(file)) failed = 1;
    fclose(file);
    if (failed) {
        fprintf(stderr, "Cannot load the attendance report.\n");
        free(latest);
        return;
    }
    /* Validate and read the register once, before presenting any report rows.
     * An invalid register cannot provide unambiguous current student names. */
    struct StudentRegister students = {0};
    if (!loadStudentRegister(&students)) {
        free(latest);
        return;
    }
    char today[11];
    currentDate(today);
    size_t leaveCount = 0, unknownReturn = 0;
    printf("\n--- %s ---\n", overdueOnly ? "Overdue Leave Report" : "Student Leave Report");
    printf("%-30s %-25s %-12s %s\n", "Student ID", "Name", "Since", "Expected return");
    for (size_t i = 0; i < count; i++) {
        if (latest[i].status != 0) continue;
        if (strcmp(latest[i].expectedReturn, "-") == 0) {
            unknownReturn++;
            if (overdueOnly) continue;
        }
        if (overdueOnly && strcmp(latest[i].expectedReturn, today) >= 0) continue;
        const struct Student *student = findRegisteredStudent(&students, latest[i].id);
        const char *name = latest[i].name;
        if (student != NULL) name = student->name;
        printf("%-30s %-25s %-12s %s\n", latest[i].id, name, latest[i].date, latest[i].expectedReturn);
        leaveCount++;
    }
    if (overdueOnly) {
        printf("Total students overdue: %zu\n", leaveCount);
        if (unknownReturn) printf("%zu legacy leave record(s) have no expected return date.\n", unknownReturn);
    } else printf("Total students currently on leave: %zu\n", leaveCount);
    freeStudentRegister(&students);
    free(latest);
}

void displayOverdueLeaves(void) {
    displayLeaveReport(1);
}

void attendanceMenu(void) {
    for (;;) {
        printf("\n===== Attendance & Leave Management =====\n");
        printf("1. Mark Attendance\n2. View All Records\n3. View Students Who Went Home\n4. View Overdue Leaves\n0. Back to Main Menu\n");
        int choice;
        if (!readMenuChoice(0, 4, &choice) || choice == 0) return;
        switch (choice) {
            case 1: markAttendance(); break;
            case 2: viewAll(); break;
            case 3: displayLeaveReport(0); break;
            case 4: displayOverdueLeaves(); break;
        }
    }
}
