/* Original fees module: H.G.Chesmi Tharushika (TG IT 2025 2465).
 * Adapted for shared student records, actual allocations, and exact-cent money.
 * payments.txt: StudentID TotalFee PaidAmount
 * fee_students.txt: StudentID|MonthlyFee
 * Legacy four-field profiles are validated and migrated without changing payments.
 */
#include "fee.h"
#include "student.h"
#include "rooms.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define PAYMENT_FILE "payments.txt"
#define PROFILE_FILE "fee_students.txt"

struct FeeRecord {
    char id[STUDENT_ID_SIZE];
    long long paidCents;
};

struct FeeList {
    struct FeeRecord *items;
    size_t count;
    size_t capacity;
};

struct FeeProfile {
    char id[STUDENT_ID_SIZE];
    long long monthlyCents;
};

struct ProfileList {
    struct FeeProfile *items;
    size_t count;
    size_t capacity;
};

static int compareFeeIds(const void *left, const void *right) {
    const struct FeeRecord *a = left, *b = right;
    return strcmp(a->id, b->id);
}

static int compareProfileIds(const void *left, const void *right) {
    const struct FeeProfile *a = left, *b = right;
    return strcmp(a->id, b->id);
}

static int pushFee(struct FeeList *list, const struct FeeRecord *record) {
    if (list->count == list->capacity) {
        size_t next = list->capacity == 0 ? 16 : list->capacity * 2;
        if (next < list->capacity || next > (size_t)-1 / sizeof(*list->items)) {
            fprintf(stderr, "Too many payment records.\n");
            return 0;
        }
        struct FeeRecord *grown = realloc(list->items, next * sizeof(*list->items));
        if (grown == NULL) { fprintf(stderr, "Cannot allocate payment records.\n"); return 0; }
        list->items = grown;
        list->capacity = next;
    }
    list->items[list->count++] = *record;
    return 1;
}

static int pushProfile(struct ProfileList *list, const struct FeeProfile *profile) {
    if (list->count == list->capacity) {
        size_t next = list->capacity == 0 ? 16 : list->capacity * 2;
        if (next < list->capacity || next > (size_t)-1 / sizeof(*list->items)) {
            fprintf(stderr, "Too many fee profiles.\n");
            return 0;
        }
        struct FeeProfile *grown = realloc(list->items, next * sizeof(*list->items));
        if (grown == NULL) { fprintf(stderr, "Cannot allocate fee profiles.\n"); return 0; }
        list->items = grown;
        list->capacity = next;
    }
    list->items[list->count++] = *profile;
    return 1;
}

static void freeFees(struct FeeList *list) {
    free(list->items);
    *list = (struct FeeList){0};
}

static void freeProfiles(struct ProfileList *list) {
    free(list->items);
    *list = (struct ProfileList){0};
}

/* Load the entire financial file before editing. Duplicate detection is sorted
 * rather than re-scanning all previous records for every line. */
static int loadFees(struct FeeList *list) {
    FILE *file = fopen(PAYMENT_FILE, "rb");
    if (file == NULL) {
        if (errno == ENOENT) return 1;
        printDataFileError(PAYMENT_FILE);
        return 0;
    }
    char line[RECORD_LINE_SIZE];
    size_t lineNumber = 0;
    int status, loaded = 1;
    while ((status = readTextLine(file, line, sizeof(line))) != TEXT_LINE_EOF) {
        lineNumber++;
        char *fields[3];
        struct FeeRecord record;
        long long total;
        if (status != TEXT_LINE_OK || strchr(line, '|') != NULL) {
            fprintf(stderr, "Invalid payments.txt: malformed line %zu.\n", lineNumber);
            loaded = 0;
            break;
        }
        int count = splitFields(line, fields, 3);
        if (count == 0) continue;
        if (count != 3 || !isValidStudentIndex(fields[0]) ||
            !parseMoney(fields[1], &total) ||
            !parseMoney(fields[2], &record.paidCents) || record.paidCents > total) {
            fprintf(stderr, "Invalid payments.txt: bad record on line %zu.\n", lineNumber);
            loaded = 0;
            break;
        }
        if (total != HOSTEL_FEE_CENTS) {
            char expected[40];
            formatMoney(HOSTEL_FEE_CENTS, expected, sizeof(expected));
            fprintf(stderr, "Invalid payments.txt: total fee on line %zu must be Rs %s. "
                    "Existing payment records are unchanged; review old amounts before continuing.\n",
                    lineNumber, expected);
            loaded = 0;
            break;
        }
        strcpy(record.id, fields[0]);
        normalizeStudentIndex(record.id);
        if (!pushFee(list, &record)) { loaded = 0; break; }
    }
    if (ferror(file)) loaded = 0;
    if (fclose(file) != 0 && loaded) {
        printDataFileError(PAYMENT_FILE);
        loaded = 0;
    }
    if (loaded && list->count > 1) qsort(list->items, list->count, sizeof(*list->items), compareFeeIds);
    for (size_t i = 1; loaded && i < list->count; i++) {
        if (strcmp(list->items[i - 1].id, list->items[i].id) == 0) {
            fprintf(stderr, "Invalid payments.txt: duplicate Student ID %s.\n", list->items[i].id);
            loaded = 0;
        }
    }
    if (!loaded) freeFees(list);
    return loaded;
}

