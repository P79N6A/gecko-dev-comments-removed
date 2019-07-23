







































#include "nspr.h"
#include <stdio.h>

int main()
{
    PRStatus rv;

    fprintf(stderr, "Init 1\n");
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    fprintf(stderr, "Cleanup 1\n");
    rv = PR_Cleanup();
    if (rv != PR_SUCCESS) {
        fprintf(stderr, "FAIL\n");
        return 1;
    }

    fprintf(stderr, "Init 2\n");
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    fprintf(stderr, "Cleanup 2\n");
    rv = PR_Cleanup();
    if (rv != PR_SUCCESS) {
        fprintf(stderr, "FAIL\n");
        return 1;
    }

    fprintf(stderr, "PASS\n");
    return 0;
}
