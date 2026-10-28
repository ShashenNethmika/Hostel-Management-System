"""All member modules use the same identities in the demonstration dataset."""
import shutil

from cli_support import HostelTestCase, PROJECT


class CoherentDemoTests(HostelTestCase):
    def test_all_five_modules_share_demo_records(self):
        shutil.copytree(PROJECT / "examples/demo-data", self.data, dirs_exist_ok=True)
        original = {path.name: path.read_bytes() for path in self.data.glob("*.txt")}
        output = self.run_cli(
            "1\n2\n3\nTGDEMO000001\n0\n2\n1\n6\n0\n"
            "3\n2\n3\n4\n0\n4\n3\nTGDEMO000001\n4\n5\n0\n"
            "5\n1\n2\n3\n4\n5\n0\n0\n"
        )
        for expected in ("Student Name: Demo Student One", "Room No     : 101",
                         "Balance Due : Rs 4000.00", "Overall Occupancy Rate: 50.00%",
                         "Total students currently on leave: 1", "Total students overdue: 1",
                         "Total Registered Students in System: 2"):
            self.assertIn(expected, output)
        self.assertNotIn("(not registered)", output)
        self.assertNotIn("legacy record", output)
        for name, content in original.items():
            self.assertEqual((self.data / name).read_bytes(), content, name)

    def test_demo_identity_flows_through_room_attendance_payment_and_reports(self):
        shutil.copytree(PROJECT / "examples/demo-data", self.data, dirs_exist_ok=True)
        output = self.run_cli(
            "1\n1\nTGDEMO000003\nDemo Student Three\n0770000003\n0\n"
            "2\n4\nTGDEMO000003\n101\n0\n"
            "3\n1\nTGDEMO000003\n1\n0\n"
            "4\n1\nTGDEMO000003\n1000\n2\nTGDEMO000003\n123.45\n3\nTGDEMO000003\n0\n"
            "5\n1\n2\n3\n4\n0\n0\n"
        )
        for expected in ("STUDENT REGISTERED SUCCESSFULLY", "allocated to room 101",
                         "Attendance saved for Demo Student Three", "Student Name: Demo Student Three",
                         "Balance Due : Rs 4876.55", "Overall Occupancy Rate: 75.00%",
                         "Total Registered Students in System: 3"):
            self.assertIn(expected, output)
        self.assertIn("TGDEMO000003 5000.00 123.45", (self.data / "payments.txt").read_text())
        self.assertIn("TGDEMO000003", (self.data / "unpaid_summary.txt").read_text())
