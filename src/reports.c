/* Original reports module: A.A.Shashen Nethmika (TG IT 2025 2469).
 * Adapted for shared data and interfaces. */
#include "reports.h"
#include "common.h"
#include "student.h"
#include "attendance.h"
#include "rooms.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Payment {
    char studentID[30];
    long long totalFee;
    long long paidAmount;
    long long balance;
};

static int comparePayments(const void *left, const void *right) {
    const struct Payment *a = left;
    const struct Payment *b = right;
    if (a->balance < b->balance) {
        return 1;
    }
    if (a->balance > b->balance) {
        return -1;
    }
    return strcmp(a->studentID, b->studentID);
}

void calculateAndDisplayDues(void) {
    FILE *file = openDataFile("payments.txt", "rb");
    if (file == NULL) {
        printDataFileError("payments.txt");
        return;
    }

    struct Payment *payments = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t lineNumber = 0;
    char line[RECORD_LINE_SIZE];
    int failed = 0;
    while (readDataLine(file, "payments.txt", line, sizeof(line), &lineNumber)) {
        char *fields[3];
        struct Payment payment;
        if (splitFields(line, fields, 3) != 3 ||
            !isValidStudentIndex(fields[0]) ||
            !parseMoney(fields[1], &payment.totalFee) ||
            !parseMoney(fields[2], &payment.paidAmount) ||
            payment.paidAmount > payment.totalFee) {
            warnInvalidRecord("payments.txt", lineNumber);
            continue;
        }
        strcpy(payment.studentID, fields[0]);
        normalizeStudentIndex(payment.studentID);
        payment.balance = payment.totalFee - payment.paidAmount;

        if (count == capacity) {
            if (capacity > SIZE_MAX / 2 ||
                (capacity == 0 ? 16 : capacity * 2) > SIZE_MAX / sizeof(*payments)) {
                failed = 1;
                break;
            }
            size_t nextCapacity = capacity == 0 ? 16 : capacity * 2;
            struct Payment *grown = realloc(payments, nextCapacity * sizeof(*payments));
            if (grown == NULL) {
                failed = 1;
                break;
            }
            payments = grown;
            capacity = nextCapacity;
        }
        payments[count++] = payment;
    }
    if (ferror(file)) {
        printDataFileError("payments.txt");
        failed = 1;
    } else if (failed) {
        fprintf(stderr, "Error: Not enough memory to generate the unpaid fees report.\n");
    }
    fclose(file);
    if (failed) {
        free(payments);
        return;
    }
    /* Bubble sort: highest unpaid balance first, then Student ID. */
    for (size_t end = count; end > 1; end--) {
        int swapped = 0;
        for (size_t i = 1; i < end; i++) {
            if (comparePayments(&payments[i - 1], &payments[i]) > 0) {
                struct Payment temporary = payments[i - 1];
                payments[i - 1] = payments[i];
                payments[i] = temporary;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }

    printf("\n--- Unpaid Hostel Fees Report (Sorted by Highest Dues) ---\n");
    printf("%-20s %-15s %-15s %-15s\n", "Student ID", "Total Fee", "Paid Amount", "Balance");
    printf("-----------------------------------------------------------------\n");
    size_t unpaidCount = 0;
    for (size_t i = 0; i < count; i++) {
        if (payments[i].balance > 0) {
            char total[40], paid[40], balance[40];
            formatMoney(payments[i].totalFee, total, sizeof(total));
            formatMoney(payments[i].paidAmount, paid, sizeof(paid));
            formatMoney(payments[i].balance, balance, sizeof(balance));
            printf("%-20s %-15s %-15s %-15s\n", payments[i].studentID,
                   total, paid, balance);
            unpaidCount++;
        }
    }
    if (unpaidCount == 0) {
        printf("No unpaid hostel fees found.\n");
    }

    FILE *output = openDataFile("unpaid_summary.txt", "w");
    if (output == NULL) {
        printDataFileError("unpaid_summary.txt");
    } else {
        int saved = fprintf(output, "--- Unpaid Hostel Fees Report ---\n") >= 0;
        for (size_t i = 0; saved && i < count; i++) {
            if (payments[i].balance > 0) {
                char balance[40];
                formatMoney(payments[i].balance, balance, sizeof(balance));
                saved = fprintf(output, "%-20s %-15s\n", payments[i].studentID,
                                balance) >= 0;
            }
        }
        if (fclose(output) != 0) {
            saved = 0;
        }
        if (!saved) {
            printDataFileError("unpaid_summary.txt");
        }
    }
    free(payments);
}

void calculateOccupancyRate(void) {
    FILE *file = openDataFile("rooms.txt", "rb");
    if (file == NULL) {
        printDataFileError("rooms.txt");
        return;
    }

    unsigned long long totalCapacity = 0;
    unsigned long long totalOccupied = 0;
    size_t lineNumber = 0;
    char line[RECORD_LINE_SIZE];
    printf("\n--- Room Occupancy & Availability Report ---\n");
    printf("%-10s %-10s %-10s %-10s\n", "Room", "Capacity", "Occupied", "Available");
    printf("--------------------------------------------\n");
    while (readDataLine(file, "rooms.txt", line, sizeof(line), &lineNumber)) {
        char *fields[3];
        int capacity;
        int occupied;
        if (splitFields(line, fields, 3) != 3 || strlen(fields[0]) >= 10 ||
            !parseInteger(fields[1], 0, INT_MAX, &capacity) ||
            !parseInteger(fields[2], 0, capacity, &occupied) ||
            ULLONG_MAX - totalCapacity < (unsigned int)capacity) {
            warnInvalidRecord("rooms.txt", lineNumber);
            continue;
        }
        totalCapacity += (unsigned int)capacity;
        totalOccupied += (unsigned int)occupied;
        printf("%-10s %-10d %-10d %-10d\n", fields[0], capacity, occupied,
               capacity - occupied);
    }
    int failed = ferror(file);
    if (failed) {
        printDataFileError("rooms.txt");
    }
    fclose(file);
    if (failed) {
        return;
    }
    double rate = totalCapacity == 0 ? 0.0 :
                  (double)totalOccupied / (double)totalCapacity * 100.0;
    printf("--------------------------------------------\n");
    printf("Overall Occupancy Rate: %.2f%%\n", rate);
    if (totalCapacity == 0) {
        printf("No room capacity is currently available.\n");
    }
}

void generateLeaveReport(void) {
    displayLeaveReport(0);
}

void generateSystemSummary(void) {
    FILE *file = openDataFile("students.txt", "rb");
    if (file == NULL) {
        printDataFileError("students.txt");
        return;
    }

    struct Student student;
    size_t lineNumber = 0;
    size_t totalStudents = 0;
    while (readStudentRecord(file, &student, &lineNumber)) {
        totalStudents++;
    }
    int failed = ferror(file);
    if (failed) {
        printDataFileError("students.txt");
    }
    fclose(file);
    if (!failed) {
        printf("\n--- Overall System Summary ---\n");
        printf("Total Registered Students in System: %zu\n", totalStudents);
        printf("----------------------------------------\n");
    }
}

void reportsMenu(void) {
    for (;;) {
        int choice;
        printf("\n=== REPORTS & STATISTICS MENU ===\n");
        printf("1. Unpaid Hostel Fees Report\n");
        printf("2. Room Occupancy Report\n");
        printf("3. Student Leave Report\n");
        printf("4. Overall System Summary\n");
        printf("5. Overdue Leave Report\n");
        printf("0. Back to Main Menu\n");
        if (!readMenuChoice(0, 5, &choice) || choice == 0) {
            return;
        }
        switch (choice) {
            case 1: calculateAndDisplayDues(); break;
            case 2: calculateOccupancyRate(); break;
            case 3: generateLeaveReport(); break;
            case 4: generateSystemSummary(); break;
            case 5: displayOverdueLeaves(); break;
        }
    }
}
