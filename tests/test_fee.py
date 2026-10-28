"""Fees use actual allocations, current names, exact cents, and safe migrations."""
from cli_support import A, B, HostelTestCase


class FeeRegressionTests(HostelTestCase):
    def test_profile_rejects_allocation_pointing_to_a_missing_room(self):
        self.register()
        (self.data / "rooms.txt").write_text("101 2 1\n")
        (self.data / "allocations.txt").write_text(f"{A} 999999\n")
        self.run_cli(f"4\n1\n{A}\n0\n0\n", warnings=True)
        self.assertIn("invalid allocation", self.last_run.stderr)
        self.assertEqual((self.data / "payments.txt").read_bytes(), b"")
        self.assertFalse((self.data / "fee_students.txt").exists())

    def allocate_a(self):
        (self.data / "rooms.txt").write_text("101 2 1\n")
        (self.data / "allocations.txt").write_text(f"{A} 101\n")

    def add_profile(self, monthly="1000"):
        self.register()
        self.allocate_a()
        return self.run_cli(f"4\n1\n{A}\n{monthly}\n0\n0\n")

    def test_payment_save_ignores_stale_temporary_symlink(self):
        self.register()
        payments = self.data / "payments.txt"
        payments.write_text(f"{A} 5000.00 0.00\n")
        (self.data / "payments_tmp.txt").symlink_to("payments.txt")
        output = self.run_cli(f"4\n2\n{A}\n123.45\n0\n0\n")
        self.assertIn("PAYMENT RECEIPT", output)
        self.assertFalse(payments.is_symlink())
        self.assertEqual(payments.read_text(), f"{A} 5000.00 123.45\n")

    def test_profile_save_ignores_stale_temporary_symlink(self):
        self.register()
        self.allocate_a()
        profiles = self.data / "fee_students.txt"
        profiles.write_text("TGOLDER|Other Student|101|1000.00\n")
        (self.data / "fee_students_tmp.txt").symlink_to("fee_students.txt")
        output = self.run_cli(f"4\n1\n{A}\n1000\n0\n0\n")
        self.assertIn("Student added successfully", output)
        self.assertFalse(profiles.is_symlink())
        self.assertEqual(profiles.read_text(), f"{A}|1000.00\nTGOLDER|1000.00\n")

    def test_embedded_nul_payment_record_is_not_rewritten(self):
        self.register()
        payments = self.data / "payments.txt"
        original = f"{A} 5000.00 0.00".encode() + b"\0 hidden invalid suffix\n"
        payments.write_bytes(original)
        output = self.run_cli(f"4\n2\n{A}\n0\n0\n", warnings=True)
        self.assertIn("Invalid payments.txt", output)
        self.assertNotIn("PAYMENT RECEIPT", output)
        self.assertEqual(payments.read_bytes(), original)
        self.assertIn("Invalid payments.txt", self.last_run.stderr)
        self.assertNotIn("Invalid payments.txt", self.last_run.stdout)

    def test_embedded_nul_profile_record_is_not_rewritten(self):
        self.register()
        profiles = self.data / "fee_students.txt"
        original = b"TGOLDER|Other Student|101|1000.00\0 hidden invalid suffix\n"
        profiles.write_bytes(original)
        output = self.run_cli("4\n0\n", warnings=True)
        self.assertIn("Invalid fee_students.txt", output)
        self.assertEqual(profiles.read_bytes(), original)
        self.assertEqual((self.data / "payments.txt").read_text(), "")

    def test_nul_menu_command_does_not_record_payment(self):
        self.register()
        payments = self.data / "payments.txt"
        original = f"{A} 5000.00 0.00\n"
        payments.write_text(original)
        output = self.run_cli(f"4\n2\0suffix\n{A}\n100\n0\n0\n", warnings=True)
        self.assertIn("Invalid input", output)
        self.assertNotIn("PAYMENT RECEIPT", output)
        self.assertEqual(payments.read_text(), original)

    def test_adding_profile_preserves_existing_payment(self):
        self.register()
        self.allocate_a()
        payments = self.data / "payments.txt"
        payments.write_text(f"{A.lower()} 5000.00 1234.56\n")
        output = self.run_cli(f"4\n1\n{A}\n1000\n3\n{A}\n0\n0\n")
        self.assertIn("Previous Paid: Rs 1234.56 (preserved)", output)
        self.assertIn("Balance Due : Rs 3765.44", output)
        self.assertEqual(payments.read_text(), f"{A.lower()} 5000.00 1234.56\n")

    def test_case_variant_duplicate_payment_records_block_writes(self):
        self.register()
        payments = self.data / "payments.txt"
        original = f"{A} 5000.00 100.00\n{A.lower()} 5000.00 200.00\n"
        payments.write_text(original)
        output = self.run_cli(f"4\n2\n{A}\n0\n0\n", warnings=True)
        self.assertIn("duplicate Student ID", output)
        self.assertEqual(payments.read_text(), original)

    def test_legacy_profile_migration_uses_current_allocation_and_name(self):
        self.register()
        self.allocate_a()
        payments = self.data / "payments.txt"
        original = f"{A} 5000.00 123.45\n"
        payments.write_text(original)
        (self.data / "fee_students.txt").write_text(
            f"  {A.lower()} | Old Name | 999 | 1000.00  \n"
        )
        output = self.run_cli(f"4\n3\n{A}\n0\n0\n")
        self.assertIn("Student Name: Sample Student", output)
        self.assertIn("Room No     : 101", output)
        self.assertNotIn("Old Name", output)
        self.assertNotIn("Room No     : 999", output)
        self.assertEqual((self.data / "fee_students.txt").read_text(), f"{A}|1000.00\n")
        self.assertEqual(payments.read_text(), original)
        self.run_cli("4\n0\n0\n")
        self.assertEqual(payments.read_text(), original)
        self.assertEqual((self.data / "fee_students.txt").read_text(), f"{A}|1000.00\n")

    def test_unknown_and_unallocated_students_cannot_create_profiles(self):
        self.register()
        output = self.run_cli("4\n1\nTGUNKNOWN\n0\n0\n", warnings=True)
        self.assertIn("Student not registered", self.last_run.stderr)
        self.assertNotIn("Student not registered", self.last_run.stdout)
        output = self.run_cli(f"4\n1\n{A}\n0\n0\n", warnings=True)
        self.assertIn("no room allocation", self.last_run.stderr)
        self.assertEqual((self.data / "payments.txt").read_text(), "")
        self.assertFalse((self.data / "fee_students.txt").exists())

    def test_fee_profile_has_only_id_and_monthly_fee(self):
        output = self.add_profile("1000.25")
        self.assertIn("Room No     : 101", output)
        self.assertNotIn("Room No: ", output)
        self.assertEqual((self.data / "fee_students.txt").read_text(), f"{A}|1000.25\n")
        self.assertEqual((self.data / "payments.txt").read_text(), f"{A} 5000.00 0.00\n")

    def test_fee_room_tracks_removal_and_move_without_editing_profile(self):
        self.add_profile()
        profile = (self.data / "fee_students.txt").read_bytes()
        removed = self.run_cli(f"2\n5\n{A}\n0\n4\n3\n{A}\n4\n0\n0\n")
        self.assertIn("Room No     : Unallocated", removed)
        self.assertNotIn("Room No     : 101", removed)
        moved = self.run_cli(f"2\n3\n202\n2\n4\n{A}\n202\n0\n4\n3\n{A}\n4\n0\n0\n")
        self.assertIn("Room No     : 202", moved)
        self.assertEqual((self.data / "fee_students.txt").read_bytes(), profile)
        # Existing debts can still be paid after a room has been removed.
        self.run_cli(f"2\n5\n{A}\n0\n4\n2\n{A}\n12.34\n0\n0\n")
        self.assertEqual((self.data / "payments.txt").read_text(), f"{A} 5000.00 12.34\n")

    def test_shared_name_update_is_visible_without_profile_snapshot(self):
        self.add_profile()
        output = self.run_cli(f"1\n4\n{A}\n1\nUpdated Student\n0\n4\n3\n{A}\n4\n0\n0\n")
        self.assertIn("Student Name: Updated Student", output)
        self.assertEqual((self.data / "fee_students.txt").read_text(), f"{A}|1000.00\n")

    def test_malformed_money_retries_same_prompt_and_eof_cancels(self):
        self.add_profile()
        output = self.run_cli(f"4\n2\n{A}\ninvalid\n-1\n2.333\n5000.01\n123.45\n0\n0\n", warnings=True)
        self.assertEqual(self.last_run.stderr.count("Invalid amount"), 4)
        self.assertNotIn("Invalid amount", self.last_run.stdout)
        self.assertIn("Amount Paid : Rs 123.45", output)
        original = (self.data / "payments.txt").read_bytes()
        self.run_cli(f"4\n2\n{A}\ninvalid\n", warnings=True)
        self.assertEqual((self.data / "payments.txt").read_bytes(), original)

    def test_monthly_fee_retry_and_duplicate_profile_error_stream(self):
        self.register()
        self.allocate_a()
        output = self.run_cli(f"4\n1\nBAD\n{A}\ninvalid\n0\n5000.01\n1000.01\n0\n0\n", warnings=True)
        self.assertIn("Student added successfully", output)
        self.assertIn("Invalid Student ID", self.last_run.stderr)
        self.assertEqual(self.last_run.stderr.count("Invalid amount"), 3)
        self.assertEqual((self.data / "fee_students.txt").read_text(), f"{A}|1000.01\n")
        self.run_cli(f"4\n1\n{A}\n0\n0\n", warnings=True)
        self.assertIn("Duplicate not added", self.last_run.stderr)
        self.assertNotIn("Duplicate not added", self.last_run.stdout)

    def test_invalid_legacy_profile_name_blocks_migration_without_data_loss(self):
        self.register()
        profiles = self.data / "fee_students.txt"
        original = f"{A}|1000.00\n{B}|Name123|101|1000.00\n"
        profiles.write_text(original)
        payments = self.data / "payments.txt"
        money = f"{A} 5000.00 1234.56\n"
        payments.write_text(money)
        self.run_cli("4\n0\n", warnings=True)
        self.assertIn("Invalid fee_students.txt", self.last_run.stderr)
        self.assertEqual(profiles.read_text(), original)
        self.assertEqual(payments.read_text(), money)

    def test_list_handles_large_shared_register_without_per_row_file_loads(self):
        ids = [f"TG{i:08d}" for i in range(2000)]
        (self.data / "students.txt").write_text("".join(f"{sid}|Sample Name|0771234567\n" for sid in ids))
        (self.data / "payments.txt").write_text("".join(f"{sid} 5000.00 123.45\n" for sid in ids))
        (self.data / "fee_students.txt").write_text("".join(f"{sid}|1000.00\n" for sid in ids))
        (self.data / "rooms.txt").write_text(f"101 {len(ids)} {len(ids)}\n")
        (self.data / "allocations.txt").write_text("".join(f"{sid} 101\n" for sid in ids))
        output = self.run_cli("4\n4\n0\n0\n")
        self.assertEqual(output.count("Sample Name"), len(ids))
        self.assertIn("4876.55", output)

    def test_mixed_profile_migration_preserves_monthly_values_and_payments(self):
        self.register()
        self.allocate_a()
        profiles = self.data / "fee_students.txt"
        profiles.write_text(f"{B}|2500.25\n{A.lower()}|Original Name|999|1000.01")
        payments = self.data / "payments.txt"
        original_payments = f"{A.lower()} 5000 543.21\n{B} 5000 321.09\n"
        payments.write_text(original_payments)
        output = self.run_cli("4\n4\n0\n0\n")
        self.assertIn("Sample Student", output)
        self.assertIn("Other Student", output)
        self.assertIn("Unallocated", output)
        self.assertEqual(profiles.read_text(), f"{A}|1000.01\n{B}|2500.25\n")
        self.assertEqual(payments.read_text(), original_payments)

    def test_invalid_or_duplicate_legacy_profiles_do_not_partially_migrate(self):
        self.register()
        profiles = self.data / "fee_students.txt"
        for bad in (
            f"{B}|Other Student|0|1000.00",
            f"{B}|Other Student|101|0.00",
            f"{B}|Other Student|101|5000.01",
            f"{A.lower()}|Other Student|101|1000.00",
        ):
            with self.subTest(bad=bad):
                original = f"{A}|Sample Student|101|1000.00\n{bad}\n"
                profiles.write_text(original)
                self.run_cli("4\n0\n", warnings=True)
                self.assertIn("Invalid fee_students.txt", self.last_run.stderr)
                self.assertEqual(profiles.read_text(), original)
                self.assertEqual((self.data / "payments.txt").read_text(), "")

    def test_eof_while_adding_profile_does_not_create_financial_records(self):
        self.register()
        self.allocate_a()
        self.run_cli(f"4\n1\n{A}\n")
        self.assertEqual((self.data / "payments.txt").read_text(), "")
        self.assertFalse((self.data / "fee_students.txt").exists())

    def test_old_fifty_thousand_payment_data_is_not_scaled_or_rewritten(self):
        self.register()
        self.allocate_a()
        payments = self.data / "payments.txt"
        original = f"{A} 50000.00 12500.00\n".encode()
        payments.write_bytes(original)
        self.run_cli(f"4\n2\n{A}\n0\n0\n", warnings=True)
        self.assertIn("Invalid payments.txt", self.last_run.stderr)
        self.assertIn("must be Rs 5000.00", self.last_run.stderr)
        self.assertIn("payment records are unchanged; review old amounts", self.last_run.stderr)
        self.assertNotIn("PAYMENT RECEIPT", self.last_run.stdout)
        self.assertEqual(payments.read_bytes(), original)
        self.assertFalse((self.data / "fee_students.txt").exists())

    def test_fixed_total_and_full_payment_are_five_thousand(self):
        output = self.add_profile("5000.00")
        self.assertIn("Total Fee   : Rs 5000.00", output)
        self.assertEqual((self.data / "fee_students.txt").read_text(), f"{A}|5000.00\n")
        paid = self.run_cli(f"4\n2\n{A}\n5000.00\n3\n{A}\n4\n5\n0\n0\n")
        self.assertIn("Total: Rs 5000.00", paid)
        self.assertIn("Total Paid  : Rs 5000.00", paid)
        self.assertIn("Balance Due : Rs 0.00", paid)
        self.assertIn("Total Fee per student: Rs 5000.00", paid)
        self.assertIn("0 student(s) owe Rs 0.00 in total.", paid)
        self.assertNotIn("50000", paid)
        self.assertEqual((self.data / "payments.txt").read_text(), f"{A} 5000.00 5000.00\n")
