"""End-to-end CLI checks against disposable data."""
import subprocess
import pathlib
import sys

if __name__ == "__main__":
    # Keep the original test entry point working while running all regressions.
    raise SystemExit(subprocess.call([sys.executable, str(pathlib.Path(__file__).with_name("run.py")), *sys.argv[1:]]))

from cli_support import HostelTestCase, A, B, BINARY, PROJECT


class HostelIntegration(HostelTestCase):
    def test_full_workflow_and_restart(self):
        self.register()
        output = self.run_cli(
            f"1\n1\n{A.lower()}\n3\n{A.lower()}\n0\n"
            f"2\n3\n101\n1\n4\n{A.lower()}\n101\n"
            f"4\n{A}\n4\n{B}\n101\n0\n"
            f"3\n1\n{A}\n0\n2099-12-31\n2\n3\n0\n"
            f"4\n1\n{A}\n1000\n2\n{A}\n1000.25\n"
            f"2\n{A}\n200.75\n1\n{A}\n3\n{A}\n4\n5\n0\n"
            "5\n1\n2\n3\n4\n5\n0\n0\n", warnings=True
        )
        for expected in (
            "Student ID Already Registered!", "STUDENT FOUND",
            "already allocated to room 101", "Room 101 is FULL",
            "Attendance saved for Sample Student (Went Home)",
            "Duplicate not added",
        ):
            self.assertIn(expected, output)
        self.assertIn("Total Paid  : Rs 1201.00", output)
        self.assertIn("Balance Due : Rs 3799.00", output)
        self.assertIn("Overall Occupancy Rate: 100.00%", output)
        self.assertIn("Total students currently on leave: 1", output)
        self.assertIn("Total Registered Students in System: 2", output)
        self.assertEqual((self.data / "payments.txt").read_text(), f"{A} 5000.00 1201.00\n")
        self.assertEqual((self.data / "rooms.txt").read_text(), "101 1 1\n")
        self.assertEqual((self.data / "allocations.txt").read_text(), f"{A} 101\n")
        self.assertEqual(len((self.data / "students.txt").read_text().splitlines()), 2)
        self.assertIn("3799.00", (self.data / "unpaid_summary.txt").read_text())
        # A new process reads the saved state; returning cancels the current leave.
        restarted = self.run_cli(f"3\n1\n{A}\n1\n3\n0\n4\n3\n{A}\n0\n5\n3\n0\n0\n")
        self.assertIn("Total students currently on leave: 0", restarted)
        self.assertIn("Paid        : Rs 1201.00", restarted)

    def test_student_updates_are_shared_and_room_removal_persists(self):
        self.register()
        self.run_cli(f"2\n3\n101\n2\n4\n{A}\n101\n0\n4\n1\n{A}\n1000\n0\n0\n")
        output = self.run_cli(
            f"1\n4\n{A}\n1\nUpdated Student\n4\n{A}\n2\ninvalid\n0770000000\n0\n"
            f"3\n1\n{A}\n1\n0\n4\n3\n{A}\n4\n0\n2\n5\n{A}\n0\n0\n", warnings=True
        )
        self.assertIn("Invalid Phone Number", output)
        self.assertIn("Student Name: Updated Student", output)
        self.assertIn("Attendance saved for Updated Student", output)
        self.assertIn("removed from room 101", output)
        self.assertIn("Updated Student|0770000000", (self.data / "students.txt").read_text())
        self.assertEqual((self.data / "rooms.txt").read_text(), "101 2 0\n")
        self.assertEqual((self.data / "allocations.txt").read_text(), "")

    def test_latest_attendance_and_overdue_dates(self):
        self.register()
        (self.data / "attendance.txt").write_text(
            f"{A}|Sample Student|0|2000-01-01|2000-01-02\n"
            f"{B}|Other Student|0|2000-01-01|2000-01-02\n"
            f"{B}|Other Student|1|2000-01-03|-\n"
        )
        output = self.run_cli("5\n3\n5\n0\n0\n")
        self.assertIn("Total students currently on leave: 1", output)
        self.assertIn("Total students overdue: 1", output)
        self.assertNotIn(B, output)
        returned = self.run_cli(f"3\n1\n{A}\n1\n0\n5\n5\n0\n0\n")
        self.assertIn("Total students overdue: 0", returned)

    def test_input_validation_and_eof(self):
        for text in ("", "x\n99\n0\n", "1\n", "2\n", "3\n", "4\n", "5\n",
                     "1\n1\n", "1\n1\nTGTEST\n", "0" , "z" * 1000 + "\n0\n"):
            with self.subTest(text=text[:30]):
                self.run_cli(text, warnings=True)
        output = self.run_cli(
            f"1\n1\nBAD\n{A}\nName1\nValid Name\n123456789012\nabcdefghij\n0771234567\n0\n"
            f"3\n1\n{A}\ninvalid\n0\n2026-02-30\n1900-01-01\n2099-12-31\n0\n0\n", warnings=True
        )
        self.assertIn("Invalid Student ID", output)
        self.assertIn("Invalid name", output)
        self.assertIn("Invalid Phone Number", output)
        self.assertIn("Invalid return date", output)
        self.assertEqual(len((self.data / "students.txt").read_text().splitlines()), 1)
        for submenu in (2, 3, 4):
            operation = 4 if submenu == 2 else 1
            output = self.run_cli(f"{submenu}\n{operation}\nTGUNKNOWN\n0\n0\n", warnings=True)
            self.assertIn("Student not registered", output)

    def test_money_validation_and_sorted_dues(self):
        self.register()
        self.run_cli(f"2\n3\n101\n2\n4\n{A}\n101\n0\n4\n1\n{A}\n1000\n0\n0\n")
        for amount in ("NaN", "-1", "0", "2.333", "5000.01", "1e4", "9" * 90):
            output = self.run_cli(f"4\n2\n{A}\n{amount}\n", warnings=True)
            self.assertIn("Invalid amount", output)
            self.assertEqual((self.data / "payments.txt").read_text(), f"{A} 5000.00 0.00\n")
        (self.data / "payments.txt").write_text(f"{A} 5000.00 1234.56\n{B} 5000.00 0.00\n")
        self.run_cli("5\n1\n0\n0\n")
        rows = (self.data / "unpaid_summary.txt").read_text().splitlines()[1:]
        self.assertEqual([row.split() for row in rows], [[B, "5000.00"], [A, "3765.44"]])
        paid = self.run_cli(f"4\n2\n{A}\n3765.44\n2\n{A}\n0\n5\n1\n0\n0\n", warnings=True)
        self.assertIn("already fully paid", paid)
        self.assertNotIn(A, (self.data / "unpaid_summary.txt").read_text())

    def test_legacy_and_malformed_records(self):
        (self.data / "attendance.txt").write_text(
            "TG/IT/2025/2469 0\n"
            f"{A}|Sample Student|0|2000-01-01\n"
            "bad|missing\nTGNOPE|Name|oops|2000-01-01\n" + "x" * 600 + "\n"
        )
        output = self.run_cli("3\n2\n3\n4\n0\n0\n", warnings=True)
        self.assertIn("Total students currently on leave: 2", output)
        self.assertIn("Total students overdue: 0", output)
        self.assertIn("2 legacy leave record(s)", output)
        self.register()
        bad_payments = f"{A} 5000 6000\n{B} 5000 1000\n"
        (self.data / "payments.txt").write_text(bad_payments)
        output = self.run_cli(f"4\n2\n{B}\n0\n5\n1\n0\n0\n", warnings=True)
        self.assertIn("Invalid payments.txt", output)
        self.assertEqual((self.data / "payments.txt").read_text(), bad_payments)
        self.assertNotIn(A, (self.data / "unpaid_summary.txt").read_text())

    def test_repository_samples(self):
        # A deterministic fixture: checkout/archive data may be empty or user-edited.
        third = "TG2024000003"
        samples = {
            "students.txt": f"{A}|Sample Student|0771234567\n{B}|Other Student|0777654321\n{third}|Third Student|0770000000\n",
            "rooms.txt": "101 4 1\n102 4 0\n",
            "allocations.txt": f"{A} 101\n",
            "attendance.txt": f"{A} 0\n{B} 0\n{third} 1\n",
            "payments.txt": f"{A} 5000 2500\n{B} 5000 5000\n{third} 5000 0\n",
            "fee_students.txt": f"{A}|1000.00\n",
        }
        for name, content in samples.items():
            (self.data / name).write_text(content)
        # This is an output, including when a supplied archive contains an empty one.
        (self.data / "unpaid_summary.txt").write_text("")
        output = self.run_cli("1\n2\n0\n2\n1\n6\n0\n3\n2\n3\n0\n4\n4\n5\n0\n5\n1\n2\n3\n4\n5\n0\n0\n")
        self.assertIn("Total Registered Students in System: 3", output)
        self.assertIn("Overall Occupancy Rate: 12.50%", output)
        self.assertIn("Total students currently on leave: 2", output)
        for name, content in samples.items():
            self.assertEqual((self.data / name).read_text(), content, name)
        rows = (self.data / "unpaid_summary.txt").read_text().splitlines()[1:]
        self.assertEqual([row.split() for row in rows], [[third, "5000.00"], [A, "2500.00"]])

    def test_all_empty_data_files_work(self):
        for name in ("fee_students.txt", "unpaid_summary.txt"):
            (self.data / name).write_text("")
        output = self.run_cli("1\n2\n0\n2\n1\n6\n0\n3\n2\n3\n0\n4\n4\n5\n0\n5\n1\n2\n3\n4\n5\n0\n0\n")
        self.assertIn("Total Registered Students in System: 0", output)
        self.assertIn("Overall Occupancy Rate: 0.00%", output)
        self.assertIn("Total students currently on leave: 0", output)
        for name in ("students", "rooms", "allocations", "attendance", "payments", "fee_students"):
            self.assertEqual((self.data / f"{name}.txt").read_bytes(), b"")
        self.assertEqual((self.data / "unpaid_summary.txt").read_text(), "--- Unpaid Hostel Fees Report ---\n")

    def test_default_and_explicit_data_directories(self):
        result = subprocess.run([str(BINARY)], input="0\n", text=True,
                                cwd=self.root, capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(BINARY), str(self.root / "missing")], input="0\n",
                                text=True, cwd=self.root, capture_output=True, timeout=5)
        self.assertNotEqual(result.returncode, 0)
        result = subprocess.run([str(BINARY), "a", "b"], text=True,
                                capture_output=True, timeout=5)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Usage:", result.stderr)
