/* Original main menu: A.A.Shashen Nethmika (TG IT 2025 2469).
 * Adapted for the integrated application. */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "reports.h"
#include "student.h"
#include "rooms.h"
#include "attendance.h"
#include "fee.h"

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [data-directory]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (!configureDataDirectory(argc == 2 ? argv[1] : NULL)) {
        return EXIT_FAILURE;
    }
    if (!recoverRoomTransaction()) {
        return EXIT_FAILURE;
    }
    if (!initializeDataFiles()) {
        return EXIT_FAILURE;
    }

    for (;;) {
        int choice;
        printf("\n=================================================\n");
        printf("   HOSTEL MANAGEMENT SYSTEM - FACULTY OF TECH\n");
        printf("=================================================\n");
        printf("1. Student Registration & Management\n");
        printf("2. Room Allocation & Management\n");
        printf("3. Attendance & Leave Management\n");
        printf("4. Fee & Payment Tracking\n");
        printf("5. Reports & Statistics\n");
        printf("0. Exit System\n");
        printf("=================================================\n");

        if (!readMenuChoice(0, 5, &choice) || choice == 0) {
            printf("\nExiting the Hostel Management System. Goodbye!\n");
            return EXIT_SUCCESS;
        }

        switch (choice) {
            case 1:
                studentMenu();
                break;
            case 2:
                roomMenu();
                break;
            case 3:
                attendanceMenu();
                break;
            case 4:
                feeMenu();
                break;
            case 5:
                reportsMenu();
                break;
        }
    }
}
