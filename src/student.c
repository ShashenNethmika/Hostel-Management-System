#include <stdio.h>
#include <string.h>

struct Student {
    char index[30];
    char name[50];
    char phone[15];
};

int isIndexExist(char checkIndex[]) {
    FILE *fp = fopen("students.txt", "r");
    if (fp == NULL) {
        return 0;
    }

    struct Student temp;
    while (fscanf(fp, "%s %s %s", temp.index, temp.name, temp.phone) != EOF) {
        if (strcmp(temp.index, checkIndex) == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

void registerStudent() {
    struct Student st;
    int i, valid = 1, nvalid = 1, pvalid = 1;

    printf("\nEnter your index Number: ");
    scanf("%s", st.index);

    if (strlen(st.index) != 10) {
        valid = 0;
    } else {
        for (i = 0; i < 10; i++) {
            if (st.index[i] == ' ') {
                valid = 0;
                break;
            }
        }
    }

    if (valid == 0 || !(st.index[0] == 'T' || st.index[0] == 't') || !(st.index[1] == 'G' || st.index[1] == 'g')) {
        printf("Invalid Index Number! Must be 10 characters and start with TG.\n");
        return;
    }

    if (isIndexExist(st.index)) {
        printf("Student already registered!\n");
        return;
    }

    printf("Enter your Name: ");
    scanf("%s", st.name);

    int count = strlen(st.name);
    for (i = 0; i < count; i++) {
        if (st.name[i] >= '0' && st.name[i] <= '9') {
            nvalid = 0;
            break;
        }
    }

    if (nvalid == 0) {
        printf("Invalid Name! Numbers are not allowed in name.\n");
        return;
    }

    printf("Enter your phone number: ");
    scanf("%s", st.phone);

    if (strlen(st.phone) != 10) {
        pvalid = 0;
    } else {
        for (i = 0; i < 10; i++) {
            if (st.phone[i] < '0' || st.phone[i] > '9') {
                pvalid = 0;
                break;
            }
        }
    }

    if (pvalid == 0) {
        printf("Invalid Phone Number! Must be exactly 10 digits.\n");
        return;
    }

    FILE *fp = fopen("students.txt", "a");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(fp, "%s %s %s\n", st.index, st.name, st.phone);
    fclose(fp);

    printf("\n--- Validation Successful & Saved to File! ---\n");
    printf("Index : %s\n", st.index);
    printf("Name  : %s\n", st.name);
    printf("Phone : %s\n", st.phone);
}

void displayStudents() {
    FILE *fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("\nNo student records found!\n");
        return;
    }

    struct Student st;
    int count = 0;

    printf("\n======================================================\n");
    printf("                  REGISTERED STUDENTS                 \n");
    printf("======================================================\n");
    printf("%-20s %-20s %-15s\n", "Index Number", "Name", "Phone Number");
    printf("------------------------------------------------------\n");

    while (fscanf(fp, "%s %s %s", st.index, st.name, st.phone) != EOF) {
        printf("%-20s %-20s %-15s\n", st.index, st.name, st.phone);
        count++;
    }

    fclose(fp);

    if (count == 0) {
        printf("No records in file.\n");
    } else {
        printf("------------------------------------------------------\n");
        printf("Total Students: %d\n", count);
    }
}

void searchStudent() {
    FILE *fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("\nNo student records found!\n");
        return;
    }

    char searchIndex[30];
    printf("\nEnter Index Number to search: ");
    scanf("%s", searchIndex);

    struct Student st;
    int found = 0;

    while (fscanf(fp, "%s %s %s", st.index, st.name, st.phone) != EOF) {
        if (strcmp(st.index, searchIndex) == 0) {
            printf("\n--- Student Found ---\n");
            printf("Index : %s\n", st.index);
            printf("Name  : %s\n", st.name);
            printf("Phone : %s\n", st.phone);
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (!found) {
        printf("Student with Index %s not found!\n", searchIndex);
    }
}

int main() {
    int choice;

    while (1) {
        printf("\n====================================\n");
        printf("    STUDENT MANAGEMENT SYSTEM      \n");
        printf("====================================\n");
        printf("1. Register Student\n");
        printf("2. Display All Students\n");
        printf("3. Search Student\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                registerStudent();
                break;
            case 2:
                displayStudents();
                break;
            case 3:
                searchStudent();
                break;
            case 0:
                printf("Exiting system...\n");
                return 0;
            default:
                printf("Invalid choice! Try again.\n");
        }
    }

    return 0;
}
