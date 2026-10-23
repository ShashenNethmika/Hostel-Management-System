#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>

#define STUDENT_ID_SIZE 30
#define STUDENT_NAME_SIZE 100
#define RECORD_LINE_SIZE 512

/* All modules use the same data directory and student ID convention. */
int configureDataDirectory(const char *directory);
FILE *openDataFile(const char *name, const char *mode);
void printDataFileError(const char *name);
void warnInvalidRecord(const char *name, size_t lineNumber);
int readDataLine(FILE *file, const char *name, char *line, size_t capacity,
                 size_t *lineNumber);
int splitFields(char *line, char **fields, int maximum);
int parseInteger(const char *text, int minimum, int maximum, int *number);
int parseMoney(const char *text, long long *cents);
void formatMoney(long long cents, char *buffer, size_t capacity);
int validDate(const char *date);
void currentDate(char date[11]);
int isValidStudentIndex(const char *id);
void normalizeStudentIndex(char *id);
int validStudentName(const char *name);
int validPhoneNumber(const char *phone);

/* Console readers consume whole lines; 0 means EOF/cancel. */
int readConsoleLine(const char *prompt, char *buffer, size_t capacity);
int readMenuChoice(int minimum, int maximum, int *choice);
int readStudentIndex(const char *prompt, char id[STUDENT_ID_SIZE]);

#endif
