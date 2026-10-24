"""Room parsing, file preservation and recoverable two-file updates."""
import resource
import signal
import subprocess

from cli_support import A, BINARY, HostelTestCase


class RoomRegressionTests(HostelTestCase):
    def journal(self, room="101 2 1", allocation=None):
        if allocation is None:
            allocation = f"{A} 101"
        return (
            "HOSTEL_ROOM_TRANSACTION_V1\nROOMS 1\n" + room +
            "\nALLOCATIONS 1\n" + allocation + "\nEND\n"
        )

    def start(self, text="0\n", **options):
        return subprocess.run(
            [str(BINARY), str(self.data)], input=text, text=True,
            cwd=self.root, capture_output=True, timeout=5, **options,
        )

    def test_room_numbers_cannot_wrap_or_omit_separators(self):
        rooms = self.data / "rooms.txt"
        for record in ("4294967397 2 0\n", "101 4294967298 0\n",
                       "101 2 4294967296\n", "101+2+0\n"):
            with self.subTest(record=record):
                rooms.write_text(record)
                result = self.start("2\n0\n")
                self.assertEqual(result.returncode, 0, result.stderr)
                output = result.stderr
                self.assertIn("invalid or duplicate room", output)
                self.assertEqual(rooms.read_text(), record)

    def test_invalid_allocation_numbers_are_not_coerced(self):
        (self.data / "rooms.txt").write_text("101 2 1\n")
        allocations = self.data / "allocations.txt"
        for record in (f"{A} 4294967397\n", "TG" + "A" * 27 + "101\n"):
            with self.subTest(record=record):
                allocations.write_text(record)
                result = self.start("2\n0\n")
                self.assertEqual(result.returncode, 0, result.stderr)
                output = result.stderr
                self.assertIn("invalid allocation", output)
                self.assertEqual(allocations.read_text(), record)

    def test_embedded_nul_records_block_room_edits(self):
        rooms = self.data / "rooms.txt"
        damaged = b"101 2 0\x00hidden\n"
        rooms.write_bytes(damaged)
        result = self.start("2\n0\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        output = result.stderr
        self.assertIn("invalid or unreadable rooms.txt", output)
        self.assertEqual(rooms.read_bytes(), damaged)
        rooms.write_text("101 2 1\n")
        allocations = self.data / "allocations.txt"
        damaged = f"{A} 101".encode() + b"\x00hidden\n"
        allocations.write_bytes(damaged)
        result = self.start("2\n0\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        output = result.stderr
        self.assertIn("invalid or unreadable allocations.txt", output)
        self.assertEqual(allocations.read_bytes(), damaged)

    def test_room_saves_do_not_follow_old_temporary_symlinks(self):
        self.register()
        for name in ("rooms", "allocations"):
            outside = self.root / f"outside-{name}"
            outside.write_text("preserve me")
            (self.data / f"{name}.txt.tmp").symlink_to(outside)
        output = self.run_cli(f"2\n3\n101\n2\n4\n{A}\n101\n0\n0\n")
        self.assertIn(f"Student {A} allocated to room 101", output)
        for name in ("rooms", "allocations"):
            self.assertEqual((self.root / f"outside-{name}").read_text(), "preserve me")
        self.assertEqual((self.data / "rooms.txt").read_text(), "101 2 1\n")
        self.assertEqual((self.data / "allocations.txt").read_text(), f"{A} 101\n")
        self.assertFalse((self.data / "room_transaction.txt").exists())

    def test_pending_transaction_recovers_both_files_before_reading(self):
        (self.data / "rooms.txt").write_text("101 2 0\n")
        (self.data / "room_transaction.txt").write_text(self.journal())
        output = self.run_cli("5\n2\n0\n0\n")
        self.assertIn("Overall Occupancy Rate: 50.00%", output)
        self.assertEqual((self.data / "rooms.txt").read_text(), "101 2 1\n")
        self.assertEqual((self.data / "allocations.txt").read_text(), f"{A} 101\n")
        self.assertFalse((self.data / "room_transaction.txt").exists())

    def test_partial_recovery_is_repeatable_after_file_error(self):
        allocations = self.data / "allocations.txt"
        allocations.unlink()
        allocations.mkdir()  # Prevent the second file replacement, after rooms commit.
        journal = self.data / "room_transaction.txt"
        journal.write_text(self.journal())
        failed = self.start()
        self.assertNotEqual(failed.returncode, 0)
        self.assertIn("Room transaction is pending", failed.stderr)
        self.assertEqual(journal.read_text(), self.journal())
        self.assertEqual((self.data / "rooms.txt").read_text(), "101 2 1\n")
        allocations.rmdir()
        self.run_cli("0\n")
        self.assertEqual(allocations.read_text(), f"{A} 101\n")
        self.assertFalse(journal.exists())

    def test_invalid_transaction_never_replaces_existing_data(self):
        rooms = self.data / "rooms.txt"
        allocations = self.data / "allocations.txt"
        rooms.write_text("101 2 0\n")
        originals = (rooms.read_bytes(), allocations.read_bytes())
        journal = self.data / "room_transaction.txt"
        for content in (self.journal(room="101 2 0"),
                        self.journal(allocation=f"{A} 999"),
                        self.journal() + "extra\n",
                        "HOSTEL_ROOM_TRANSACTION_V1\nROOMS 4294967296\n"):
            with self.subTest(content=content):
                journal.write_text(content)
                result = self.start()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Invalid room transaction journal", result.stderr)
                self.assertEqual((rooms.read_bytes(), allocations.read_bytes()), originals)
                self.assertEqual(journal.read_text(), content)

    def test_console_ids_accept_surrounding_whitespace_and_final_line(self):
        self.register()
        output = self.run_cli(f"2\n3\n101\n2\n4\n  {A.lower()}  \n101\n0\n0")
        self.assertIn(f"Student {A} allocated to room 101", output)
        self.assertEqual((self.data / "allocations.txt").read_text(), f"{A} 101\n")

    def test_pre_journal_write_failure_reloads_tentative_changes(self):
        self.register()
        (self.data / "rooms.txt").write_text("101 2 0\n")

        def refuse_file_writes():
            signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
            resource.setrlimit(resource.RLIMIT_FSIZE, (0, 0))

        result = self.start(
            f"2\n4\n{A}\n101\n6\n1\n4\n{A}\n101\n0\n0\n",
            preexec_fn=refuse_file_writes,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr.count("no recovery journal was created"), 2)
        self.assertNotIn("restart", result.stderr.lower())
        self.assertNotIn("already allocated", result.stderr)
        self.assertIn("No identified students have been allocated", result.stdout)
        self.assertEqual((self.data / "rooms.txt").read_text(), "101 2 0\n")
        self.assertEqual((self.data / "allocations.txt").read_text(), "")
        self.assertFalse((self.data / "room_transaction.txt").exists())

    def test_room_format_errors_retry_without_restarting_operation(self):
        self.register()
        result = self.start(
            f"2\n3\ninvalid\n0\n101\nnope\n-1\n2\n"
            f"4\nBAD\n{A.lower()}\nnot-a-number\n-1\n101\n0\n0\n"
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Invalid number", result.stderr)
        self.assertIn("Invalid Student ID", result.stderr)
        self.assertNotIn("Invalid", result.stdout)
        self.assertIn("Room 101 added successfully", result.stdout)
        self.assertIn(f"Student {A} allocated to room 101", result.stdout)
        self.assertEqual((self.data / "rooms.txt").read_text(), "101 2 1\n")

    def test_room_prompt_eof_cancels_without_writes(self):
        self.register()
        for text in ("2\n3\ninvalid\n", "2\n3\n101\ninvalid\n", "2\n4\nBAD\n"):
            with self.subTest(text=text):
                result = self.start(text)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual((self.data / "rooms.txt").read_text(), "")
                self.assertEqual((self.data / "allocations.txt").read_text(), "")
                self.assertFalse((self.data / "room_transaction.txt").exists())
