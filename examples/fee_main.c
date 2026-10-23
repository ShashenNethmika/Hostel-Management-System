#include "fee.h"
#include "common.h"

/* Test main only. The leader's real main() will call feeMenu() the same way. */
int main(int argc, char **argv) {
    if (argc > 2 || !configureDataDirectory(argc == 2 ? argv[1] : NULL)) return 1;
    feeMenu();
    return 0;
}
