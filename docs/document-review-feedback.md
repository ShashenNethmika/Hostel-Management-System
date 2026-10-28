# Developer feedback on the six submitted Word documents

Review date: 28 October 2026. Compared with the integrated C application and
the requested change to a fixed hostel fee of **Rs 5,000.00**. References use
the section titles in the supplied documents, not inferred Word page numbers.

| Developer | Documents reviewed |
| --- | --- |
| Chesmi | `Individual_Contribution_Report_Fee_Module.docx`; `SRS_Document_Fee_Payment_Tracking.docx` |
| Tharushi / S.G.A.T.V Subhawickrama | `Individual_Contribution_Report_TG-IT-2024-2518.docx`; `SRS_Document_TG-IT-2024-2518.docx` |
| Prabodhini | `Individual_Contribution_Report_ (2).docx`; `SRS_Room_Allocation (2).docx` |

The module descriptions and function lists mostly match the implemented work.
The corrections below address identification errors, unfinished sections,
incorrect integration/environment claims, unsupported guarantees and missing
testing evidence. The messages can be copied to the responsible developers.

## Chesmiට යවන්න — Fee & Payment Tracking

Chesmi, Fee SRS එකත් Individual Contribution Report එකත් final code එකට
ගැළපෙන්න පහත corrections කරලා දෙන්න:

1. **Hostel fee එක Rs 5,000.00 ලෙස update කරන්න.** Individual Report එකේ
   B, D, E, F sections සහ SRS 1.2, 1.3, FR-2, FR-4, Data Requirements,
   Input Validation සහ Appendices වල තිබෙන Rs 50,000 / `50000.00` values
   වෙනස් කරන්න. Shared constant එක `common.h` හි
   `HOSTEL_FEE_CENTS = 500000LL`. Monthly-fee input maximum එකත් Rs 5,000.00.
   Total fee එකට අදාළ අලුත් monthly/annual period assumption එකක් එකතු
   කරන්න එපා; program එකේ fixed total එක සහ monthly profile field එක
   වෙන වෙනම පැහැදිලි කරන්න.
2. **Sample records සහ receipt arithmetic නැවත ගණනය කරන්න.** දැන් SRS
   Appendix එකේ monthly fee `8000.00`, total paid `17500.00`, balance
   `32500.00` අලුත් limit එකට ගැළපෙන්නේ නැහැ. එකම scenario එකකට මෙහෙම
   දාන්න පුළුවන්: කලින් paid Rs 1,250; අලුත් payment Rs 500; total paid
   Rs 1,750; balance Rs 3,250. අදාළ ID එක register සහ allocate කර ඇති බව
   example එකේ සඳහන් කරන්න.
3. **Main menu integration එක නිවැරදි කරන්න.** SRS 2.3 සහ 8 හි project
   modules පහම `feeMenu()` යටතේ කියන විස්තර වෙනස් කරන්න. Whole project
   එක `main()`/main menu යටතේ; `feeMenu()` Fees component එකට පමණයි.
   Individual Report G සහ SRS 8 හි actual call chain එක
   `main() -> feeMenu() -> migrateFeeProfiles()` ලෙස ලියන්න.
   `main.c` migration function එක direct call කරන්නේ නැහැ.
4. **Profile creation සහ payment prerequisites වෙන් කරන්න.** අලුත්
   fee profile එකකට registered student සහ actual room allocation අවශ්‍යයි.
   Existing debt එක ගෙවීමට registered student සහ existing payment record
   එක අවශ්‍යයි. Room එකෙන් ඉවත් වුණත් තිබෙන debt එක ගෙවන්න පුළුවන්.
   Profile save එක කලින් fail වුණා නම් payment record එක තිබී profile එක
   අසම්පූර්ණ විය හැකියි. SRS 8 සහ workflow figure එකේ `profile exists`
   label එක මේ actual rule එකට ගළපන්න. Workflow figure එක document එකේ
   තිබෙනවා; ඒක නැති බවක් මෙහි අදහස් කරන්නේ නැහැ.