/* Accept current ID|MonthlyFee and old ID|Name|RoomNo|MonthlyFee records.
 * Old names/rooms are validated, but never used as current business facts. */
static int loadProfiles(struct ProfileList *list, int *legacyFound) {
    *legacyFound = 0;
    FILE *file = fopen(PROFILE_FILE, "rb");
    if (file == NULL) {
        if (errno == ENOENT) return 1;
        printDataFileError(PROFILE_FILE);
        return 0;
    }
    char line[RECORD_LINE_SIZE];
    size_t lineNumber = 0;
    int status, loaded = 1;
    while ((status = readTextLine(file, line, sizeof(line))) != TEXT_LINE_EOF) {
        lineNumber++;
        char *fields[4];
        struct FeeProfile profile;
        if (status != TEXT_LINE_OK) {
            fprintf(stderr, "Invalid fee_students.txt: malformed line %zu.\n", lineNumber);
            loaded = 0;
            break;
        }
        int hasSeparator = strchr(line, '|') != NULL;
        int count = splitFields(line, fields, 4);
        if (count == 0) continue;
        int room;
        if (!hasSeparator || (count != 2 && count != 4) ||
            !isValidStudentIndex(fields[0]) ||
            (count == 4 && (!validStudentName(fields[1]) ||
                            !parseInteger(fields[2], 1, INT_MAX, &room))) ||
            !parseMoney(fields[count == 4 ? 3 : 1], &profile.monthlyCents) ||
            profile.monthlyCents <= 0 || profile.monthlyCents > HOSTEL_FEE_CENTS) {
            fprintf(stderr, "Invalid fee_students.txt: bad record on line %zu.\n", lineNumber);
            loaded = 0;
            break;
        }
        strcpy(profile.id, fields[0]);
        normalizeStudentIndex(profile.id);
        if (count == 4) *legacyFound = 1;
        if (!pushProfile(list, &profile)) { loaded = 0; break; }
    }
    if (ferror(file)) loaded = 0;
    if (fclose(file) != 0 && loaded) {
        printDataFileError(PROFILE_FILE);
        loaded = 0;
    }
    if (loaded && list->count > 1) qsort(list->items, list->count, sizeof(*list->items), compareProfileIds);
    for (size_t i = 1; loaded && i < list->count; i++) {
        if (strcmp(list->items[i - 1].id, list->items[i].id) == 0) {
            fprintf(stderr, "Invalid fee_students.txt: duplicate Student ID %s.\n", list->items[i].id);
            loaded = 0;
        }
    }
    if (!loaded) freeProfiles(list);
    return loaded;
}

static struct FeeRecord *findFee(struct FeeList *list, const char *id) {
    size_t low = 0, high = list->count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        int compared = strcmp(id, list->items[middle].id);
        if (compared == 0) return &list->items[middle];
        if (compared < 0) high = middle;
        else low = middle + 1;
    }
    return NULL;
}

static const struct FeeProfile *findProfile(const struct ProfileList *list, const char *id) {
    size_t low = 0, high = list->count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        int compared = strcmp(id, list->items[middle].id);
        if (compared == 0) return &list->items[middle];
        if (compared < 0) high = middle;
        else low = middle + 1;
    }
    return NULL;
}

