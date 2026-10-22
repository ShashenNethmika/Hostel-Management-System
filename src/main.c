#include <stdio.h>
#include <stdlib.h>

// ඔබේ reports.h ගොනුව මෙලෙස සම්බන්ධ කරයි
#include "reports.h" 

// අනාගතයේදී අනෙක් අයගේ ගොනු මෙතැනට එකතු වේ
// #include "student.h"
// #include "room.h"

int main() {
    int mainChoice;
    
    do {
        printf("\n=================================================\n");
        printf("   HOSTEL MANAGEMENT SYSTEM - FACULTY OF TECH\n");
        printf("=================================================\n");
        printf("1. Student Registration & Management \n");
        printf("2. Room Allocation & Management \n");
        printf("3. Attendance & Leave Management \n");
        printf("4. Fee & Payment Tracking \n");
        printf("5. Reports & Statistics \n");
        printf("0. Exit System \n");
        printf("=================================================\n");
        printf("Enter your choice: ");
        scanf("%d", &mainChoice);
        
        switch(mainChoice) {
            case 1:
                printf("\n[System] Student module is under construction...\n");
                break;
            case 2:
                printf("\n[System] Room module is under construction...\n");
                break;
            case 3:
                printf("\n[System] Attendance module is under construction...\n");
                break;
            case 4:
                printf("\n[System] Fee module is under construction...\n");
                break;
            case 5:
                // අංක 5 එබූ විට ඔබේ Reports මෙනුව ක්‍රියාත්මක වේ!
                reportsMenu(); 
                break;
            case 0:
                printf("\nExiting the Hostel Management System. Goodbye!\n");
                break;
            default:
                printf("\nInvalid choice!.Please select between 0-5.\n");
        }
    } while (mainChoice != 0);
    
    return 0;
}
