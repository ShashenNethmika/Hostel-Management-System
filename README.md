# Hostel Management System

A C console application for the group's hostel management mini project.
All five member modules run through **one executable and one main menu**.
Data is stored in text files and remains available after the program exits.

## Group contributions

| Member | Student ID | Original contribution | Files |
| --- | --- | --- | --- |
| A.A.Shashen Nethmika | TG IT 2025 2469 | Main menu and reports/statistics | `src/main.c`, `src/reports.c`, `src/reports.h` |
| M.M.Ishara Madhushan | TG IT 2025 2514 | Student registration and management | `src/student.c`, `src/student.h` |
| S.G.A.T.V Subhawickrama | TG IT 2025 2518 | Attendance and leave management | `src/attendance.c`, `src/attendance.h` |
| R.M.P.Prabodhini Rathnayake | TG IT 2025 2509 | Room allocation and management | `src/rooms.c`, `src/rooms.h` |
| H.G.Chesmi Tharushika | TG IT 2025 2465 | Fees and payment tracking | `src/fee.c`, `src/fee.h` |

These credits identify the authors of the supplied group modules. The repository
also includes subsequent integration changes, shared helpers, and tests.

## Build and run on Ubuntu

Requirements: GCC with C11 support and Make. Python 3 is only needed for tests.

```sh
make
make run
```

For a complete demonstration with matching student/room/payment data:

```sh
make demo
```

Each demo starts with a fresh temporary copy of `examples/demo-data`; its path
is printed so you can reopen the same records later. The supplied files stay
unchanged. See [lecture alignment and integration](docs/lecture-alignment.md)
for the mapping to Lectures 07–11 and the module connections.

The [Word-document review feedback](docs/document-review-feedback.md) contains
section-specific corrections and ready-to-forward messages for the module
developers, including the updated Rs 5,000 fee specification.

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
It creates missing `students.txt`, `rooms.txt`, `allocations.txt`,
`attendance.txt`, and `payments.txt` without resetting existing records.
Fee profiles and the generated dues report are created when needed. An existing
empty directory is enough for a first run; unreadable/uncreatable paths are
reported rather than treated as successful initialization.

## Main menu

| Choice | Module | Features |
| --- | --- | --- |
| 1 | Student registration | Register, list, search, update names/phones; reject duplicate IDs |
| 2 | Room allocation | Add rooms, show vacancies, allocate/remove students; enforce capacity and one room per student |
| 3 | Attendance and leave | Record attendance, view history/current leave, expected return dates and overdue students |
| 4 | Fees and payments | Create profiles for allocated students, accumulate payments, reject overpayments, show current rooms and balances |
| 5 | Reports | Dues sorted by highest balance using bubble sort, room occupancy, current leave, student totals, overdue leave |

Register students in menu 1 before adding new allocations or attendance. Allocate
a real room in menu 2 before creating a fee profile in menu 4. The fee module
derives the room from that allocation; it never accepts a separate room number.
Student IDs are case insensitive and stored uppercase for new
records. They must start with `TG`, contain 3–29 characters, and use letters,
digits, `/`, `-`, or `_`. Existing slash-separated IDs remain supported;
slashes are not removed, so `TG/IT/1234` and `TGIT1234` are distinct IDs.
Names can contain spaces; phone numbers require exactly 10 digits.
Menus accept complete input lines and return safely on end of input.
LF and CRLF line endings are accepted, including names at the maximum length.
Embedded NUL bytes and overlong input are rejected as complete lines.
Invalid field formats/ranges retry the same prompt in every module; EOF cancels
without saving. Unknown students, unavailable/full rooms, and duplicate entities
report the problem and cancel that operation. Prompts/results use stdout; errors
use stderr.

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
examples/demo-data/       Matching synthetic records for all five modules
docs/lecture-alignment.md Lecture coverage and integration explanation
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
| `fee_students.txt` | `StudentID\|MonthlyFee` (created by the fee menu) |
| `unpaid_summary.txt` | Generated sorted unpaid balances; ignored by Git |
| `room_transaction.txt` | Internal recovery journal for interrupted room/allocation updates; ignored by Git |

Student records require pipe separators. Attendance history also requires pipe
separators; only the original whitespace `ID Status` two-field format is supported
as legacy attendance. Whitespace student records and whitespace 4/5-field
attendance history are rejected.