static int saveFees(struct FeeList *list) {
    if (list->count > 1) qsort(list->items, list->count, sizeof(*list->items), compareFeeIds);
    char temporary[RECORD_LINE_SIZE];
    FILE *file = createDataTempFile(PAYMENT_FILE, temporary);
    if (file == NULL) { printDataFileError(PAYMENT_FILE); return 0; }
    char total[40];
    formatMoney(HOSTEL_FEE_CENTS, total, sizeof(total));
    for (size_t i = 0; i < list->count; i++) {
        char paid[40];
        formatMoney(list->items[i].paidCents, paid, sizeof(paid));
        if (fprintf(file, "%s %s %s\n", list->items[i].id, total, paid) < 0) {
            fclose(file); remove(temporary); printDataFileError(PAYMENT_FILE); return 0;
        }
    }
    if (!commitDataTempFile(file, temporary, PAYMENT_FILE)) {
        printDataFileError(PAYMENT_FILE);
        return 0;
    }
    return 1;
}

static int saveProfiles(struct ProfileList *list) {
    if (list->count > 1) qsort(list->items, list->count, sizeof(*list->items), compareProfileIds);
    char temporary[RECORD_LINE_SIZE];
    FILE *file = createDataTempFile(PROFILE_FILE, temporary);
    if (file == NULL) { printDataFileError(PROFILE_FILE); return 0; }
    for (size_t i = 0; i < list->count; i++) {
        char monthly[40];
        formatMoney(list->items[i].monthlyCents, monthly, sizeof(monthly));
        if (fprintf(file, "%s|%s\n", list->items[i].id, monthly) < 0) {
            fclose(file); remove(temporary); printDataFileError(PROFILE_FILE); return 0;
        }
    }
    if (!commitDataTempFile(file, temporary, PROFILE_FILE)) {
        printDataFileError(PROFILE_FILE);
        return 0;
    }
    return 1;
}

int migrateFeeProfiles(void) {
    struct ProfileList profiles = {0};
    int legacy;
    if (!loadProfiles(&profiles, &legacy)) return 0;
    int saved = !legacy || saveProfiles(&profiles);
    freeProfiles(&profiles);
    return saved;
}

static int readPositiveAmount(const char *prompt, long long maximum, long long *cents) {
    char line[128], limit[40];
    formatMoney(maximum, limit, sizeof(limit));
    while (readConsoleLine(prompt, line, sizeof(line))) {
        if (parseMoney(line, cents) && *cents > 0 && *cents <= maximum) return 1;
        fprintf(stderr, "Invalid amount. Enter Rs 0.01-%s with up to 2 decimals.\n", limit);
    }
    return 0;
}

/* A display gets one validated register and allocation snapshot, not one file
 * load per payment row. Names and rooms are always current shared data. */
static int loadSharedSnapshot(struct StudentRegister *students) {
    if (!loadStudentRegister(students)) return 0;
    if (!loadRoomState()) { freeStudentRegister(students); return 0; }
    return 1;
}

static void printStudentDetails(const char *id, const struct FeeProfile *profile,
                                const struct StudentRegister *students) {
    const struct Student *student = findRegisteredStudent(students, id);
    printf("Student Name: %s\n", student != NULL ? student->name : "(not registered)");
    int room;
    if (lookupStudentRoom(id, &room)) printf("Room No     : %d\n", room);
    else printf("Room No     : Unallocated\n");
    if (profile != NULL) {
        char monthly[40];
        formatMoney(profile->monthlyCents, monthly, sizeof(monthly));
        printf("Monthly Fee : Rs %s\n", monthly);
    } else printf("Monthly Fee : Not configured\n");
}

