#ifndef STUDENT_H
#define STUDENT_H

#include "common.h"

struct Student {
    char studentID[STUDENT_ID_SIZE];
    char name[STUDENT_NAME_SIZE];
    char phone[11];
};

/* A validated snapshot for one operation; reuse it for every displayed row. */
struct StudentRegister {
    struct Student *items;
    size_t count;
};

int loadStudentRegister(struct StudentRegister *students);
void freeStudentRegister(struct StudentRegister *students);
const struct Student *findRegisteredStudent(const struct StudentRegister *students, const char *id);
void studentMenu(void);
int readStudentRecord(FILE *file, struct Student *student, size_t *lineNumber);
/* 1: found, 0: not registered, -1: file error. */
int lookupStudent(const char *id, struct Student *student);

#endif