Attendance status is `1` for in hostel and `0` for on leave. Dates use
`YYYY-MM-DD`; `-` means no return date. New leave records require a valid
expected return date of today or later. A student is overdue when their latest
record is leave and its expected return date is before today. Dates use the
machine's local timezone. Original `ID Status` samples and four-field attendance
history remain readable; records without return dates are identified separately
and cannot establish overdue status. Returning to the hostel clears current leave
without deleting history. Future-dated attendance events are rejected; expected
return dates can be in the future.

The total hostel fee is Rs 5,000.00, defined once by `HOSTEL_FEE_CENTS` in
`common.h`. Every module uses `long long` integer cents
and shared money parsing/formatting to preserve exact totals. A fee profile stores
only the student's ID and monthly fee; names come from `students.txt` and rooms
come from `allocations.txt`, validated against `rooms.txt`. Removing an allocation
shows the student as unallocated; moving to another room shows that room without
changing the fee profile. Existing debt remains payable after removal.

Versioned sample/demo payments use the Rs 5,000 fee and revised illustrative
paid amounts. Previously saved runtime payments are never automatically scaled,
clamped, or rewritten. Old Rs 50,000 totals need review before payment entry;
the dues report also preserves its previous output until those amounts are
reviewed. The monthly-fee input remains profile information rather than an
automatic monthly billing calculation.

Opening the fee menu validates and migrates legacy
`ID|Name|RoomNumber|MonthlyFee` profiles to `ID|MonthlyFee`. Migration validates
legacy names using the same rules as registration, removes the copied room/name
fields, and preserves payment amounts. Invalid legacy data is left unchanged and
must be corrected before that menu can open.

Fee and current-leave lists load the student register once per operation and use
in-memory searches for each row. Fee lists similarly load allocations once.
Name updates are visible in both modules; historical attendance keeps the name
recorded at the time. A corrupt register aborts a list with one diagnostic rather
than repeating the same error for every row.

The original student, room, allocation and attendance samples retain their IDs;
illustrative payment amounts were revised for the Rs 5,000 fee. These files
originate from different module examples and may
reference different student IDs. Historical records remain readable; new writes
use the shared register. Use `make demo` for a consistent demonstration across
all modules. Do not use the mixed original samples as live hostel records.
Room records can include older occupants without ID allocations; those occupants
cannot be removed by ID until their actual allocations are recorded.

## File safety and recovery

Student edits and payment/fee-profile updates validate existing records before
writing. Malformed or duplicate IDs block changes so ambiguous data is preserved
for correction. Read-only reports warn about malformed records; duplicate payment
IDs or room numbers prevent the affected financial/occupancy calculation.
Student summaries count each normalized ID once.

Saves use uniquely created temporary files and atomic replacement. Appending
student or attendance records also handles files without a final newline.
Failed saves preserve the prior destination; stale temporary files cannot redirect
writes to another file.

Room allocation updates affect both `rooms.txt` and `allocations.txt`. A validated
`room_transaction.txt` journal records the intended state before either file is
replaced. If saving fails before the journal is committed, no room change is
saved and the original state is reloaded immediately; no restart/recovery is
required. If the journal was committed, the change is pending until replay
completes. Startup and room reports recover pending updates after a process
interruption. If a file error prevents recovery, correct the reported filesystem
problem and reopen Room Management or restart; check the recovered state before
retrying the operation.
Keep the journal until recovery succeeds. A malformed journal blocks startup
without replacing the original data and needs manual correction from a known
good backup. This recovery supports process interruption; it does not guarantee
recovery after a machine power loss.

## Validation

```sh
make test
```

Integration and regression tests exercise the real executable: registration/updates,
cross-module data sharing, capacity/duplicate checks, attendance/overdue state,
payment accumulation and exact balances, sorted reports, restart persistence,
legacy/malformed records, invalid input/EOF, supplied sample data, integer
overflow, NUL/CRLF boundaries, failed writes, and interrupted room recovery.
They also cover first-run file creation and the matching demonstration dataset
through registration, allocation, attendance, payment, and reports.
Tests create their own deterministic sample fixtures in temporary directories,
so they also work with a completely empty `data/` directory or an extracted
source archive. An existing `unpaid_summary.txt` is treated as generated output,
not an input that must stay byte-identical. Repository data is left unchanged.
To test another build, run `python3 tests/run.py /path/to/hostel`.

`make clean` removes only the ignored build directory. Rebuild with `make`.
This single-user CLI should be run one instance at a time against a data directory.
