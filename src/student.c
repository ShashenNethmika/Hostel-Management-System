/* Original student registration module: Ishara. Adapted for shared data and input. */
#include "student.h"

#include <errno.h>
#include <string.h>

int readStudentRecord(FILE *file, struct Student *student, size_t *lineNumber) {
    char line[RECORD_LINE_SIZE];
    while (readDataLine(file, "students.txt", line, sizeof(line), lineNumber)) {
        char *fields[3];
        if (splitFields(line, fields, 3) != 3 ||
            !isValidStudentIndex(fields[0]) || !validStudentName(fields[1]) ||
            !validPhoneNumber(fields[2])) {
            warnInvalidRecord("students.txt", *lineNumber);
            continue;
        }
        strcpy(student->studentID, fields[0]);
        normalizeStudentIndex(student->studentID);
        strcpy(student->name, fields[1]);
        strcpy(student->phone, fields[2]);
        return 1;
    }
    return 0;
}

int lookupStudent(const char *id, struct Student *student) {
    FILE *file = fopen("students.txt", "r");
    if (file == NULL) {
        if (errno == ENOENT) return 0;
        printDataFileError("students.txt");
        return -1;
    }
    char normalized[STUDENT_ID_SIZE];
    if (!isValidStudentIndex(id)) { fclose(file); return 0; }
    strcpy(normalized, id);
    normalizeStudentIndex(normalized);
    size_t lineNumber = 0;
    while (readStudentRecord(file, student, &lineNumber)) {
        if (strcmp(normalized, student->studentID) == 0) {
            fclose(file);
            return 1;
        }
    }
    int failed = ferror(file);
    fclose(file);
    if (failed) printDataFileError("students.txt");
    return failed ? -1 : 0;
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
    FILE *file = fopen("students.txt", "a");
    if (file == NULL) { printDataFileError("students.txt"); return; }
    int saved = fprintf(file, "%s|%s|%s\n", student.studentID, student.name, student.phone) >= 0;
    if (fclose(file) != 0) saved = 0;
    if (!saved) { printDataFileError("students.txt"); return; }
    printf("\nSTUDENT REGISTERED SUCCESSFULLY\n");
    printStudent(&student);
}

static void viewStudents(void) {
    FILE *file = fopen("students.txt", "r");
    if (file == NULL) { printDataFileError("students.txt"); return; }
    struct Student student;
    size_t lineNumber = 0, count = 0;
    printf("\n%-30s %-30s %s\n", "Student ID", "Student Name", "Phone Number");
    while (readStudentRecord(file, &student, &lineNumber)) {
        printf("%-30s %-30s %s\n", student.studentID, student.name, student.phone);
        count++;
    }
    if (ferror(file)) printDataFileError("students.txt");
    else printf("Total Registered Students: %zu\n", count);
    fclose(file);
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
    struct Student student;
    if (!readStudentIndex("Student ID to update: ", id)) return;
    int found = lookupStudent(id, &student);
    if (found < 0) return;
    if (!found) { printf("Student Not Found!\n"); return; }
    printf("\n1. Update Name\n2. Update Phone\n0. Cancel\n");
    int choice;
    if (!readMenuChoice(0, 2, &choice) || choice == 0) return;
    if (choice == 1 && !readName(student.name)) return;
    if (choice == 2 && !readPhone(student.phone)) return;

    FILE *file = fopen("students.txt", "r");
    if (file == NULL) { printDataFileError("students.txt"); return; }
    FILE *temp = fopen("temp_students.txt", "w");
    if (temp == NULL) { fclose(file); printDataFileError("temp_students.txt"); return; }
    char line[RECORD_LINE_SIZE];
    int saved = 1;
    while (fgets(line, sizeof(line), file) != NULL) {
        char copy[RECORD_LINE_SIZE], *fields[3];
        strcpy(copy, line);
        int fieldsRead = splitFields(copy, fields, 3);
        if (fieldsRead == 3 && isValidStudentIndex(fields[0])) {
            normalizeStudentIndex(fields[0]);
        }
        if (fieldsRead == 3 && strcmp(fields[0], id) == 0) {
            if (fprintf(temp, "%s|%s|%s\n", student.studentID, student.name, student.phone) < 0)
                saved = 0;
        } else if (fputs(line, temp) == EOF) saved = 0;
    }
    if (ferror(file)) saved = 0;
    fclose(file);
    if (fclose(temp) != 0) saved = 0;
    if (!saved || rename("temp_students.txt", "students.txt") != 0) {
        remove("temp_students.txt");
        printDataFileError("students.txt");
        return;
    }
    printf("\nSTUDENT UPDATED SUCCESSFULLY!\n");
    printStudent(&student);
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
