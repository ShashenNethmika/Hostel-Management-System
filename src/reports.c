#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reports.h"

struct Payment {
    char studentID[20];
    float totalFee;
    float paidAmount;
    float balance;
};

void calculateAndDisplayDues() {
    FILE *file = fopen("../data/payments.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open payments.txt.\n");
        return;
    }

    struct Payment p[100];
    int count = 0;

    while (fscanf(file, "%s %f %f", p[count].studentID, &p[count].totalFee, &p[count].paidAmount) == 3) {
        p[count].balance = p[count].totalFee - p[count].paidAmount;
        count++;
    }
    fclose(file);

    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (p[j].balance < p[j+1].balance) {
                struct Payment temp = p[j];
                p[j] = p[j+1];
                p[j+1] = temp;
            }
        }
    }

    FILE *outFile = fopen("../data/unpaid_summary.txt", "w");
    if (outFile != NULL) {
        fprintf(outFile, "--- Unpaid Hostel Fees Report ---\n");
        for (int i = 0; i < count; i++) {
            if(p[i].balance > 0) {
                fprintf(outFile, "%-20s %-15.2f\n", p[i].studentID, p[i].balance);
            }
        }
        fclose(outFile);
    }

    printf("\n--- Unpaid Hostel Fees Report (Sorted by Highest Dues) ---\n");
    printf("%-20s %-15s %-15s %-15s\n", "Student ID", "Total Fee", "Paid Amount", "Balance");
    printf("-----------------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        if(p[i].balance > 0) {
            printf("%-20s %-15.2f %-15.2f %-15.2f\n", p[i].studentID, p[i].totalFee, p[i].paidAmount, p[i].balance);
        }
    }
}

void calculateOccupancyRate() {
    FILE *file = fopen("../data/rooms.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open rooms.txt.\n");
        return;
    }

    char roomNo[10];
    int capacity, occupied;
    int totalCapacity = 0, totalOccupied = 0;

    printf("\n--- Room Occupancy & Availability Report ---\n");
    printf("%-10s %-10s %-10s %-10s\n", "Room", "Capacity", "Occupied", "Available");
    printf("--------------------------------------------\n");
    
    while(fscanf(file, "%s %d %d", roomNo, &capacity, &occupied) == 3) {
        int available = capacity - occupied;
        totalCapacity += capacity;
        totalOccupied += occupied;
        printf("%-10s %-10d %-10d %-10d\n", roomNo, capacity, occupied, available);
    }
    fclose(file);
    
    if(totalCapacity > 0) {
        float occupancyRate = ((float)totalOccupied / totalCapacity) * 100.0;
        printf("--------------------------------------------\n");
        printf("Overall Occupancy Rate: %.2f%%\n", occupancyRate);
    }
}

// 3. Nava Student Leave Report
void generateLeaveReport() {
    FILE *file = fopen("../data/attendance.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open attendance.txt.\n");
        return;
    }

    char studentID[20];
    int status;
    int leaveCount = 0;

    printf("\n--- Student Leave & Defaulter Report ---\n");
    printf("%-20s %-15s\n", "Student ID", "Status");
    printf("----------------------------------------\n");

    while(fscanf(file, "%s %d", studentID, &status) == 2) {
        if(status == 0) {
            printf("%-20s %-15s\n", studentID, "On Leave");
            leaveCount++;
        }
    }
    fclose(file);
    printf("----------------------------------------\n");
    printf("Total students currently on leave: %d\n", leaveCount);
}

// 4. Nava System Summary
void generateSystemSummary() {
    FILE *file = fopen("../data/students.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open students.txt.\n");
        return;
    }

    char studentID[20];
    char name[50];
    char phone[20];
    int totalStudents = 0;

    printf("\n--- Overall System Summary ---\n");
    while(fscanf(file, "%s %s %s", studentID, name, phone) == 3) {
        totalStudents++;
    }
    fclose(file);

    printf("Total Registered Students in System: %d\n", totalStudents);
    printf("Status: System is running optimally.\n");
    printf("----------------------------------------\n");
}

void reportsMenu() {
    int choice;
    do {
        printf("\n=== REPORTS & STATISTICS MENU ===\n");
        printf("1. Unpaid Hostel Fees Report\n");
        printf("2. Room Occupancy Report\n");
        printf("3. Student Leave Report\n");
        printf("4. Overall System Summary\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        
        if (choice == 1) {
            calculateAndDisplayDues();
        } else if (choice == 2) {
            calculateOccupancyRate();
        } else if (choice == 3) {
            generateLeaveReport();
        } else if (choice == 4) {
            generateSystemSummary();
        } else if (choice != 0) {
            printf("Invalid choice!.Please try again.\n");
        }
    } while (choice != 0);
}