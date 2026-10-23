"""Shared real-CLI test fixture. All writes happen in temporary data directories."""
import os
import pathlib
import subprocess
import tempfile
import unittest

PROJECT = pathlib.Path(__file__).resolve().parents[1]
BINARY = pathlib.Path(os.environ.get("HOSTEL_TEST_BINARY", PROJECT / "build/hostel")).resolve()
A = "TG2024000001"
B = "TG2024000002"


class HostelTestCase(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="hostel-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        self.data = self.root / "data"
        self.data.mkdir()
        for name in ("students", "rooms", "allocations", "attendance", "payments"):
            (self.data / f"{name}.txt").write_text("")

    def run_cli(self, text, *, warnings=False):
        result = subprocess.run(
            [str(BINARY), str(self.data)], input=text, text=True,
            cwd=self.root, capture_output=True, timeout=5,
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        if not warnings:
            self.assertEqual(result.stderr, "")
        return result.stdout

    def register(self):
        output = self.run_cli(
            f"1\n1\n{A}\nSample Student\n0771234567\n"
            f"1\n{B}\nOther Student\n0777654321\n0\n0\n"
        )
        self.assertEqual(output.count("STUDENT REGISTERED SUCCESSFULLY"), 2)
