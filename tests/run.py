"""Run all CLI tests using an optional executable path."""
import os
import pathlib
import sys
import unittest

if len(sys.argv) > 2:
    raise SystemExit("Usage: python3 tests/run.py [executable]")
if len(sys.argv) == 2:
    os.environ["HOSTEL_TEST_BINARY"] = str(pathlib.Path(sys.argv[1]).resolve())
suite = unittest.defaultTestLoader.discover(str(pathlib.Path(__file__).parent), pattern="test_*.py")
suite.addTests(unittest.defaultTestLoader.loadTestsFromName("integration"))
result = unittest.TextTestRunner(verbosity=2).run(suite)
raise SystemExit(not result.wasSuccessful())
