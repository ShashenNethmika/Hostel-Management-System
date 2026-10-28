"""A real first run starts with an empty directory, not precreated data files."""
import subprocess

from cli_support import BINARY, HostelTestCase


REQUIRED = {"students.txt", "rooms.txt", "allocations.txt", "attendance.txt", "payments.txt"}


class FirstRunTests(HostelTestCase):
    def setUp(self):
        super().setUp()
        for path in self.data.iterdir():
            path.unlink()

    def test_empty_directory_creates_only_required_files(self):
        self.assertEqual(list(self.data.iterdir()), [])
        self.run_cli("0\n")
        self.assertEqual({path.name for path in self.data.iterdir()}, REQUIRED)
        for name in REQUIRED:
            self.assertEqual((self.data / name).read_bytes(), b"", name)
        self.assertFalse((self.data / "fee_students.txt").exists())
        self.assertFalse((self.data / "unpaid_summary.txt").exists())

    def test_all_read_menus_and_reports_work_from_truly_empty_directory(self):
        self.assertEqual(list(self.data.iterdir()), [])
        output = self.run_cli(
            "1\n2\n0\n2\n1\n2\n6\n0\n3\n2\n3\n4\n0\n"
            "4\n4\n5\n0\n5\n1\n2\n3\n4\n5\n0\n0\n"
        )
        for expected in (
            "Total Registered Students: 0",
            "No rooms found.",
            "No fee records found.",
            "No unpaid hostel fees found.",
            "Overall Occupancy Rate: 0.00%",
            "Total students currently on leave: 0",
            "Total students overdue: 0",
            "Total Registered Students in System: 0",
        ):
            self.assertIn(expected, output)
        self.assertNotIn("Error reading or writing", output)
        self.assertEqual(self.last_run.stderr, "")
        self.assertFalse((self.data / "fee_students.txt").exists())
        self.assertTrue((self.data / "unpaid_summary.txt").is_file())

    def test_repeated_startup_preserves_existing_record_bytes(self):
        original = {
            "students.txt": b"tgtest|Sample Student|0771234567\r\n",
            "rooms.txt": b"101 2 1",
            "allocations.txt": b"TGTEST 101\r\n",
            "attendance.txt": b"TGTEST|Sample Student|1|2000-01-01|-\n",
            "payments.txt": b"tgtest 5000 123.45",
        }
        for name, content in original.items():
            (self.data / name).write_bytes(content)
        for _ in range(2):
            self.run_cli("0\n")
            self.assertEqual({path.name for path in self.data.iterdir()}, REQUIRED)
            for name, content in original.items():
                self.assertEqual((self.data / name).read_bytes(), content, name)

    def test_partial_missing_dataset_keeps_existing_records(self):
        original = {
            "students.txt": b"TGTEST|Sample Student|0771234567\n",
            "payments.txt": b"TGTEST 5000.00 1234.56\n",
        }
        for name, content in original.items():
            (self.data / name).write_bytes(content)
        self.run_cli("0\n")
        self.assertEqual({path.name for path in self.data.iterdir()}, REQUIRED)
        for name in REQUIRED:
            self.assertEqual((self.data / name).read_bytes(), original.get(name, b""), name)

    def test_initialization_does_not_erase_existing_invalid_records(self):
        original = b"broken student\0record\n"
        (self.data / "students.txt").write_bytes(original)
        self.run_cli("0\n")
        self.assertEqual((self.data / "students.txt").read_bytes(), original)
        self.run_cli("1\n2\n0\n0\n", warnings=True)
        self.assertIn("Invalid students.txt", self.last_run.stderr)
        self.assertEqual((self.data / "students.txt").read_bytes(), original)

    def test_uncreatable_required_file_reports_failure_without_losing_data(self):
        original = b"TGTEST|Sample Student|0771234567\n"
        (self.data / "students.txt").write_bytes(original)
        rooms = self.data / "rooms.txt"
        rooms.symlink_to("missing-parent/rooms.txt")
        result = subprocess.run(
            [str(BINARY), str(self.data)], input="0\n", text=True,
            cwd=self.root, capture_output=True, timeout=5,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("rooms.txt", result.stderr)
        self.assertNotIn("HOSTEL MANAGEMENT SYSTEM", result.stdout)
        self.assertEqual((self.data / "students.txt").read_bytes(), original)
        self.assertTrue(rooms.is_symlink())

    def test_required_file_directory_is_not_accepted_as_initialized_data(self):
        (self.data / "students.txt").mkdir()
        result = subprocess.run(
            [str(BINARY), str(self.data)], input="0\n", text=True,
            cwd=self.root, capture_output=True, timeout=5,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("students.txt", result.stderr)
        self.assertNotIn("HOSTEL MANAGEMENT SYSTEM", result.stdout)
        self.assertTrue((self.data / "students.txt").is_dir())
