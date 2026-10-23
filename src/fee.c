/* Original fees module: H.G.Chesmi Tharushika (TG IT 2025 2465).
 * Adapted for shared student records and data paths.
 *
 * Payment file: payments.txt in the shared data directory
 * Payment format: Student_ID Total_Fee Paid_Amount
 * Example: TgIt20252469 50000.00 15000.00
 * Student details: fee_students.txt (ID|Name|RoomNo|MonthlyFee)
 * Monthly fee is stored separately; the total payable remains Rs 50,000.
 *
 * Only feeMenu() is public. No main() is defined here.
 */

#include "fee.h"
#include "student.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAYMENT_FILE "payments.txt"
#define STUDENT_FILE "fee_students.txt"
#define ID_LEN       30
#define NAME_LEN     100
#define TOTAL_CENTS  5000000L  /* Rs 50,000.00 */

typedef struct {
    char id[ID_LEN];
    long paidCents;
} FeeRecord;

typedef struct {
    FeeRecord *items;
    size_t count;
    size_t capacity;
} FeeList;

typedef struct {
    char id[ID_LEN];
    char name[NAME_LEN];
    int roomNo;
    long monthlyCents;
} StudentInfo;

typedef struct {
    StudentInfo *items;
    size_t count;
    size_t capacity;
} StudentList;

/* Read complete console input, rejecting overlong lines and embedded NULs. */
static int readLine(char *buffer, size_t capacity) {
    int status = readTextLine(stdin, buffer, capacity);
    if (status == TEXT_LINE_EOF || status == TEXT_LINE_ERROR) return 0;
    if (status == TEXT_LINE_INVALID) {
        printf("Invalid input: line is too long or contains a NUL byte.\n");
        return -1;
    }
    return 1;
}