5. **Save guarantees සීමාව නිවැරදිව ලියන්න.** SRS 7 හි failed operation
   එකකදී කිසිම partial state එකක් ඉතිරි නොවන බව කියන්න එපා. Files
   දෙක වෙන වෙනම atomic save කරනවා. Payment record save වී profile save
   fail වුණොත්, existing payments reset නොකර retry කර profile එක complete
   කළ හැකියි. Existing runtime payments automatically scale කරන බවත්
   ලියන්න එපා; old Rs 50,000 records review කරන තෙක් code ඒවා තබනවා.
6. **Environment claim එක update කරන්න.** SRS 1.5 හි any operating
   system/standard compiler කියන claim එක වෙනුවට tested environment එක
   **Ubuntu/Linux, GCC C11, Make; tests සඳහා Python 3** ලෙස දාන්න.
   Shared file/path helpers වල POSIX functions භාවිතා වෙනවා.
7. **Testing section එකක් එකතු කරන්න.** Individual Report එකේ function
   descriptions තිබුණත් actual test inputs/results නැහැ. Rs 0.01 සහ
   Rs 5,000.00 boundaries, Rs 5,000.01/overpayment rejection, cents,
   duplicate/unregistered/unallocated students, room move/removal, legacy
   migration, EOF සහ old payment data unchanged යන cases සඳහා
   input / expected / actual / pass-fail / command හෝ test reference දාන්න.
8. **ඉතිරි wording/template issues හදන්න.** SRS cover එකේ `[Group No.]`
   confirmed group number එකෙන් පුරවන්න. සෑම duplicate/close error එකකටම
   line number ලැබෙනවා කියන්න එපා: duplicate errors ID එකත්, I/O errors
   filename එකත් දක්වනවා. Imported historical fee records අතර register
   එකේ නැති IDs තිබිය හැකි නිසා “all records are only kept for registered
   students” වෙනුවට new writes සඳහා registration check කරන බව ලියන්න.

Consistent fee example after recording the Rs 500 payment:

```text
payments.txt:     TGIT20252465 5000.00 1750.00
fee_students.txt: TGIT20252465|1000.00

Amount Paid : Rs 500.00
Total Paid  : Rs 1750.00
Balance Due : Rs 3250.00
```

## Tharushiට යවන්න — Attendance & Leave Management

Tharushi, Attendance SRS එකත් Individual Contribution Report එකත් බොහෝ
දුරට code එකට ගැළපෙනවා. Final කරන්න කලින් මේ ටික හදලා දෙන්න:

1. **Index year එක වැරදියි.** Documents දෙකේ title/name fields, SRS
   component description, sample records සහ filenames වල
   `TG/IT/2024/2518` වෙනුවට **TG/IT/2025/2518** දාන්න. Attendance event
   dates වල 2026 තිබීම වෙනම දෙයක්; intake year හදන විට ඒ dates වෙනස්
   කරන්න අවශ්‍ය නැහැ.
2. **Placeholders ඉවත් කරන්න.** Individual Report E හි student register
   filename එක `students.txt`, format එක `ID|Name|Phone` ලෙස දාන්න.
   H හි `Challenge: Challenge Solution: Solution` හිස් row එක ඉවත්
   කරන්න. SRS හි `[to be added]`, `[to be completed]`, `[confirm OS and
   compiler]` වගේ අසම්පූර්ණ තැන් සම්පූර්ණ කරන්න.
3. **Document scope එක පැහැදිලි කරන්න.** මේ SRS එක Attendance component
   draft එකක් නම් ඒ නමින් label කරන්න. Whole project SRS එක ලෙස submit
   කරනවා නම් Student, Room, Fees, Reports requirements අදාළ අයගෙන්
   එකතු කරන්න. SRS 2.3 හි Attendance `Component 2` ලෙස තියෙන අතර main
   menu එකේ choice 3; document numbering සහ menu numbering එකේ සම්බන්ධය
   පැහැදිලි කරලා එකම naming/order එකක් භාවිතා කරන්න.
4. **Environment සහ helper ownership නිවැරදි කරන්න.** SRS 1.5 හි
   standard-library-only claim එක වෙනුවට Ubuntu/Linux, GCC C11 සහ shared
   POSIX helpers තිබෙන බව ලියන්න. `readStudentIndex()` declared වෙන්නේ
   `common.h` තුළයි. `lookupStudent`, register loading/search/freeing සහ
   `struct Student` අයත් වෙන්නේ `student.h` එකටයි.
