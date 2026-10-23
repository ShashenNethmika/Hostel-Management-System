# Hostel Management System

A C console application for the group's hostel management mini project.
All five member modules run through **one executable and one main menu**.
Data is stored in text files and remains available after the program exits.

## Build and run on Ubuntu

Requirements: GCC with C11 support and Make. Python 3 is only needed for tests.

```sh
make
make run
```

Or run the executable directly from the repository root:

```sh
./build/hostel
```

To keep your own data separate from the supplied samples, copy the data
directory and pass its path:

```sh
cp -r data /tmp/hostel-demo-data
./build/hostel /tmp/hostel-demo-data
```

The program selects `data/` from the project root, `../data/` when started
from `src/` or `build/`, or the data directory beside the executable's parent
directory on Ubuntu. An explicit data-directory argument takes priority.
Startup changes the working directory once so every module opens the same files.

## Main menu

| Choice | Module | Features |
| --- | --- | --- |
| 1 | Student registration | Register, list, search, update names/phones; reject duplicate IDs |
| 2 | Room allocation | Add rooms, show vacancies, allocate/remove students; enforce capacity and one room per student |
| 3 | Attendance and leave | Record attendance, view history/current leave, expected return dates and overdue students |
| 4 | Fees and payments | Add fee profiles, accumulate payments, reject overpayments, view receipts/balances and pending fees |
| 5 | Reports | Dues sorted by highest balance using bubble sort, room occupancy, current leave, student totals, overdue leave |

Register students in menu 1 before adding new allocations, attendance, or fee
profiles. Student IDs are case insensitive and stored uppercase for new
records. They must start with `TG`, contain 3–29 characters, and use letters,
digits, `/`, `-`, or `_`. Existing slash-separated IDs remain supported;
slashes are not removed, so `TG/IT/1234` and `TGIT1234` are distinct IDs.
Names can contain spaces; phone numbers require exactly 10 digits.
Menus accept complete input lines and return safely on end of input.

## Repository layout

```text
src/
  main.c                 Common main menu (the only application main)
  common.c, common.h     Shared input, validation, parsing and data directory
  student.c, student.h   Member 1: student registration and management
  rooms.c, rooms.h       Member 2: room allocation and management
  attendance.c, attendance.h
                         Member 3: attendance and leave management
  fee.c, fee.h           Member 4: fee and payment tracking
  reports.c, reports.h   Member 5: reports and statistics
data/                    Uploaded and existing sample text files
examples/fee_main.c       Optional standalone fee-menu driver
tests/integration.py      CLI integration tests using disposable data
Makefile                 Build, run, test and clean commands
```

The modules were adapted from the supplied group files to share interfaces,
student records and data paths. The fee-only uploaded `main.c` is kept as
`examples/fee_main.c`; its separate main is excluded from the application build.
Previously committed binaries and the generated dues report were removed.
PDFs, executable/object files, build output, temporary files, and Windows
`Zone.Identifier` metadata are excluded from Git.

## Shared data formats

| File | Record format |
| --- | --- |
| `students.txt` | `ID\|Name\|Phone` |
| `rooms.txt` | `RoomNumber Capacity Occupied` |
| `allocations.txt` | `StudentID RoomNumber` |
| `attendance.txt` | `ID\|Name\|Status\|Date\|ExpectedReturn` |
| `payments.txt` | `StudentID TotalFee PaidAmount` |
| `fee_students.txt` | `StudentID\|Name\|RoomNumber\|MonthlyFee` (created by the fee menu) |
| `unpaid_summary.txt` | Generated sorted unpaid balances; ignored by Git |

Attendance status is `1` for in hostel and `0` for on leave. Dates use
`YYYY-MM-DD`; `-` means no return date. New leave records require a valid
expected return date of today or later. A student is overdue when their latest
record is leave and its expected return date is before today. Dates use the
machine's local timezone. Original `ID Status` samples and four-field attendance
history remain readable; records without return dates are identified separately
and cannot establish overdue status. Returning to the hostel clears current leave
without deleting history.

The total hostel fee is Rs 50,000.00. Money is tracked in integer cents to
preserve exact payment totals. The fee module keeps optional monthly-fee and
room details separately so payments stay compatible with the reports. That
room number is informational; actual allocation changes only through menu 2.
Student names shown in fee and current-leave views come from the shared student
register, so name updates are reflected there. Historical attendance keeps the
name recorded at the time.

The uploaded student/room/allocation samples and existing payment/attendance
samples were preserved. They originate from different module examples and may
reference different student IDs. Historical records remain readable; new writes
use the shared register. Do not use these mixed samples as live hostel records.
Room records can include older occupants without ID allocations; those occupants
cannot be removed by ID until their actual allocations are recorded.

## Validation

```sh
make test
```

Eight integration tests exercise the real executable: registration/updates,
cross-module data sharing, capacity/duplicate checks, attendance/overdue state,
payment accumulation and exact balances, sorted reports, restart persistence,
legacy/malformed records, invalid input/EOF, and supplied sample data.
Tests create temporary data directories and leave repository data unchanged.

`make clean` removes only the ignored build directory. Rebuild with `make`.
This single-user CLI should be run one instance at a time against a data directory.
