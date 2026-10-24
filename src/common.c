#define _POSIX_C_SOURCE 200809L
#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *trimText(char *text) {
    while (isspace((unsigned char)*text)) text++;
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

int configureDataDirectory(const char *directory) {
    if (directory != NULL) {
        if (chdir(directory) == 0) return 1;
        perror("Cannot open data directory");
        return 0;
    }
    if (chdir("data") == 0 || chdir("../data") == 0) return 1;

    /* Ubuntu: also support launching build/hostel from another directory. */
    char executable[4096];
    ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
    if (length > 0 && (size_t)length < sizeof(executable) - 1) {
        executable[length] = '\0';
        char *slash = strrchr(executable, '/');
        if (slash != NULL) {
            *slash = '\0';
            char path[4096];
            int n = snprintf(path, sizeof(path), "%s/../data", executable);
            if (n > 0 && (size_t)n < sizeof(path) && chdir(path) == 0) return 1;
        }
    }
    fprintf(stderr, "Cannot find data/. Run from the project root or specify a data directory.\n");
    return 0;
}

FILE *openDataFile(const char *name, const char *mode) {
    return fopen(name, mode);
}

void printDataFileError(const char *name) {
    fprintf(stderr, "Error reading or writing %s.\n", name);
}

void warnInvalidRecord(const char *name, size_t lineNumber) {
    fprintf(stderr, "Warning: ignoring invalid %s record on line %zu.\n", name, lineNumber);
}

int readTextLine(FILE *file, char *line, size_t capacity) {
    if (capacity < 2 || capacity > INT_MAX) return TEXT_LINE_ERROR;
    size_t length = 0;
    int ch, invalid = 0, sawByte = 0, pendingCR = 0;
    while ((ch = fgetc(file)) != EOF) {
        sawByte = 1;
        if (ch == '\n') break;
        /* Keep an interior CR, but do not charge the CRLF terminator against
         * payload capacity. This also accepts a final line ending in CR. */
        if (pendingCR) {
            if (length < capacity - 1) line[length++] = '\r';
            else invalid = 1;
        }
        pendingCR = ch == '\r';
        if (pendingCR) continue;
        if (ch == '\0') invalid = 1;
        if (length < capacity - 1) line[length++] = (char)ch;
        else invalid = 1;
    }
    line[length] = '\0';
    if (ferror(file)) return TEXT_LINE_ERROR;
    if (!sawByte) return TEXT_LINE_EOF;
    if (invalid) return TEXT_LINE_INVALID;
    return TEXT_LINE_OK;
}

int readDataLine(FILE *file, const char *name, char *line, size_t capacity,
                 size_t *lineNumber) {
    int status;
    while ((status = readTextLine(file, line, capacity)) != TEXT_LINE_EOF &&
           status != TEXT_LINE_ERROR) {
        (*lineNumber)++;
        if (status == TEXT_LINE_INVALID) {
            warnInvalidRecord(name, *lineNumber);
            continue;
        }
        char *trimmed = trimText(line);
        if (*trimmed == '\0') continue;
        memmove(line, trimmed, strlen(trimmed) + 1);
        return 1;
    }
    return 0;
}

FILE *createDataTempFile(const char *destination, char temporary[RECORD_LINE_SIZE]) {
    int length = snprintf(temporary, RECORD_LINE_SIZE, "%s.tmp.XXXXXX", destination);
    if (length < 0 || length >= RECORD_LINE_SIZE) return NULL;
    int fd = mkstemp(temporary);
    if (fd < 0) return NULL;
    FILE *file = fdopen(fd, "w");
    if (file == NULL) { close(fd); remove(temporary); }
    return file;
}

int commitDataTempFile(FILE *file, const char *temporary, const char *destination) {
    int saved = !ferror(file);
    if (fflush(file) != 0) saved = 0;
    if (saved && fsync(fileno(file)) != 0) saved = 0;
    if (fclose(file) != 0) saved = 0;
    if (saved && rename(temporary, destination) == 0) return 1;
    remove(temporary);
    return 0;
}

int appendDataRecord(const char *destination, const char *record) {
    FILE *source = fopen(destination, "rb");
    if (source == NULL && errno != ENOENT) return 0;
    char temporary[RECORD_LINE_SIZE];
    FILE *output = createDataTempFile(destination, temporary);
    if (output == NULL) { if (source != NULL) fclose(source); return 0; }
    int lastByte = '\n', saved = 1;
    if (source != NULL) {
        unsigned char buffer[4096];
        size_t count;
        while ((count = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            if (fwrite(buffer, 1, count, output) != count) { saved = 0; break; }
            lastByte = buffer[count - 1];
        }
        if (ferror(source)) saved = 0;
        if (fclose(source) != 0) saved = 0;
    }
    if (lastByte != '\n' && fputc('\n', output) == EOF) saved = 0;
    if (fputs(record, output) == EOF) saved = 0;
    if (!saved) { fclose(output); remove(temporary); return 0; }
    return commitDataTempFile(output, temporary, destination);
}

int splitFields(char *line, char **fields, int maximum) {
    int count = 0;
    if (strchr(line, '|') != NULL) {
        char *start = line;
        for (;;) {
            if (count == maximum) return maximum + 1;
            char *next = strchr(start, '|');
            if (next != NULL) *next = '\0';
            fields[count++] = trimText(start);
            if (next == NULL) break;
            start = next + 1;
        }
    } else {
        char *field = strtok(line, " \t\r\n");
        while (field != NULL) {
            if (count == maximum) return maximum + 1;
            fields[count++] = field;
            field = strtok(NULL, " \t\r\n");
        }
    }
    return count;
}

int parseInteger(const char *text, int minimum, int maximum, int *number) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (text == end || *end != '\0' || errno == ERANGE ||
        value < minimum || value > maximum) return 0;
    *number = (int)value;
    return 1;
}

