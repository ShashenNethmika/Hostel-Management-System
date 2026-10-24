"""Regression coverage for shared input and lossless student edits."""
import subprocess

from cli_support import HostelTestCase, A, B, BINARY


class StudentRegressions(HostelTestCase):
    def test_student_records_require_pipe_delimiters(self):
        original = f"{A} Sample 0771234567\n"
        (self.data / "students.txt").write_text(original)
        output = self.run_cli("1\n2\n0\n5\n4\n0\n0\n", warnings=True)
        self.assertNotIn("Total Registered Students:", output)
        self.assertIn("Total Registered Students in System: 0", output)
        self.assertEqual((self.data / "students.txt").read_text(), original)

    def test_input_errors_use_stderr_and_retry_the_same_field(self):
        output = self.run_cli(f"1\n1\nBAD\n{A}\nName1\nGood Name\nabc\n0771234567\n0\n0\n", warnings=True)
        self.assertIn("STUDENT REGISTERED SUCCESSFULLY", output)
        self.assertIn("Invalid Student ID", self.last_run.stderr)
        self.assertIn("Invalid name", self.last_run.stderr)
        self.assertIn("Invalid Phone Number", self.last_run.stderr)
        self.assertNotIn("Invalid", self.last_run.stdout)

    def test_append_separates_a_record_without_final_newline(self):
        first = f"{A}|Sample Student|0771234567"
        (self.data / "students.txt").write_text(first)
        output = self.run_cli(f"1\n1\n{B}\nOther Student\n0777654321\n2\n0\n0\n")
        self.assertIn("STUDENT REGISTERED SUCCESSFULLY", output)
        self.assertIn("Total Registered Students: 2", output)
        self.assertEqual((self.data / "students.txt").read_text(), first + f"\n{B}|Other Student|0777654321\n")

    def test_duplicate_register_refuses_ambiguous_existing_data(self):
        original = f"{A}|First Student|0771234567\n{A.lower()}|Other Student|0777654321\n"
        (self.data / "students.txt").write_text(original)
        output = self.run_cli(f"1\n1\n{B}\n0\n0\n", warnings=True)
        self.assertNotIn("STUDENT REGISTERED SUCCESSFULLY", output)
        self.assertEqual((self.data / "students.txt").read_text(), original)

    def test_student_list_does_not_count_ambiguous_duplicate_records(self):
        original = f"{A}|First Student|0771234567\n{A.lower()}|Other Student|0777654321\n"
        (self.data / "students.txt").write_text(original)
        output = self.run_cli("1\n2\n0\n0\n", warnings=True)
        self.assertNotIn("Total Registered Students:", output)
        self.assertEqual((self.data / "students.txt").read_text(), original)

    def test_invalid_register_blocks_updates_and_cross_module_writes(self):
        for suffix in ("bad|broken\n", "x" * 600 + f"{A}|Name|0771234567\n",
                       f"{B}|Other Student|0777654321\0hidden\n"):
            with self.subTest(suffix=suffix[:30]):
                original = f"{A}|Sample Student|0771234567\n" + suffix
                (self.data / "students.txt").write_text(original)
                output = self.run_cli(f"1\n4\n{A}\n0\n3\n1\n{A}\n0\n0\n", warnings=True)
                self.assertNotIn("STUDENT UPDATED SUCCESSFULLY", output)
                self.assertEqual((self.data / "students.txt").read_text(), original)
                self.assertEqual((self.data / "attendance.txt").read_text(), "")

    def test_update_does_not_follow_old_temporary_file_symlink(self):
        self.register()
        old_temp = self.data / "temp_students.txt"
        old_temp.symlink_to("students.txt")
        output = self.run_cli(f"1\n4\n{A}\n1\nChanged Student\n0\n0\n")
        self.assertIn("STUDENT UPDATED SUCCESSFULLY", output)
        self.assertFalse((self.data / "students.txt").is_symlink())
        text = (self.data / "students.txt").read_text()
        self.assertIn(f"{A}|Changed Student|0771234567", text)
        self.assertIn(f"{B}|Other Student|0777654321", text)

    def test_embedded_nul_input_is_rejected_as_a_complete_line(self):
        output = self.run_cli(f"1\n1\n{A}\0hidden\n{A}\nBad\0Name\nGood Name\n0771234567\0hidden\n0771234567\n0\n0\n", warnings=True)
        self.assertIn("Invalid input", output)
        self.assertEqual((self.data / "students.txt").read_text(), f"{A}|Good Name|0771234567\n")

    def test_maximum_name_length_is_accepted(self):
        name = "N" * 99
        self.run_cli(f"1\n1\n{A}\n{name}\n0771234567\n0\n0\n")
        self.assertIn(f"{A}|{name}|0771234567", (self.data / "students.txt").read_text())

    def test_crlf_does_not_reduce_maximum_name_length(self):
        name = "N" * 99
        output = self.run_cli(f"1\n1\n{A}\n{name}\n0771234567\n0\n0\n".replace("\n", "\r\n"))
        self.assertIn("STUDENT REGISTERED SUCCESSFULLY", output)
        self.assertEqual((self.data / "students.txt").read_text(), f"{A}|{name}|0771234567\n")

    def test_failed_atomic_registration_preserves_source(self):
        source = self.data / "students.txt"
        # A directory cannot be read as a register; reject instead of reporting success.
        source.unlink()
        source.mkdir()
        result = subprocess.run([str(BINARY), str(self.data)], input="0\n", text=True,
                                cwd=self.root, capture_output=True, timeout=5)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("students.txt", result.stderr)
        self.assertNotIn("STUDENT REGISTERED SUCCESSFULLY", result.stdout)
        self.assertTrue(source.is_dir())
