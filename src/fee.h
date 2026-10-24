#ifndef FEE_H
#define FEE_H

/* Member 4: Fee & Payment Tracking (called from the leader's main.c). */
void feeMenu(void);
/* Validate and atomically migrate legacy profile snapshots; never edits payments. */
int migrateFeeProfiles(void);

#endif /* FEE_H */
