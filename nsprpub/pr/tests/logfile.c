












































#include "prinit.h"
#include "prlog.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{
    PRLogModuleInfo *test_lm;

    if (putenv("NSPR_LOG_MODULES=all:5") != 0) {
        fprintf(stderr, "putenv failed\n");
        exit(1);
    }
    if (putenv("NSPR_LOG_FILE=logfile.log") != 0) {
        fprintf(stderr, "putenv failed\n");
        exit(1);
    }

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    test_lm = PR_NewLogModule("test");
    PR_LOG(test_lm, PR_LOG_MIN, ("logfile: test log message"));
    PR_Cleanup();
    return 0;
}
