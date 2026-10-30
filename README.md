# 🏫 Hostel Management System

![C](https://img.shields.io/badge/C-11-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Build](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

A comprehensive, console-based **Hostel Management System** written in C. Designed for the Faculty of Technology, it manages student registration, room allocations, attendance, fee payments, and generates detailed reports.

The system relies on persistent text-file storage and features atomic writes, recovery journaling for data safety, and robust input validation.

---

## 🌟 Features

| Module | Features |
| :--- | :--- |
| **1. Student Registration** | Register, list, search, and update names/phones. Rejects duplicate IDs. |
| **2. Room Allocation** | Add rooms, show vacancies, allocate/remove students. Enforces capacity and 1 room/student rule. |
| **3. Attendance & Leave** | Record daily attendance, view history/current leave, expected return dates, and overdue students. |
| **4. Fees & Payments** | Manage student fee profiles, accumulate payments, reject overpayments, show current balances. |
| **5. Reports & Statistics** | View dues (sorted using Bubble Sort), room occupancy, leave totals, and overdue summaries. |

---

## 🚀 Getting Started

### Prerequisites

* GCC (or any compatible C11 compiler)
* Make (for easy building)
* Python 3 (for running integration tests)
* Valgrind (optional, for memory testing)

### Installation & Build

1. Clone the repository and navigate into the directory.
2. Build the system using Make:
   ```bash
   make
   ```
3. Run the application:
   ```bash
   make run
   ```
   *Alternatively:* `./build/hostel [data-directory]`

### 🎬 Demo Mode

To see the system in action with sample synthetic records (across all 5 modules):
```bash
make demo
```

---

## 🗃️ Architecture & Repository Layout

The system uses a modular architecture where separate `.c` and `.h` files handle distinct business logic but share a common `main.c` entry point.

```text
src/
  ├── main.c                 # Common main menu (Application Entry Point)
  ├── common.c / .h          # Shared input, validation, and data directory helpers
  ├── student.c / .h         # Module 1: Student registration
  ├── rooms.c / .h           # Module 2: Room management
  ├── attendance.c / .h      # Module 3: Attendance & leave
  ├── fee.c / .h             # Module 4: Fee & payments
  └── reports.c / .h         # Module 5: Reports and stats
data/                        # Sample and active text-file records
examples/                    # Standalone drivers (fee_main.c) & demo data
tests/                       # Python-based CLI integration tests (integration.py)
Makefile                     # Build, run, test, and clean commands
```

<details>
<summary><b>Click to view Data Formats & Integration Details</b></summary>

### Data Sharing
All modules open files in the same directory (default is `data/`). Missing files are automatically generated upon the first run, preserving any existing records.
- **Student IDs** are case-insensitive, must begin with `TG`, contain 3–29 characters, and may include `/`, `-`, or `_`. (e.g., `TG/IT/1234` is distinct from `TGIT1234`).
- **Fees**: The standard hostel fee is Rs 5,000.00 (`HOSTEL_FEE_CENTS` in `common.h`).

### File Formats
| File | Format |
| --- | --- |
| `students.txt` | `ID\|Name\|Phone` |
| `rooms.txt` | `RoomNumber Capacity Occupied` |
| `allocations.txt` | `StudentID RoomNumber` |
| `attendance.txt` | `ID\|Name\|Status\|Date\|ExpectedReturn` (1=Hostel, 0=Leave) |
| `payments.txt` | `StudentID TotalFee PaidAmount` |
| `fee_students.txt` | `StudentID\|MonthlyFee` |

*(Note: Data fields requiring spaces, like Names, use `|` as a delimiter).*
</details>

---

## 🛡️ File Safety & Recovery

The system prioritizes data integrity during writes and process interruptions:

* **Validation First**: Student edits and payment updates validate against existing records *before* writing. Ambiguous or duplicate IDs block the process to prevent corruption.
* **Atomic Saves**: All saves utilize uniquely created temporary files (`mkstemp`) and atomic replacement (via standard POSIX file maneuvers).
* **Recovery Journaling**: Room allocation updates affect two files (`rooms.txt` and `allocations.txt`). A `room_transaction.txt` journal logs intended states. If an interruption occurs (e.g., crash, power loss), the startup process reads this journal to safely commit or rollback changes without user intervention.

---

## 🧪 Testing

The codebase includes an extensive suite of integration and regression tests written in Python. These tests cover:

- Capacity & Duplicate checks
- Exact monetary balances & Integer overflows
- Restart persistence & Recovery journaling
- Legacy data & Malformed record handling

To run the complete test suite:
```bash
make test
```

To clean build artifacts:
```bash
make clean
```

---
*Developed for the Faculty of Technology Hostel Management Project.*

## 👥 Group Contributions

| Member | Student ID | Original contribution | Files |
| --- | --- | --- | --- |
| A.A.Shashen Nethmika | TG IT 2025 2469 | Main menu and reports/statistics | `src/main.c`, `src/reports.c`, `src/reports.h` |
| M.M.Ishara Madhushan | TG IT 2025 2514 | Student registration and management | `src/student.c`, `src/student.h` |
| S.G.A.T.V Subhawickrama | TG IT 2025 2518 | Attendance and leave management | `src/attendance.c`, `src/attendance.h` |
| R.M.P.Prabodhini Rathnayake | TG IT 2025 2509 | Room allocation and management | `src/rooms.c`, `src/rooms.h` |
| H.G.Chesmi Tharushika | TG IT 2025 2465 | Fees and payment tracking | `src/fee.c`, `src/fee.h` |

These credits identify the authors of the supplied group modules. The repository
also includes subsequent integration changes, shared helpers, and tests.
