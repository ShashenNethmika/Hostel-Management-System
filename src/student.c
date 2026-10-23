/* Original student registration module: M.M.Ishara Madhushan (TG IT 2025 2514).
 * Adapted for shared data and input. */
#include "student.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

static int parseStudentLine(char *line, struct Student *student) {
    char *fields[3];
    int count = splitFields(line, fields, 3);
    if (count == 0) return 2; /* Blank line. */
    if (count != 3 || !isValidStudentIndex(fields[0]) ||
        !validStudentName(fields[1]) || !validPhoneNumber(fields[2])) return 0;
    strcpy(student->studentID, fields[0]);
    normalizeStudentIndex(student->studentID);
    strcpy(student->name, fields[1]);
    strcpy(student->phone, fields[2]);
    return 1;
}

int readStudentRecord(FILE *file, struct Student *student, size_t *lineNumber) {
    char line[RECORD_LINE_SIZE];
    while (readDataLine(file, "students.txt", line, sizeof(line), lineNumber)) {
        int parsed = parseStudentLine(line, student);
        if (parsed == 1) return 1;
        if (parsed == 0) warnInvalidRecord("students.txt", *lineNumber);
    }
    return 0;
}

/* Editing and cross-module lookups require an unambiguous, complete register.
 * Do not silently skip damaged records and then write new data over them. */
static int loadStudents(struct Student **students, size_t *count) {
    *students = NULL;
    *count = 0;
    FILE *file = fopen("students.txt", "rb");
    if (file == NULL) {
        if (errno == ENOENT) return 1;
        printDataFileError("students.txt");
        return 0;
    }
    size_t capacity = 0, lineNumber = 0;
    char line[RECORD_LINE_SIZE];
    int status, loaded = 1;
    while ((status = readTextLine(file, line, sizeof(line))) != TEXT_LINE_EOF) {
        lineNumber++;
        struct Student student = {0};
        int parsed = status == TEXT_LINE_OK ? parseStudentLine(line, &student) : 0;
        if (parsed == 2) continue;
        if (parsed != 1) {
            fprintf(stderr, "Invalid students.txt: record on line %zu. No changes made.\n", lineNumber);
            loaded = 0;
            break;
        }
        for (size_t i = 0; i < *count; i++) {
            if (strcmp((*students)[i].studentID, student.studentID) == 0) {
                fprintf(stderr, "Invalid students.txt: duplicate Student ID %s. No changes made.\n", student.studentID);
                loaded = 0;
                break;
            }
        }
        if (!loaded) break;
        if (*count == capacity) {
            size_t next = capacity == 0 ? 16 : capacity * 2;
            if (next < capacity || next > (size_t)-1 / sizeof(**students)) {
                loaded = 0; break;
            }
            struct Student *grown = realloc(*students, next * sizeof(**students));
            if (grown == NULL) { loaded = 0; break; }
            *students = grown;
            capacity = next;
        }
        (*students)[(*count)++] = student;
    }
    if (ferror(file)) loaded = 0;
    fclose(file);
    if (!loaded) {
        printDataFileError("students.txt");
        free(*students);
        *students = NULL;
        *count = 0;
    }
    return loaded;
}

int lookupStudent(const char *id, struct Student *student) {
    if (!isValidStudentIndex(id)) return 0;
    char normalized[STUDENT_ID_SIZE];
    strcpy(normalized, id);
    normalizeStudentIndex(normalized);
    struct Student *students;
    size_t count;
    if (!loadStudents(&students, &count)) return -1;
    int found = 0;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(normalized, students[i].studentID) == 0) {
            *student = students[i];
            found = 1;
            break;
        }
    }
    free(students);
    return found;
}

static int readName(char name[STUDENT_NAME_SIZE]) {
    while (readConsoleLine("Student Name: ", name, STUDENT_NAME_SIZE)) {
        if (validStudentName(name)) return 1;
        printf("Invalid name. Use a nonempty name without digits or '|'.\n");
    }
    return 0;
}

