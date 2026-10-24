"""Check packaged source hygiene without depending on user sample records."""
import subprocess
import unittest

from cli_support import PROJECT


class RepositoryHygiene(unittest.TestCase):
    def test_build_and_generated_artifacts_are_not_tracked(self):
        if (PROJECT / ".git").exists():
            files = subprocess.check_output(["git", "ls-files", "-z"], cwd=PROJECT).decode().split("\0")
            self.assertNotIn("src/hostel", files)
            self.assertNotIn("data/unpaid_summary.txt", files)
            for name in filter(None, files):
                path = PROJECT / name
                self.assertNotEqual(path.read_bytes()[:4], b"\x7fELF", name)
                self.assertNotIn(path.suffix.lower(), (".o", ".obj", ".exe", ".pdf"), name)
        else:
            # Extracted source archives have no Git metadata.
            for path in (PROJECT / "src").rglob("*"):
                if path.is_file():
                    self.assertNotEqual(path.read_bytes()[:4], b"\x7fELF", str(path))

    def test_legacy_hostel_binary_and_generated_report_are_ignored(self):
        patterns = (PROJECT / ".gitignore").read_text().splitlines()
        self.assertIn("/src/hostel", patterns)
        self.assertIn("/data/unpaid_summary.txt", patterns)
