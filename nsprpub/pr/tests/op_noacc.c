


















#include "prinit.h"
#include "prmem.h"
#include "prio.h"
#include "prerror.h"
#include <stdio.h>
#include "plgetopt.h"

static PRFileDesc *err01;
PRIntn error_code;

int main(int argc, char **argv)
{
#ifdef XP_PC
    printf("op_noacc: Test not valid on MS-Windows.\n\tNo concept of 'mode' on Open() call\n");
    return(0);
#endif


    PR_STDIO_INIT();
    err01 = PR_Open("err01.tmp", PR_CREATE_FILE | PR_RDWR, 0);
    if (err01 == NULL) {
        int error = PR_GetError();
        printf ("error code is %d\n", error);
        if (error == PR_NO_ACCESS_RIGHTS_ERROR) {
            printf ("PASS\n");
            return 0;
        }
    }
    printf ("FAIL\n");
    return 1;
}