static int readPhone(char phone[11]) {
    char line[64];
    while (readConsoleLine("Phone Number (10 digits): ", line, sizeof(line))) {
        if (validPhoneNumber(line)) { strcpy(phone, line); return 1; }
        printf("Invalid Phone Number! Enter exactly 10 digits.\n");
    }
    return 0;
}

static void printStudent(const struct Student *student) {
    printf("Student ID   : %s\nStudent Name : %s\nPhone Number : %s\n",
           student->studentID, student->name, student->phone);
}

static void registerStudent(void) {
    struct Student student, existing;
    if (!readStudentIndex("Student ID: ", student.studentID)) return;
    int found = lookupStudent(student.studentID, &existing);
    if (found < 0) return;
    if (found) {
        printf("Student ID Already Registered!\n");
        return;
    }
    if (!readName(student.name) || !readPhone(student.phone)) return;
    char record[RECORD_LINE_SIZE];
    snprintf(record, sizeof(record), "%s|%s|%s\n", student.studentID, student.name, student.phone);
    if (!appendDataRecord("students.txt", record)) {
        printDataFileError("students.txt");
        return;
    }
    printf("\nSTUDENT REGISTERED SUCCESSFULLY\n");
    printStudent(&student);
}

static void viewStudents(void) {
    struct Student *students;
    size_t count;
    if (!loadStudents(&students, &count)) return;
    printf("\n%-30s %-30s %s\n", "Student ID", "Student Name", "Phone Number");
    for (size_t i = 0; i < count; i++) {
        printf("%-30s %-30s %s\n", students[i].studentID, students[i].name, students[i].phone);
    }
    printf("Total Registered Students: %zu\n", count);
    free(students);
}

static void searchStudent(void) {
    char id[STUDENT_ID_SIZE];
    struct Student student;
    if (!readStudentIndex("Student ID to search: ", id)) return;
    int found = lookupStudent(id, &student);
    if (found == 1) { printf("\nSTUDENT FOUND\n"); printStudent(&student); }
    else if (found == 0) printf("Student Not Found!\n");
}

static void updateStudent(void) {
    char id[STUDENT_ID_SIZE];
    if (!readStudentIndex("Student ID to update: ", id)) return;
    struct Student *students;
    size_t count;
    if (!loadStudents(&students, &count)) return;
    size_t index;
    for (index = 0; index < count; index++)
        if (strcmp(students[index].studentID, id) == 0) break;
    if (index == count) { printf("Student Not Found!\n"); free(students); return; }
    printf("\n1. Update Name\n2. Update Phone\n0. Cancel\n");
    int choice;
    if (!readMenuChoice(0, 2, &choice) || choice == 0 ||
        (choice == 1 && !readName(students[index].name)) ||
        (choice == 2 && !readPhone(students[index].phone))) {
        free(students);
        return;
    }
    char temporary[RECORD_LINE_SIZE];
    FILE *file = createDataTempFile("students.txt", temporary);
    if (file == NULL) { printDataFileError("students.txt"); free(students); return; }
    for (size_t i = 0; i < count; i++) {
        if (fprintf(file, "%s|%s|%s\n", students[i].studentID, students[i].name, students[i].phone) < 0)
            break;
    }
    if (!commitDataTempFile(file, temporary, "students.txt")) {
        printDataFileError("students.txt");
        free(students);
        return;
    }
    printf("\nSTUDENT UPDATED SUCCESSFULLY!\n");
    printStudent(&students[index]);
    free(students);
}

void studentMenu(void) {
    for (;;) {
        printf("\n===== Student Registration & Management =====\n");
        printf("1. Register Student\n2. View Students\n3. Search Student\n4. Update Student\n0. Back to Main Menu\n");
        int choice;
        if (!readMenuChoice(0, 4, &choice) || choice == 0) return;
        switch (choice) {
            case 1: registerStudent(); break;
            case 2: viewStudents(); break;
            case 3: searchStudent(); break;
            case 4: updateStudent(); break;
        }
    }
}
