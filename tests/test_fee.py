"""Fee regressions: preserve data on malformed records and safe replacements."""
from cli_support import A, HostelTestCase


class FeeRegressionTests(HostelTestCase):
    def test_payment_save_ignores_stale_temporary_symlink(self):
        self.register()
        payments = self.data / "payments.txt"
        payments.write_text(f"{A} 50000.00 0.00\n")
        (self.data / "payments_tmp.txt").symlink_to("payments.txt")
        output = self.run_cli(f"4\n2\n{A}\n123.45\n0\n0\n")
        self.assertIn("PAYMENT RECEIPT", output)
        self.assertFalse(payments.is_symlink())
        self.assertEqual(payments.read_text(), f"{A} 50000.00 123.45\n")

    def test_profile_save_ignores_stale_temporary_symlink(self):
        self.register()
        profiles = self.data / "fee_students.txt"
        profiles.write_text("TGOLDER|Other Student|101|1000.00\n")
        (self.data / "fee_students_tmp.txt").symlink_to("fee_students.txt")
        output = self.run_cli(f"4\n1\n{A}\n101\n1000\n0\n0\n")
        self.assertIn("Student added successfully", output)
        self.assertFalse(profiles.is_symlink())
        self.assertEqual(
            profiles.read_text(),
            f"TGOLDER|Other Student|101|1000.00\n{A}|Sample Student|101|1000.00\n",
        )

    def test_embedded_nul_payment_record_is_not_rewritten(self):
        self.register()
        payments = self.data / "payments.txt"
        original = f"{A} 50000.00 0.00".encode() + b"\0 hidden invalid suffix\n"
        payments.write_bytes(original)
        output = self.run_cli(f"4\n2\n{A}\n0\n0\n")
        self.assertIn("Invalid payments.txt", output)
        self.assertNotIn("PAYMENT RECEIPT", output)
        self.assertEqual(payments.read_bytes(), original)

    def test_embedded_nul_profile_record_is_not_rewritten(self):
        self.register()
        profiles = self.data / "fee_students.txt"
        original = b"TGOLDER|Other Student|101|1000.00\0 hidden invalid suffix\n"
        profiles.write_bytes(original)
        output = self.run_cli(f"4\n1\n{A}\n0\n0\n")
        self.assertIn("Invalid fee_students.txt", output)
        self.assertEqual(profiles.read_bytes(), original)
        self.assertEqual((self.data / "payments.txt").read_text(), "")

    def test_nul_menu_command_does_not_record_payment(self):
        self.register()
        payments = self.data / "payments.txt"
        original = f"{A} 50000.00 0.00\n"
        payments.write_text(original)
        output = self.run_cli(f"4\n2\0suffix\n{A}\n100\n0\n0\n")
        self.assertIn("Invalid input", output)
        self.assertNotIn("PAYMENT RECEIPT", output)
        self.assertEqual(payments.read_text(), original)

    def test_adding_profile_preserves_existing_payment(self):
        self.register()
        payments = self.data / "payments.txt"
        payments.write_text(f"{A.lower()} 50000.00 12345.67\n")
        output = self.run_cli(f"4\n1\n{A}\n101\n1000\n3\n{A}\n0\n0\n")
        self.assertIn("Previous Paid: Rs 12345.67 (preserved)", output)
        self.assertIn("Balance Due : Rs 37654.33", output)
        self.assertEqual(payments.read_text(), f"{A.lower()} 50000.00 12345.67\n")

    def test_case_variant_duplicate_payment_records_block_writes(self):
        self.register()
        payments = self.data / "payments.txt"
        original = f"{A} 50000.00 100.00\n{A.lower()} 50000.00 200.00\n"
        payments.write_text(original)
        output = self.run_cli(f"4\n2\n{A}\n0\n0\n")
        self.assertIn("duplicate Student ID", output)
        self.assertEqual(payments.read_text(), original)

    def test_profile_fields_allow_surrounding_whitespace(self):
        self.register()
        (self.data / "payments.txt").write_text(f"{A} 50000.00 100.00\n")
        (self.data / "fee_students.txt").write_text(
            f"  {A.lower()}  | Sample Student | 101 | 1000.00  \n"
        )
        output = self.run_cli(f"4\n3\n{A}\n0\n0\n")
        self.assertNotIn("Invalid fee_students.txt", output)
        self.assertIn("Student Name: Sample Student", output)
        self.assertIn("Room No     : 101", output)
        self.assertIn("Monthly Fee : Rs 1000.00", output)
