"""Regression coverage for attendance history and report validation/safe output."""
import os

from cli_support import A, B, HostelTestCase


class AttendanceAndReportsTests(HostelTestCase):
    def test_attendance_validation_errors_use_stderr_and_retry_dates(self):
        self.register()
        self.run_cli(f"3\n1\nTGUNKNOWN\n1\n{A}\n0\nbad-date\n2099-12-31\n0\n0\n", warnings=True)
        self.assertIn("Student not registered", self.last_run.stderr)
        self.assertIn("Invalid return date", self.last_run.stderr)
        self.assertNotIn("Student not registered", self.last_run.stdout)
        self.assertNotIn("Invalid return date", self.last_run.stdout)
        self.assertIn("Attendance saved for Sample Student", self.last_run.stdout)

    def test_attendance_history_requires_pipes_and_retains_only_legacy_two_fields(self):
        (self.data / "attendance.txt").write_text(
            f"{A} Sample 0 2000-01-01\n"
            f"{B} Other 0 2000-01-01 2000-01-02\n"
            "TGPIPE|0\n"
            "TGLEGACY 0\n"
            "TGFOUR|History Student|0|2000-01-01\n"
            "TGFIVE|Current Student|0|2000-01-01|2000-01-02\n"
        )
        self.run_cli("5\n3\n0\n0\n", warnings=True)
        output = self.last_run.stdout
        self.assertIn("Total students currently on leave: 3", output)
        for rejected in (A, B, "TGPIPE"):
            self.assertNotIn(rejected, output)
        for accepted in ("TGLEGACY", "TGFOUR", "TGFIVE"):
            self.assertIn(accepted, output)
        self.assertEqual(self.last_run.stderr.count("invalid attendance.txt record"), 3)

    def test_leave_reports_load_register_once_and_abort_with_one_corruption_error(self):
        self.register()
        (self.data / "students.txt").write_text(
            f"{A}|Sample Student|0771234567\n"
            "damaged registry row\n"
            f"{B}|Other Student|0777654321\n"
        )
        (self.data / "attendance.txt").write_text(
            f"{A}|Historical Student|0|2000-01-01|2000-01-02\n"
            f"{B}|Old Student|0|2000-01-01|2000-01-02\n"
        )
        for report in (3, 5):
            with self.subTest(report=report):
                self.run_cli(f"5\n{report}\n0\n0\n", warnings=True)
                self.assertEqual(len(self.last_run.stderr.strip().splitlines()), 1)
                self.assertIn("students.txt", self.last_run.stderr)
                self.assertNotIn("--- Student Leave Report ---", self.last_run.stdout)
                self.assertNotIn("--- Overdue Leave Report ---", self.last_run.stdout)
                self.assertNotIn("Total students", self.last_run.stdout)

    def test_leave_report_uses_current_registered_names_for_all_rows(self):
        self.register()
        (self.data / "attendance.txt").write_text(
            f"{A}|Historical Student|0|2000-01-01|2000-01-02\n"
            f"{B}|Old Student|0|2000-01-01|2000-01-02\n"
        )
        output = self.run_cli("5\n3\n0\n0\n")
        self.assertIn("Sample Student", output)
        self.assertIn("Other Student", output)
        self.assertNotIn("Historical Student", output)
        self.assertNotIn("Old Student", output)
        self.assertIn("Total students currently on leave: 2", output)

    def test_corrupt_register_is_not_reported_as_unregistered_attendance_student(self):
        (self.data / "students.txt").write_text("damaged registry row\n")
        self.run_cli(f"3\n1\n{A}\n0\n0\n", warnings=True)
        self.assertEqual(len(self.last_run.stderr.strip().splitlines()), 1)
        self.assertIn("students.txt", self.last_run.stderr)
        self.assertNotIn("Student not registered", self.last_run.stdout)
        self.assertEqual((self.data / "attendance.txt").read_text(), "")

    def test_append_after_unterminated_attendance_keeps_both_events(self):
        self.register()
        history = self.data / "attendance.txt"
        original = f"{A}|Sample Student|1|2000-01-01|-"
        history.write_text(original)
        output = self.run_cli(f"3\n1\n{A}\n0\n2099-12-31\n2\n3\n0\n0\n")
        self.assertIn("Attendance saved for Sample Student (Went Home)", output)
        self.assertIn("Total students currently on leave: 1", output)
        lines = history.read_text().splitlines()
        self.assertEqual(len(lines), 2)
        self.assertEqual(lines[0], original)
        self.assertEqual(lines[1].split("|")[:3], [A, "Sample Student", "0"])
        self.assertFalse(list(self.data.glob("*.tmp.*")))

    def test_return_after_unterminated_leave_preserves_history(self):
        self.register()
        history = self.data / "attendance.txt"
        original = f"{A}|Sample Student|0|2000-01-01|2000-01-02"
        history.write_text(original)
        output = self.run_cli(f"3\n1\n{A}\n1\n2\n3\n4\n0\n0\n")
        self.assertIn("Total students currently on leave: 0", output)
        self.assertIn("Total students overdue: 0", output)
        self.assertEqual(history.read_text().splitlines()[0], original)
        self.assertEqual(len(history.read_text().splitlines()), 2)

    def test_future_and_nul_attendance_do_not_override_current_records(self):
        self.register()
        (self.data / "attendance.txt").write_bytes(
            f"{A}|Sample Student|1|2000-01-01|-\n"
            f"{A}|Sample Student|0|2099-01-01|2099-12-31\n"
            f"{A}|Sample Student|0|2000-01-01|2000-01-02".encode()
            + b"\x00hidden\n"
            + f"{B}|Other Student|0|2000-01-01|2000-01-02\n".encode()
        )
        output = self.run_cli("5\n3\n5\n0\n0\n", warnings=True)
        self.assertIn("Total students currently on leave: 1", output)
        self.assertIn("Total students overdue: 1", output)
        self.assertNotIn(A, output)
        self.assertIn(B, output)

    def test_duplicate_payment_ids_refuse_report_and_preserve_previous_output(self):
        previous = "previous verified report\n"
        report = self.data / "unpaid_summary.txt"
        report.write_text(previous)
        (self.data / "payments.txt").write_text(
            f"{A} 50000.00 10000.00\n{A.lower()} 50000.00 20000.00\n"
        )
        output = self.run_cli("5\n1\n0\n0\n", warnings=True)
        self.assertNotIn("Unpaid Hostel Fees Report (Sorted", output)
        self.assertEqual(report.read_text(), previous)

    def test_report_rejects_wrong_fee_total_and_extreme_amounts(self):
        (self.data / "payments.txt").write_text(
            f"{A} 60000 10000\nTGOVERFLOW 99999999999999999999999999 0\n"
            f"{B} 50000 123.45\n"
        )
        self.run_cli("5\n1\n0\n0\n", warnings=True)
        rows = (self.data / "unpaid_summary.txt").read_text().splitlines()[1:]
        self.assertEqual([row.split() for row in rows], [[B, "49876.55"]])

    def test_occupancy_uses_same_positive_integer_rules_as_room_module(self):
        (self.data / "rooms.txt").write_text(
            "abc 100 100\n0 100 100\n101 0 0\n999999999999999999999999 4 4\n"
            "2147483647 4 1\n"
        )
        output = self.run_cli("5\n2\n0\n0\n", warnings=True)
        self.assertIn("Overall Occupancy Rate: 25.00%", output)
        self.assertIn("2147483647", output)
        self.assertNotIn("abc", output)

    def test_duplicate_numeric_rooms_refuse_occupancy_rate(self):
        (self.data / "rooms.txt").write_text("101 4 1\n0101 8 8\n")
        output = self.run_cli("5\n2\n0\n0\n", warnings=True)
        self.assertNotIn("Overall Occupancy Rate", output)

    def test_payment_report_rejects_pipe_records_like_fee_writer(self):
        (self.data / "payments.txt").write_text(
            f"{A}|50000|10000\n{B} 50000 123.45\n"
        )
        output = self.run_cli("4\n4\n0\n5\n1\n0\n0\n", warnings=True)
        self.assertIn("Invalid payments.txt", output)
        rows = (self.data / "unpaid_summary.txt").read_text().splitlines()[1:]
        self.assertEqual([row.split() for row in rows], [[B, "49876.55"]])

    def test_occupancy_report_rejects_pipe_records_like_room_writer(self):
        (self.data / "rooms.txt").write_text("101|4|4\n102 4 1\n")
        output = self.run_cli("2\n1\n0\n5\n2\n0\n0\n", warnings=True)
        self.assertIn("invalid or duplicate room", output)
        self.assertIn("Overall Occupancy Rate: 25.00%", output)

    def test_student_summary_counts_unique_normalized_ids(self):
        (self.data / "students.txt").write_text(
            f"{A}|Sample Student|0771234567\n"
            f"{A.lower()}|Sample Student|0771234567\n"
            f"{B}|Other Student|0777654321\n"
        )
        output = self.run_cli("5\n4\n0\n0\n", warnings=True)
        self.assertIn("Total Registered Students in System: 2", output)

    def test_report_save_failure_preserves_existing_destination(self):
        (self.data / "payments.txt").write_text(f"{A} 50000 10000\n")
        destination = self.data / "unpaid_summary.txt"
        destination.mkdir()
        (destination / "previous-report.txt").write_text("keep this report\n")
        self.run_cli("5\n1\n0\n0\n", warnings=True)
        self.assertEqual((destination / "previous-report.txt").read_text(), "keep this report\n")
        self.assertFalse(list(self.data.glob("*.tmp.*")))

    def test_unwritable_directory_keeps_previous_report(self):
        if os.geteuid() == 0:
            self.skipTest("root bypasses directory write permissions")
        (self.data / "payments.txt").write_text(f"{A} 50000 10000\n")
        report = self.data / "unpaid_summary.txt"
        report.write_text("previous verified report\n")
        self.addCleanup(self.data.chmod, 0o755)
        self.data.chmod(0o555)
        self.run_cli("5\n1\n0\n0\n", warnings=True)
        self.assertEqual(report.read_text(), "previous verified report\n")
