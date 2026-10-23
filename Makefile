CC = gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
CPPFLAGS ?= -Isrc

SOURCES := src/main.c src/common.c src/student.c src/rooms.c src/attendance.c src/fee.c src/reports.c
OBJECTS := $(SOURCES:src/%.c=build/%.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all run test clean
all: build/hostel

build/hostel: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p $@

run: build/hostel
	./build/hostel

test: build/hostel
	python3 tests/run.py ./build/hostel

clean:
	rm -rf build

-include $(DEPS)