static void addStudentFee(void) {
    struct StudentRegister students = {0};
    struct FeeList fees = {0};
    struct ProfileList profiles = {0};
    struct FeeProfile profile;
    int legacy;
    printf("\n--- Add Student Fee Profile ---\n");
    if (!readStudentIndex("Student ID: ", profile.id)) return;
    if (!loadSharedSnapshot(&students)) return;
    if (findRegisteredStudent(&students, profile.id) == NULL) {
        fprintf(stderr, "Student not registered. Register through Student Management first.\n");
        goto cleanup;
    }
    int room;
    if (!lookupStudentRoom(profile.id, &room)) {
        fprintf(stderr, "Student has no room allocation. Allocate a room through Room Management first.\n");
        goto cleanup;
    }
    if (!loadFees(&fees) || !loadProfiles(&profiles, &legacy)) goto cleanup;
    if (findProfile(&profiles, profile.id) != NULL) {
        fprintf(stderr, "Student %s already has a fee profile. Duplicate not added.\n", profile.id);
        goto cleanup;
    }
    struct FeeRecord *existing = findFee(&fees, profile.id);
    int existingPayment = existing != NULL;
    long long previousPaid = existingPayment ? existing->paidCents : 0;
    if (!readPositiveAmount("Monthly Fee (Rs): ", HOSTEL_FEE_CENTS, &profile.monthlyCents)) goto cleanup;
    if (!pushProfile(&profiles, &profile)) goto cleanup;
    /* Persist payment first. If profile saving fails, retry safely completes the
     * profile without resetting any payment that already exists. */
    if (!existingPayment) {
        struct FeeRecord record = {0};
        strcpy(record.id, profile.id);
        if (!pushFee(&fees, &record) || !saveFees(&fees)) goto cleanup;
    }
    if (!saveProfiles(&profiles)) {
        fprintf(stderr, "Profile not saved. Existing payments remain safe; retry to complete the profile.\n");
        goto cleanup;
    }
    printf("\nStudent added successfully!\nStudent ID  : %s\n", profile.id);
    printStudentDetails(profile.id, &profile, &students);
    char total[40];
    formatMoney(HOSTEL_FEE_CENTS, total, sizeof(total));
    printf("Total Fee   : Rs %s\n", total);
    char paid[40];
    formatMoney(previousPaid, paid, sizeof(paid));
    if (existingPayment) printf("Previous Paid: Rs %s (preserved)\n", paid);
    else printf("Paid Amount : Rs 0.00\n");
cleanup:
    freeStudentRegister(&students);
    freeFees(&fees);
    freeProfiles(&profiles);
}

static void recordPayment(void) {
    char id[STUDENT_ID_SIZE];
    printf("\n--- Record Payment ---\n");
    if (!readStudentIndex("Student ID: ", id)) return;
    struct StudentRegister students = {0};
    if (!loadStudentRegister(&students)) return;
    int registered = findRegisteredStudent(&students, id) != NULL;
    freeStudentRegister(&students);
    if (!registered) {
        fprintf(stderr, "Student not registered. Register through Student Management first.\n");
        return;
    }
    struct FeeList fees = {0};
    if (!loadFees(&fees)) return;
    struct FeeRecord *record = findFee(&fees, id);
    if (record == NULL) {
        fprintf(stderr, "Student fee record not found. Add the fee profile first.\n");
        freeFees(&fees);
        return;
    }
    long long balance = HOSTEL_FEE_CENTS - record->paidCents;
    if (balance == 0) {
        printf("This student's fee is already fully paid.\n");
        freeFees(&fees);
        return;
    }
    char fixedTotal[40], paid[40], due[40];
    formatMoney(HOSTEL_FEE_CENTS, fixedTotal, sizeof(fixedTotal));
    formatMoney(record->paidCents, paid, sizeof(paid));
    formatMoney(balance, due, sizeof(due));
    printf("Total: Rs %s | Paid: Rs %s | Balance: Rs %s\n", fixedTotal, paid, due);
    long long amount;
    if (!readPositiveAmount("Amount to pay (Rs): ", balance, &amount)) { freeFees(&fees); return; }
    record->paidCents += amount;
    long long totalPaid = record->paidCents;
    if (saveFees(&fees)) {
        char payment[40], total[40];
        formatMoney(amount, payment, sizeof(payment));
        formatMoney(totalPaid, total, sizeof(total));
        formatMoney(HOSTEL_FEE_CENTS - totalPaid, due, sizeof(due));
        printf("\n========== PAYMENT RECEIPT ==========\nStudent ID  : %s\n", id);
        printf("Amount Paid : Rs %s\nTotal Paid  : Rs %s\nBalance Due : Rs %s\n", payment, total, due);
        printf("=====================================\n");
    }
    freeFees(&fees);
}

