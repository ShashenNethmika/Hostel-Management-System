# Hostel Management System

<div align="center">

![C](https://img.shields.io/badge/Language-C11-blue.svg?style=for-the-badge&logo=c)
![Build](https://img.shields.io/badge/Build-Passing-brightgreen.svg?style=for-the-badge)
![Tests](https://img.shields.io/badge/Tests-82_Passed-success.svg?style=for-the-badge)

A robust, console-based **Hostel Management System** developed in C.
Designed as a mini-project for the **Faculty of Technology**, this application seamlessly handles student registrations, room allocations, daily attendance, fee collections, and statistical reporting.

</div>

---

## 🌟 Key Features

The system is organized into five core modules, each accessible via a unified main menu:

| 🧩 Module | ⚙️ Functionality |
| :--- | :--- |
| **1. Student Registration** | Register new students, search by ID, update contact details, and list all records. Enforces unique IDs. |
| **2. Room Allocation** | Add new rooms, view vacancies, and allocate/deallocate students. Strictly enforces room capacities (1 room per student). |
| **3. Attendance & Leave** | Track daily attendance (In/Out). Manage leave history, expected return dates, and identify overdue students. |
| **4. Fees & Payments** | Maintain student fee profiles, accumulate monthly payments, track balances, and prevent overpayments. |
| **5. Reports & Statistics** | Generate insightful reports including dues (sorted via Bubble Sort), room occupancy rates, and leave summaries. |

Requirements: GCC with C11 support and Make. Python 3 is only needed for tests.

```sh
make
make run
```

### 📋 Prerequisites

To compile and run this project, you will need:
* **GCC** (or any compatible C11 compiler)
* **Make** (for automated build processes)
* **Python 3** (Required *only* for running the integration test suite)

### 🛠️ Installation & Execution

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```

2. **Build the application:**
   Compile the source code using the provided Makefile.
   ```bash
   make
   ```

3. **Run the system:**
   Launch the main application.
   ```bash
   make run
   ```
   *(Alternatively, run the binary directly: `./build/hostel [data-directory]`)*

### 🎬 Interactive Demo

Want to test the system with pre-loaded data? Run the demo mode! It populates the system with sample synthetic records across all modules so you can test features immediately.
```bash
make demo
```

To keep your own data separate from the supplied samples, copy the data
directory and pass its path:

```sh
cp -r data /tmp/hostel-demo-data
./build/hostel /tmp/hostel-demo-data
```

The system utilizes a modular C architecture. Core logic is divided into distinct `.c` and `.h` files, which are all linked through a central `main.c` entry point. Data is persisted using structured text files.

```text
📁 src/
  ├── main.c                 # Application Entry Point & Main Menu
  ├── common.c / .h          # Shared utilities (input validation, formatting, file paths)
  ├── student.c / .h         # Module 1: Student Management
  ├── rooms.c / .h           # Module 2: Room Allocation
  ├── attendance.c / .h      # Module 3: Attendance Tracking
  ├── fee.c / .h             # Module 4: Financial Transactions
  └── reports.c / .h         # Module 5: Analytics and Reports
📁 data/                     # Storage for text-based database files
📁 examples/                 # Demo data and standalone module drivers
📁 tests/                    # Comprehensive Python-based integration tests
📄 Makefile                  # Build and test orchestration
```

<details>
<summary><b>🛠️ Click to expand: Data Formats & Storage Details</b></summary>

### Data Storage Strategy
All modules read from and write to the same directory (defaulting to `data/`). Upon the first run, the system intelligently creates any missing necessary files without overwriting existing data.

- **Student IDs**: Case-insensitive, must begin with `TG`, length of 3–29 characters. Supported symbols: `/`, `-`, `_`.
- **Currency**: The standard hostel fee is defined as Rs 5,000.00 (`HOSTEL_FEE_CENTS` in `common.h`). All monetary calculations are performed in *cents* using `long long` integers to prevent floating-point precision errors.

### Text File Schemas
| File Name | Record Format |
| --- | --- |
| `students.txt` | `ID\|Name\|Phone` |
| `rooms.txt` | `RoomNumber Capacity Occupied` |
| `allocations.txt` | `StudentID RoomNumber` |
| `attendance.txt` | `ID\|Name\|Status\|Date\|ExpectedReturn` (Status: 1=Hostel, 0=Leave) |
| `payments.txt` | `StudentID TotalFee PaidAmount` |
| `fee_students.txt` | `StudentID\|MonthlyFee` |

*(Note: Files storing data that may contain spaces, such as Names, utilize `|` as a strict delimiter).*
</details>

---

## 🛡️ Reliability & File Safety

Data integrity is a primary focus of this application. It implements several fail-safes to protect against data loss or corruption during runtime or power failures.

* **Pre-Write Validation**: Modifications (like student edits or payments) are validated against the active memory state *before* any disk writes occur. Ambiguous or duplicate inputs are rejected early.
* **Atomic Operations**: All file saves write to temporary files first (`mkstemp`), followed by an atomic POSIX file rename, ensuring files are never left in partially-written states.
* **Transaction Journaling**: Operations that span multiple files (e.g., Room Allocation updates `rooms.txt` and `allocations.txt`) use a `room_transaction.txt` journal. If the system crashes mid-update, the next startup automatically detects the journal and completes the rollback or commit process.

---

## 🧪 Testing

This codebase is backed by a robust suite of integration and regression tests written in Python, ensuring all C modules interact flawlessly.

**Test coverage includes:**
- Validating system capacity constraints and duplicate ID rejection.
- Ensuring exact monetary accumulation (preventing integer overflow).
- Verifying the recovery journal handles interrupted processes correctly.
- Parsing legacy or malformed text records safely.

To run the full suite of 80+ tests:
```bash
make test
```

To clean all compiled binaries and temporary artifacts:
```bash
make clean
```

---

## 👥 Group Contributions

This project was developed collaboratively by the following members.

| Member | Student ID | Original Module Contribution | Files |
| --- | --- | --- | --- |
| [A.A.Shashen Nethmika](https://github.com/ShashenNethmika) | TG IT 2025 2469 | Main menu and Reports/Statistics | `src/main.c`, `src/reports.c`, `src/reports.h` |
| [M.M.Ishara Madhushan](https://github.com/madusha2005) | TG IT 2025 2514 | Student Registration and Management | `src/student.c`, `src/student.h` |
| [S.G.A.T.V Subhawickrama](https://github.com/Tharushi-Vihanga) | TG IT 2025 2518 | Attendance and Leave Management | `src/attendance.c`, `src/attendance.h` |
| [R.M.P.Prabodhini Rathnayake](https://github.com/yumi675) | TG IT 2025 2509 | Room Allocation and Management | `src/rooms.c`, `src/rooms.h` |
| [H.G.Chesmi Tharushika](https://github.com/chesmihalpagoda-bit) | TG IT 2025 2465 | Fees and Payment Tracking | `src/fee.c`, `src/fee.h` |

*These credits identify the authors of the supplied core group modules. The repository also encompasses subsequent integration changes, shared utility helpers, and the comprehensive test suite.*