5. **Input behavior code එකට ගළපන්න.** SRS 6.1 හි invalid ID එකට
   operation එක cancel වෙනවා කියන විස්තර වෙනස් කරන්න. Malformed ID
   format එකකට නැවත අහනවා; EOF එකකට cancel කරනවා. Valid format එකක
   unregistered ID එකක් දුන්නොත් save නොකර operation එක නවත්වනවා.
6. **Formats තුනම සඳහන් කරන්න:** current
   `ID|Name|Status|Date|ExpectedReturn`; legacy
   `ID|Name|Status|Date`; legacy whitespace `ID Status`.
   Whitespace fields 4/5 සහ pipe fields 2 reject වෙනවා. SRS 4 දැනට
   legacy pipe fields 4 format එක සඳහන් කරලා නැහැ.
7. **Corruption behavior වෙන් කරන්න.** Malformed attendance rows warning
   සමඟ skip කරනවා. නමුත් corrupt student register එකක් තිබුණොත්
   marking/report generation එක diagnostic එකක් සමඟ නවත්වනවා;
   partial report එකක් දෙන්නේ නැහැ. SRS Objectives, 6.2 සහ 7 මේ වෙනසට
   ගළපන්න. History view එකේ stored name එක පෙන්වන අතර leave report එකේ
   current registered name එක, නැතිනම් stored/legacy name එක පෙන්වනවා.
8. **Date rules සහ testing evidence එකතු කරන්න.** “Latest” කියන්නේ
   normalized ID එකකට file එකේ අන්තිම valid record එකයි. Due date එක
   today නම් තවම overdue නැහැ; todayට කලින් නම් overdue. Dates සඳහා
   local system timezone භාවිතා වෙන බව සඳහන් කරන්න. Individual Report
   එකට input / expected / actual / pass-fail table එකක් දාලා registered
   සහ unregistered students, invalid dates/IDs, same-day return, overdue,
   returned status, latest-record selection, legacy formats, corrupt
   register සහ EOF cases වල ඇත්තටම run කළ results දාන්න.

## Prabodhiniට යවන්න — Room Allocation & Management

Prabodhini, Room SRS එකත් Individual Contribution Report එකත් current
functions, menu choices, capacity logic සහ limits වලට බොහෝ දුරට ගැළපෙනවා.
මේ corrections කරලා දෙන්න:

1. **නම හදන්න.** Documents දෙකේ `Rathnayaka` වෙනුවට නිවැරදි
   **R.M.P.Prabodhini Rathnayake**, ID **TG IT 2025 2509** යොදන්න.
2. **Environment claim එක හදන්න.** SRS 1.5 හි C99, standard library only,
   Windows/Linux කියන විස්තර වෙනුවට tested build එක වන
   **GCC C11 on Ubuntu/Linux with POSIX helpers** දාන්න.
3. **Recovery guarantee එක overstate කරන්න එපා.** SRS 7 හි power cut
   එකකදීත් files අනිවාර්යයෙන් එකට ගැළපෙනවා කියන claim එකත් Individual
   Report D හි “two files always match” කියන claim එකත් නිවැරදි කරන්න.
   Separate replacements දෙක අතර interruption එකක් ඇති විය හැකියි;
   pending journal එකෙන් supported reads වලට කලින් recover කරනවා.
   Machine power loss එකකට guarantee එකක් දෙන්නේ නැහැ.
4. **Failure stages දෙක වෙන් කරන්න.** Journal commit වීමට කලින්
   failure නම් change එක save වෙන්නේ නැහැ; original data reload කරනවා.
   Reload success නම් room menu එක දිගටම භාවිතා කළ හැකියි. Journal
   commit වුණාට පස්සේ failure නම් update එක pending; journal එක තියාගෙන
   filesystem error හදා recovery කළ යුතුයි. Individual Report C/H සහ
   SRS SS-5 මේ වෙනසට ගළපන්න.