static void viewStudentFee(void) {
    char id[STUDENT_ID_SIZE];
    printf("\n--- View Student Fee Details ---\n");
    if (!readStudentIndex("Student ID: ", id)) return;
    struct FeeList fees = {0};
    struct ProfileList profiles = {0};
    struct StudentRegister students = {0};
    int legacy;
    if (!loadFees(&fees) || !loadProfiles(&profiles, &legacy) || !loadSharedSnapshot(&students)) goto cleanup;
    struct FeeRecord *record = findFee(&fees, id);
    if (record == NULL) fprintf(stderr, "Student fee record not found.\n");
    else {
        printf("Student ID  : %s\n", id);
        printStudentDetails(id, findProfile(&profiles, id), &students);
        char fixedTotal[40], paid[40], due[40];
        formatMoney(HOSTEL_FEE_CENTS, fixedTotal, sizeof(fixedTotal));
        formatMoney(record->paidCents, paid, sizeof(paid));
        formatMoney(HOSTEL_FEE_CENTS - record->paidCents, due, sizeof(due));
        printf("Total Fee   : Rs %s\nPaid        : Rs %s\nBalance Due : Rs %s\n", fixedTotal, paid, due);
    }
cleanup:
    freeFees(&fees);
    freeProfiles(&profiles);
    freeStudentRegister(&students);
}

static void listAllFees(void) {
    struct FeeList fees = {0};
    struct ProfileList profiles = {0};
    struct StudentRegister students = {0};
    int legacy;
    if (!loadFees(&fees) || !loadProfiles(&profiles, &legacy) || !loadSharedSnapshot(&students)) goto cleanup;
    if (fees.count == 0) printf("No fee records found.\n");
    else {
        printf("\n%-29s %-22s %-12s %-12s %-12s %-12s\n",
               "Student ID", "Student Name", "Room", "Monthly(Rs)", "Paid(Rs)", "Balance(Rs)");
        for (size_t i = 0; i < fees.count; i++) {
            const struct FeeRecord *fee = &fees.items[i];
            const struct Student *student = findRegisteredStudent(&students, fee->id);
            const struct FeeProfile *profile = findProfile(&profiles, fee->id);
            char roomText[20], monthly[40] = "-", paid[40], due[40];
            int room;
            if (lookupStudentRoom(fee->id, &room)) snprintf(roomText, sizeof(roomText), "%d", room);
            else strcpy(roomText, "Unallocated");
            if (profile != NULL) formatMoney(profile->monthlyCents, monthly, sizeof(monthly));
            formatMoney(fee->paidCents, paid, sizeof(paid));
            formatMoney(HOSTEL_FEE_CENTS - fee->paidCents, due, sizeof(due));
            printf("%-29s %-22s %-12s %-12s %-12s %-12s\n", fee->id,
                   student != NULL ? student->name : "(not registered)", roomText, monthly, paid, due);
        }
        char fixedTotal[40];
        formatMoney(HOSTEL_FEE_CENTS, fixedTotal, sizeof(fixedTotal));
        printf("Total Fee per student: Rs %s\n", fixedTotal);
    }
cleanup:
    freeFees(&fees);
    freeProfiles(&profiles);
    freeStudentRegister(&students);
}

static void listPendingFees(void) {
    struct FeeList fees = {0};
    if (!loadFees(&fees)) return;
    long long total = 0;
    size_t students = 0;
    printf("\n--- Students With Pending Fees ---\n%-29s %s\n", "Student ID", "Due (Rs)");
    for (size_t i = 0; i < fees.count; i++) {
        long long due = HOSTEL_FEE_CENTS - fees.items[i].paidCents;
        if (due > 0) {
            if (total > LLONG_MAX - due) {
                fprintf(stderr, "Pending fee total is too large to represent.\n");
                freeFees(&fees);
                return;
            }
            char amount[40];
            formatMoney(due, amount, sizeof(amount));
            printf("%-29s %s\n", fees.items[i].id, amount);
            total += due;
            students++;
        }
    }
    char amount[40];
    formatMoney(total, amount, sizeof(amount));
    printf("%zu student(s) owe Rs %s in total.\n", students, amount);
    freeFees(&fees);
}

void feeMenu(void) {
    if (!migrateFeeProfiles()) return;
    for (;;) {
        printf("\n===== FEE & PAYMENT TRACKING =====\n");
        printf("1. Add Student Fee Profile (registered and allocated students)\n");
        printf("2. Record Payment\n3. View Student Fee Details\n4. List All Students\n");
        printf("5. List Students With Pending Fees\n0. Back\n");
        int choice;
        if (!readMenuChoice(0, 5, &choice) || choice == 0) return;
        switch (choice) {
            case 1: addStudentFee(); break;
            case 2: recordPayment(); break;
            case 3: viewStudentFee(); break;
            case 4: listAllFees(); break;
            case 5: listPendingFees(); break;
        }
    }
}