static char *trim(char *s) {
    char *end;
    while (isspace((unsigned char)*s)) s++;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

/* Share the registration module's ID rules, including legacy slash IDs. */
static int validId(const char *id) {
    return isValidStudentIndex(id);
}

static int readStudentId(char id[ID_LEN]) {
    char line[128];
    char *s;

    printf("Student ID: ");
    if (readLine(line, sizeof(line)) != 1) return 0;
    s = trim(line);
    normalizeStudentIndex(s);
    /* Student ID must start with "TG" (e.g. TG1234). */
    if (strncmp(s, "TG", 2) != 0) {
        printf("Invalid Student ID. ID must start with \"TG\".\n");
        return 0;
    }
    if (!validId(s)) {
        printf("Invalid Student ID (3-%d characters; letters, numbers, /, - or _).\n",
               ID_LEN - 1);
        return 0;
    }
    strcpy(id, s);
    return 1;
}

/* Only one-line printable names, without the '|' file delimiter. */
static int validName(const char *name) {
    size_t i, len = strlen(name);
    if (len == 0 || len >= NAME_LEN) return 0;
    for (i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (c == '|' || iscntrl(c)) return 0;
    }
    return 1;
}

static int parseRoomNo(const char *text, int *room) {
    char *end;
    long n;
    if (*text == '\0') return 0;
    /* Disallow signs and leading non-digit characters. */
    {
        const char *p;
        for (p = text; *p; ++p) {
            if (!isdigit((unsigned char)*p)) return 0;
        }
    }
    errno = 0;
    n = strtol(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || n <= 0 || n > INT_MAX) return 0;
    *room = (int)n;
    return 1;
}

static int readRoomNo(int *room) {
    char line[128];
    printf("Room No: ");
    if (readLine(line, sizeof(line)) != 1) return 0;
    if (!parseRoomNo(trim(line), room)) {
        printf("Invalid room number (enter a positive whole number).\n");
        return 0;
    }
    return 1;
}

/* Convert a nonnegative amount to exact cents (max Rs 50,000.00).
 * Rejects partial input, NaN, signs, exponents and >2 decimals.
 */
static int parseFeeMoney(const char *s, long *cents) {
    long whole = 0;
    long fraction = 0;
    int digits = 0;
    int fractionDigits = 0;

    if (*s == '\0') return 0;
    while (*s >= '0' && *s <= '9') {
        int digit = *s++ - '0';
        if (whole > (TOTAL_CENTS / 100 - digit) / 10) return 0;
        whole = whole * 10 + digit;
        ++digits;
    }
    if (digits == 0) return 0;

    if (*s == '.') {
        ++s;
        while (*s >= '0' && *s <= '9') {
            if (fractionDigits >= 2) return 0;
            fraction = fraction * 10 + (*s++ - '0');
            ++fractionDigits;
        }
        if (fractionDigits == 0) return 0;
        if (fractionDigits == 1) fraction *= 10;
    }
    if (*s != '\0') return 0;

    *cents = whole * 100 + fraction;
    return *cents <= TOTAL_CENTS;
}

static int readPaymentAmount(long *cents) {
    char line[128];
    char *amount;

    printf("Amount to pay (Rs): ");
    if (readLine(line, sizeof(line)) != 1) return 0;
    amount = trim(line);
    if (!parseFeeMoney(amount, cents) || *cents == 0) {
        printf("Invalid amount. Enter a positive amount with up to 2 decimals.\n");
        return 0;
    }
    return 1;
}

static int readMonthlyFee(long *cents) {
    char line[128];
    printf("Monthly Fee (Rs): ");
    if (readLine(line, sizeof(line)) != 1) return 0;
    if (!parseFeeMoney(trim(line), cents) || *cents <= 0) {
        printf("Invalid monthly fee (Rs 0.01 - Rs 50000.00; up to 2 decimals).\n");
        return 0;
    }
    return 1;
}

static void freeRecords(FeeList *list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int pushRecord(FeeList *list, FeeRecord record) {
    FeeRecord *grown;
    size_t newCapacity;

    if (list->count == list->capacity) {
        newCapacity = list->capacity ? list->capacity * 2 : 16;
        if (newCapacity < list->capacity ||
            newCapacity > ((size_t)-1) / sizeof(FeeRecord)) {
            printf("Too many payment records.\n");
            return 0;
        }
        grown = (FeeRecord *)realloc(list->items,
                                     newCapacity * sizeof(FeeRecord));
        if (grown == NULL) {
            printf("Not enough memory to load payment records.\n");
            return 0;
        }
        list->items = grown;
        list->capacity = newCapacity;
    }
    list->items[list->count++] = record;
    return 1;
}

static FeeRecord *findRecord(FeeList *list, const char *id) {
    size_t i;
    for (i = 0; i < list->count; ++i) {
        if (strcmp(list->items[i].id, id) == 0) return &list->items[i];
    }
    return NULL;
}

/* Load and validate every line BEFORE any editing, so a broken data file
 * is never silently truncated or partly rewritten.
 */
static int loadRecords(FeeList *list) {
    FILE *fp;
    char line[256];
    unsigned long lineNumber = 0;
    int ok = 1;

    fp = fopen(PAYMENT_FILE, "r");
    if (fp == NULL) {
        if (errno == ENOENT) return 1;  /* new project / empty database */
        perror("Cannot read payments.txt");
        return 0;
    }

    int status;
    while ((status = readTextLine(fp, line, sizeof(line))) != TEXT_LINE_EOF) {
        char id[ID_LEN + 1];
        char totalText[32], paidText[32], extra[2];
        long total, paid;
        FeeRecord record;
        ++lineNumber;

        if (status != TEXT_LINE_OK) {
            printf("Invalid payments.txt: malformed line %lu.\n", lineNumber);
            ok = 0;
            break;
        }
        if (*trim(line) == '\0') continue; /* blank lines are harmless */

        if (sscanf(line, "%30s %31s %31s %1s",
                   id, totalText, paidText, extra) != 3 ||
            !validId(id) ||
            !parseFeeMoney(totalText, &total) || total != TOTAL_CENTS ||
            !parseFeeMoney(paidText, &paid) || paid > total) {
            printf("Invalid payments.txt: bad record on line %lu.\n", lineNumber);
            ok = 0;
            break;
        }
        normalizeStudentIndex(id);
        if (findRecord(list, id) != NULL) {
            printf("Invalid payments.txt: duplicate Student ID %s on line %lu.\n",
                   id, lineNumber);
            ok = 0;
            break;
        }
        strcpy(record.id, id);
        record.paidCents = paid;
        if (!pushRecord(list, record)) {
            ok = 0;
            break;
        }
    }
    if (ferror(fp)) {
        perror("Error reading payments.txt");
        ok = 0;
    }
    if (fclose(fp) != 0) {
        perror("Error closing payments.txt");
        ok = 0;
    }
    if (!ok) freeRecords(list);
    return ok;
}

/* Student information is kept OUT of payments.txt, preserving reports.c's
 * strict three-space-separated-fields format. Names can contain spaces.
 * Each line is: StudentID|Student Name|RoomNo|MonthlyFee
 */
static void freeStudents(StudentList *list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static StudentInfo *findStudent(StudentList *list, const char *id) {
    size_t i;
    for (i = 0; i < list->count; ++i) {
        if (strcmp(list->items[i].id, id) == 0) return &list->items[i];
    }
    return NULL;
}

static int pushStudent(StudentList *list, StudentInfo record) {
    StudentInfo *grown;
    size_t newCapacity;
    if (list->count == list->capacity) {
        newCapacity = list->capacity ? list->capacity * 2 : 16;
        if (newCapacity < list->capacity ||
            newCapacity > ((size_t)-1) / sizeof(StudentInfo)) {
            printf("Too many student records.\n");
            return 0;
        }
        grown = (StudentInfo *)realloc(list->items,
                                       newCapacity * sizeof(StudentInfo));
        if (!grown) {
            printf("Not enough memory to load student details.\n");
            return 0;
        }
        list->items = grown;
        list->capacity = newCapacity;
    }
    list->items[list->count++] = record;
    return 1;
}

static int loadStudents(StudentList *list) {
    FILE *fp = fopen(STUDENT_FILE, "r");
    char line[512];
    unsigned long lineNumber = 0;
    int ok = 1;

    if (!fp) {
        if (errno == ENOENT) return 1; /* allow projects with old fee data */
        perror("Cannot read fee_students.txt");
        return 0;
    }
    int status;
    while ((status = readTextLine(fp, line, sizeof(line))) != TEXT_LINE_EOF) {
        char *id, *name, *room, *monthly, *p;
        StudentInfo record;
        ++lineNumber;
        if (status != TEXT_LINE_OK) {
            printf("Invalid fee_students.txt: malformed line %lu.\n", lineNumber);
            ok = 0;
            break;
        }
        p = trim(line);
        if (*p == '\0') continue;

        id = p;
        name = strchr(id, '|');
        if (!name) { ok = 0; goto invalid_student; }
        *name++ = '\0';
        room = strchr(name, '|');
        if (!room) { ok = 0; goto invalid_student; }
        *room++ = '\0';
        monthly = strchr(room, '|');
        if (!monthly) { ok = 0; goto invalid_student; }
        *monthly++ = '\0';

        id = trim(id);
        normalizeStudentIndex(id);
        name = trim(name);
        room = trim(room);
        monthly = trim(monthly);
        if (!validId(id) || !validName(name) ||
            !parseRoomNo(room, &record.roomNo) ||
            !parseFeeMoney(monthly, &record.monthlyCents) ||
            record.monthlyCents <= 0 || findStudent(list, id) != NULL) {
            ok = 0;
            goto invalid_student;
        }
        strcpy(record.id, id);
        strcpy(record.name, name);
        if (!pushStudent(list, record)) { ok = 0; break; }
        continue;
invalid_student:
        printf("Invalid fee_students.txt: bad or duplicate record on line %lu.\n",
               lineNumber);
        break;
    }
    if (ferror(fp)) { perror("Error reading fee_students.txt"); ok = 0; }
    if (fclose(fp) != 0) { perror("Error closing fee_students.txt"); ok = 0; }
    if (!ok) freeStudents(list);
    return ok;
}

/* Use an exclusively created temporary file, then atomically replace the
 * destination. Existing stale temporary files cannot redirect these writes.
 */
static int saveRecords(const FeeList *list) {
    char temporary[RECORD_LINE_SIZE];
    FILE *fp = createDataTempFile(PAYMENT_FILE, temporary);
    if (fp == NULL) {
        perror("Cannot create temporary payments file (check data directory)");
        return 0;
    }
    for (size_t i = 0; i < list->count; ++i) {
        long paid = list->items[i].paidCents;
        if (fprintf(fp, "%s 50000.00 %ld.%02ld\n",
                    list->items[i].id, paid / 100, paid % 100) < 0) {
            fclose(fp);
            remove(temporary);
            printf("Cannot write payment data. Original file unchanged.\n");
            return 0;
        }
    }
    if (!commitDataTempFile(fp, temporary, PAYMENT_FILE)) {
        printf("Cannot save payment data. Original file unchanged.\n");
        return 0;
    }
    return 1;
}

/* Metadata changes never reset previously recorded payments. */
static int saveStudents(const StudentList *list) {
    char temporary[RECORD_LINE_SIZE];
    FILE *fp = createDataTempFile(STUDENT_FILE, temporary);
    if (fp == NULL) {
        perror("Cannot create temporary student-details file (check data directory)");
        return 0;
    }
    for (size_t i = 0; i < list->count; ++i) {
        const StudentInfo *record = &list->items[i];
        if (fprintf(fp, "%s|%s|%d|%ld.%02ld\n",
                    record->id, record->name, record->roomNo,
                    record->monthlyCents / 100, record->monthlyCents % 100) < 0) {
            fclose(fp);
            remove(temporary);
            printf("Cannot write student details. Original file unchanged.\n");
            return 0;
        }
    }
    if (!commitDataTempFile(fp, temporary, STUDENT_FILE)) {
        printf("Cannot save student details. Original file unchanged.\n");
        return 0;
    }
    return 1;
}

/* Returns the optional student details for one ID. */
static const StudentInfo *studentForId(const StudentList *students, const char *id) {
    size_t i;
    for (i = 0; i < students->count; ++i) {
        if (strcmp(students->items[i].id, id) == 0) return &students->items[i];
    }
    return NULL;
}

static void printStudentDetails(const StudentInfo *student) {
    if (!student) {
        printf("Student details: Not registered in fee_students.txt.\n");
        return;
    }
    struct Student registered;
    const char *name = student->name;
    if (lookupStudent(student->id, &registered) == 1) name = registered.name;
    printf("Student Name: %s\n", name);
    printf("Room No     : %d\n", student->roomNo);
    printf("Monthly Fee : Rs %ld.%02ld\n",
           student->monthlyCents / 100, student->monthlyCents % 100);
}

static void addStudentFee(void) {
    FeeList fees = {0};
    StudentList students = {0};
    FeeRecord record;
    StudentInfo student;
    FeeRecord *existingFee;
    struct Student registered;

    printf("\n--- Add Student ---\n");
    if (!readStudentId(student.id)) return;
    if (lookupStudent(student.id, &registered) != 1) {
        printf("Student not registered. Register through Student Management first.\n");
        return;
    }
    strcpy(student.name, registered.name);
    if (!loadRecords(&fees)) return;
    if (!loadStudents(&students)) { freeRecords(&fees); return; }

    existingFee = findRecord(&fees, student.id);
    if (findStudent(&students, student.id) != NULL) {
        printf("Student %s already has registered details. Duplicate not added.\n",
               student.id);
        goto cleanup;
    }
    if (existingFee != NULL) {
        printf("Existing payment record found. Details will be added without "
               "resetting paid amounts.\n");
    }
    if (!readRoomNo(&student.roomNo) ||
        !readMonthlyFee(&student.monthlyCents)) goto cleanup;

    /* Existing payment records can gain missing personal details safely.
     * For new records, write the payment file FIRST, then student details.
     * If the second write fails, rerunning Add Student completes the profile
     * instead of zeroing or overwriting existing payments.
     */
    if (existingFee == NULL) {
        strcpy(record.id, student.id);
        record.paidCents = 0;
        if (!pushRecord(&fees, record) || !saveRecords(&fees)) {
            printf("Student NOT added; could not save payment record.\n");
            goto cleanup;
        }
    }
    if (!pushStudent(&students, student) || !saveStudents(&students)) {
        printf("Could not save student details. Payment record, if created, "
               "remains safe. Choose Add Student again to complete details.\n");
        goto cleanup;
    }
    printf("\nStudent added successfully!\n");
    printf("Student ID  : %s\n", student.id);
    printStudentDetails(&student);
    printf("Total Fee   : Rs 50000.00\n");
    if (existingFee) {
        printf("Previous Paid: Rs %ld.%02ld (preserved)\n",
               existingFee->paidCents / 100, existingFee->paidCents % 100);
    } else {
        printf("Paid Amount : Rs 0.00\n");
    }
cleanup:
    freeStudents(&students);
    freeRecords(&fees);
}

static void recordPayment(void) {
    FeeList list = {0};
    FeeRecord *record;
    char id[ID_LEN];
    long amount, balance;

    printf("\n--- Record Payment ---\n");
    if (!readStudentId(id)) return;
    if (!loadRecords(&list)) return;

    record = findRecord(&list, id);
    if (record == NULL) {
        printf("Student not found. Add the fee record first.\n");
        freeRecords(&list);
        return;
    }
    balance = TOTAL_CENTS - record->paidCents;
    printf("Total: Rs 50000.00 | Paid: Rs %ld.%02ld | Balance: Rs %ld.%02ld\n",
           record->paidCents / 100, record->paidCents % 100,
           balance / 100, balance % 100);
    if (balance == 0) {
        printf("This student's fee is already fully paid.\n");
        freeRecords(&list);
        return;
    }
    if (!readPaymentAmount(&amount)) {
        freeRecords(&list);
        return;
    }
    if (amount > balance) {
        printf("Payment exceeds the remaining balance of Rs %ld.%02ld.\n",
               balance / 100, balance % 100);
        freeRecords(&list);
        return;
    }

    record->paidCents += amount;  /* ADD, never overwrite the old payment */
    if (saveRecords(&list)) {
        balance = TOTAL_CENTS - record->paidCents;
        printf("\n========== PAYMENT RECEIPT ==========\n");
        printf("Student ID  : %s\n", record->id);
        printf("Amount Paid : Rs %ld.%02ld\n", amount / 100, amount % 100);
        printf("Total Paid  : Rs %ld.%02ld\n",
               record->paidCents / 100, record->paidCents % 100);
        printf("Balance Due : Rs %ld.%02ld\n", balance / 100, balance % 100);
        printf("=====================================\n");
    } else {
        printf("Payment NOT saved.\n");
    }
    freeRecords(&list);
}

static void viewStudentFee(void) {
    FeeList fees = {0};
    StudentList students = {0};
    FeeRecord *record;
    char id[ID_LEN];
    long balance;

    printf("\n--- View Student Fee Details ---\n");
    if (!readStudentId(id)) return;
    if (!loadRecords(&fees)) return;
    if (!loadStudents(&students)) { freeRecords(&fees); return; }

    record = findRecord(&fees, id);
    if (record == NULL) {
        printf("Student not found.\n");
    } else {
        balance = TOTAL_CENTS - record->paidCents;
        printf("Student ID  : %s\n", record->id);
        printStudentDetails(studentForId(&students, id));
        printf("Total Fee   : Rs 50000.00\n");
        printf("Paid        : Rs %ld.%02ld\n",
               record->paidCents / 100, record->paidCents % 100);
        printf("Balance Due : Rs %ld.%02ld\n", balance / 100, balance % 100);
    }
    freeStudents(&students);
    freeRecords(&fees);
}

static void listAllFees(void) {
    FeeList fees = {0};
    StudentList students = {0};
    size_t i;

    if (!loadRecords(&fees)) return;
    if (!loadStudents(&students)) { freeRecords(&fees); return; }
    if (fees.count == 0) {
        printf("No fee records found.\n");
    } else {
        printf("\n%-29s %-22s %-7s %-12s %-12s %-12s\n",
               "Student ID", "Student Name", "Room", "Monthly(Rs)",
               "Paid(Rs)", "Balance(Rs)");
        printf("------------------------------------------------------------------------------------------\n");
        for (i = 0; i < fees.count; ++i) {
            const FeeRecord *fee = &fees.items[i];
            const StudentInfo *student = studentForId(&students, fee->id);
            char room[20], monthly[32], paid[32], balance[32];
            if (student) {
                snprintf(room, sizeof(room), "%d", student->roomNo);
                snprintf(monthly, sizeof(monthly), "%ld.%02ld",
                         student->monthlyCents / 100, student->monthlyCents % 100);
            } else {
                strcpy(room, "-");
                strcpy(monthly, "-");
            }
            snprintf(paid, sizeof(paid), "%ld.%02ld",
                     fee->paidCents / 100, fee->paidCents % 100);
            snprintf(balance, sizeof(balance), "%ld.%02ld",
                     (TOTAL_CENTS - fee->paidCents) / 100,
                     (TOTAL_CENTS - fee->paidCents) % 100);
            struct Student registered;
            const char *name = student ? student->name : "(not entered)";
            if (lookupStudent(fee->id, &registered) == 1) name = registered.name;
            printf("%-29.29s %-22.22s %-7s %-12s %-12s %-12s\n",
                   fee->id, name,
                   room, monthly, paid, balance);
        }
        printf("Total Fee per student: Rs 50000.00\n");
    }
    freeStudents(&students);
    freeRecords(&fees);
}

static void listPendingFees(void) {
    FeeList list = {0};
    size_t i;
    size_t students = 0;
    long long total = 0;

    if (!loadRecords(&list)) return;
    printf("\n--- Students With Pending Fees ---\n");
    printf("%-29s %s\n", "Student ID", "Due (Rs)");
    for (i = 0; i < list.count; ++i) {
        long due = TOTAL_CENTS - list.items[i].paidCents;
        if (due > 0) {
            printf("%-29s %ld.%02ld\n",
                   list.items[i].id, due / 100, due % 100);
            ++students;
            total += due;
        }
    }
    printf("%zu student(s) owe Rs %lld.%02lld in total.\n",
           students, total / 100, total % 100);
    freeRecords(&list);
}

/* Called by the team's main.c. No separate main() in this file. */
void feeMenu(void) {
    char line[128];
    int status;
    char *s, *end;
    long choice;

    for (;;) {
        printf("\n===== FEE & PAYMENT TRACKING =====\n");
        printf("1. Add Student Fee Profile (registered ID, Room No, Monthly Fee)\n");
        printf("2. Record Payment\n");
        printf("3. View Student Fee Details\n");
        printf("4. List All Students\n");
        printf("5. List Students With Pending Fees\n");
        printf("0. Back\n");
        printf("Choice: ");

        status = readLine(line, sizeof(line));
        if (status == 0) return;  /* don't loop forever on stdin EOF */
        if (status < 0) continue;
        s = trim(line);
        if (*s == '\0') {
            printf("Invalid choice.\n");
            continue;
        }
        errno = 0;
        choice = strtol(s, &end, 10);
        if (errno == ERANGE || *end != '\0' || end == s) {
            printf("Invalid choice.\n");
            continue;
        }

        switch (choice) {
            case 1: addStudentFee();   break;
            case 2: recordPayment();   break;
            case 3: viewStudentFee();  break;
            case 4: listAllFees();     break;
            case 5: listPendingFees(); break;
            case 0: return;
            default: printf("Invalid choice.\n");
        }
    }
}