5. **Journal එක backup එකක් ලෙස විස්තර කරන්න එපා.**
   `room_transaction.txt` එක intended new state එක තබන transaction/
   recovery journal එකක්. Recovery fail වුණොත් ඒක short time එකකට පමණක්
   නොව තවදුරටත් තිබිය හැකියි. Malformed journal එකක් තිබුණොත් original
   files replace නොකර startup එක නවත්වන බව සඳහන් කරන්න.
6. **Validation examples හදන්න.** `abc`/`ABC` දෙක valid IDs නෙවෙයි.
   `tgit20252509`/`TGIT20252509` වගේ valid example එකක් දාන්න. Rule එක:
   TG prefix, characters 3–29, letters/digits සහ `/ - _`, case insensitive.
   Room/capacity integers 1…INT_MAX. Slash සහිත සහ slash රහිත IDs
   automatically merge වෙන්නේ නැහැ.
7. **Occupancy calculation එක හදන්න.** Reports integration හි full/empty
   room percentages කියන විස්තර වෙනුවට actual calculation එක වන
   **occupied beds / total bed capacity × 100** දාන්න. Available beds
   පෙන්වන නමුත් separate empty-room percentage එකක් print කරන්නේ නැහැ.
8. **Testing section එකක් එකතු කරන්න.** Duplicate allocation, full room,
   invalid/unregistered ID, removal, failed save සහ pending recovery සඳහා
   input / expected / actual / pass-fail සහ test references දාන්න.
   200-room/10,000-allocation limits code එකේ තිබුණත් boundary tests run
   නොකළා නම් pass කියලා ලියන්න එපා.
9. **API contract එක එකතු කරන්න.** `loadRoomState()` success වූ පසුවයි
   `lookupStudentRoom(id, &roomNo)` භාවිතා කළ යුත්තේ. Return value එක
   found/not-found status; room number එක output parameter එකෙන් ලැබෙනවා.
   Lookup එක memory තුළයි, per-row file read නැහැ. Fee module එක actual
   room allocation ලබාගන්නේ මේ interface එකෙන්.

## Shashen / group leaderට යවන්න

Shashen, මේ documents හය Room, Attendance සහ Fees components තුනට අදාළ
drafts. Final submission එකට:

- Actual group number එක verify කරලා හැම document එකකම එකම විදිහට දාමු;
  Fee SRS එකේ තවම `[Group No.]` තිබෙනවා. GROUP 01 නිවැරදි බව මෙහි
  independent verification එකක් කරලා නැහැ.
- Student component එකට Isharaගේත් Reports component එකට ඔයාගේත්
  requirements/contribution/testing sections එකතු කරලා complete group SRS
  එකක් සකස් කරමු. මේ supplied documents තුළ ඒ complete sections නැහැ;
  වෙනම files තිබෙනවා නම් ඒවා merge කරන්න.
- Developer names/IDs, module names, menu numbering, shared file schemas,
  fee Rs 5,000 rule, dates සහ terminology documents අතර එකම විදිහට තබමු.
- Original individual work සහ පසුව කළ shared integration changes වෙන් කරලා
  actual contribution එක නිවැරදිව ලියමු. Shared helpers සියල්ලම එක්
  අයෙක් ලිව්වා කියන blanket claim එකක් evidence නැතිව දාන්න එපා.
- Contribution reports වල actual test evidence දාමු. Requirements IDs
  වලට අදාළ tests map කරලා current build එකේ `make test` result, tested
  code version/commit සහ test date එක සටහන් කරමු.

## Implemented fee change and validation

- Fixed total: Rs 5,000.00, shared `HOSTEL_FEE_CENTS = 500000LL`.
- Fee validation, saved totals, receipt/list displays, balances and dues reports
  use the same current policy. Sample/demo amounts and test expectations were
  updated consistently.
- Checked-in sample payments are illustrative examples; their paid amounts
  were revised for the new example total. Runtime financial files are not
  automatically rescaled or clamped. Old Rs 50,000 records cause an explicit
  review message, and a previous dues report is preserved.
- Current full suite: **82 tests passed**. Includes Rs 5,000 full payment,
  Rs 5,000.01/overpayment rejection, exact-cent balances, and old-data
  preservation. Test counts describe the code run for this review; developers
  should rerun their final revision before recording submission results.
- The supplied Word documents were reviewed; this feedback does not mean those
  original documents have already been corrected.