int parseMoney(const char *text, long long *cents) {
    long long whole = 0;
    int fraction = 0, digits = 0;
    if (!isdigit((unsigned char)*text)) return 0;
    while (isdigit((unsigned char)*text)) {
        int digit = *text++ - '0';
        if (whole > (LLONG_MAX / 100 - digit) / 10) return 0;
        whole = whole * 10 + digit;
    }
    if (*text == '.') {
        text++;
        while (isdigit((unsigned char)*text)) {
            if (++digits > 2) return 0;
            fraction = fraction * 10 + (*text++ - '0');
        }
        if (digits == 0) return 0;
        if (digits == 1) fraction *= 10;
    }
    if (*text != '\0' || whole > (LLONG_MAX - fraction) / 100) return 0;
    *cents = whole * 100 + fraction;
    return 1;
}

void formatMoney(long long cents, char *buffer, size_t capacity) {
    snprintf(buffer, capacity, "%lld.%02lld", cents / 100, cents % 100);
}

int validDate(const char *date) {
    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') return 0;
    for (int i = 0; i < 10; i++) {
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i])) return 0;
    }
    int year, month, day;
    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3 ||
        year < 1900 || month < 1 || month > 12) return 0;
    const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int limit = days[month - 1];
    if (month == 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))) limit++;
    return day >= 1 && day <= limit;
}

void currentDate(char date[11]) {
    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    if (today == NULL || strftime(date, 11, "%Y-%m-%d", today) == 0)
        strcpy(date, "1900-01-01");
}

int isValidStudentIndex(const char *id) {
    size_t length = strlen(id);
    if (length < 3 || length >= STUDENT_ID_SIZE ||
        toupper((unsigned char)id[0]) != 'T' || toupper((unsigned char)id[1]) != 'G') return 0;
    for (size_t i = 2; i < length; i++) {
        unsigned char ch = (unsigned char)id[i];
        if (!isalnum(ch) && ch != '/' && ch != '-' && ch != '_') return 0;
    }
    return 1;
}

void normalizeStudentIndex(char *id) {
    for (; *id != '\0'; id++) *id = (char)toupper((unsigned char)*id);
}

int validStudentName(const char *name) {
    size_t length = strlen(name);
    if (length == 0 || length >= STUDENT_NAME_SIZE) return 0;
    for (size_t i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)name[i];
        if (ch == '|' || iscntrl(ch) || isdigit(ch)) return 0;
    }
    return 1;
}

int validPhoneNumber(const char *phone) {
    if (strlen(phone) != 10) return 0;
    for (int i = 0; i < 10; i++) if (!isdigit((unsigned char)phone[i])) return 0;
    return 1;
}

int readConsoleLine(const char *prompt, char *buffer, size_t capacity) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        int status = readTextLine(stdin, buffer, capacity);
        if (status == TEXT_LINE_EOF || status == TEXT_LINE_ERROR) return 0;
        if (status == TEXT_LINE_INVALID) {
            fprintf(stderr, "Invalid input: line is too long or contains a NUL byte.\n");
            continue;
        }
        char *trimmed = trimText(buffer);
        memmove(buffer, trimmed, strlen(trimmed) + 1);
        return 1;
    }
}

int readMenuChoice(int minimum, int maximum, int *choice) {
    char line[64];
    while (readConsoleLine("Enter your choice: ", line, sizeof(line))) {
        if (parseInteger(line, minimum, maximum, choice)) return 1;
        fprintf(stderr, "Invalid choice. Please select between %d-%d.\n", minimum, maximum);
    }
    return 0;
}

int readStudentIndex(const char *prompt, char id[STUDENT_ID_SIZE]) {
    char line[128];
    while (readConsoleLine(prompt, line, sizeof(line))) {
        if (isValidStudentIndex(line)) {
            strcpy(id, line);
            normalizeStudentIndex(id);
            return 1;
        }
        fprintf(stderr, "Invalid Student ID. Use TG followed by letters, digits, '/', '-' or '_' (max 29 characters).\n");
    }
    return 0;
}
