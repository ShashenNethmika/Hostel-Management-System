#ifndef STUDENT_H
#define STUDENT_H

#include "common.h"

struct Student {
    char studentID[STUDENT_ID_SIZE];
    char name[STUDENT_NAME_SIZE];
    char phone[11];
};

void studentMenu(void);
int readStudentRecord(FILE *file, struct Student *student, size_t *lineNumber);
/* 1: found, 0: not registered, -1: file error. */
int lookupStudent(const char *id, struct Student *student);

#endif
