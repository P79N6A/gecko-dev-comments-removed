











































#if !defined(WINNT)

#include <stdio.h>

int main(int argc, char **argv)
{
    printf("This test is not relevant on this platform\n");
    return 0;
}

#else 

#include "nspr.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_FILE_NAME "primblok.dat"


static LONG iothread_done;

static void PR_CALLBACK IOThread(void *arg)
{
    PRFileDesc *fd;
    char buf[32];
    PRInt32 nbytes;

    
    Sleep(1000);

    



    fd = PR_Open(TEST_FILE_NAME, PR_WRONLY|PR_CREATE_FILE, 0666);
    if (NULL == fd) {
        fprintf(stderr, "PR_Open failed\n");
        exit(1);
    }
    memset(buf, 0xaf, sizeof(buf));
    fprintf(stderr, "iothread: calling PR_Write\n");
    nbytes = PR_Write(fd, buf, sizeof(buf));
    fprintf(stderr, "iothread: PR_Write returned\n");
    if (nbytes != sizeof(buf)) {
        fprintf(stderr, "PR_Write returned %d\n", nbytes);
        exit(1);
    }
    if (PR_Close(fd) == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    if (PR_Delete(TEST_FILE_NAME) == PR_FAILURE) {
        fprintf(stderr, "PR_Delete failed\n");
        exit(1);
    }

    
    InterlockedExchange(&iothread_done, 1);
}

int main(int argc, char **argv)
{
    PRThread *iothread;

    
    iothread = PR_CreateThread(
        PR_USER_THREAD, IOThread, NULL, PR_PRIORITY_NORMAL,
        PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (iothread == NULL) {
        fprintf(stderr, "cannot create thread\n");
        exit(1);
    }

    



    Sleep(5000);

    






    if (InterlockedExchange(&iothread_done, 1) == 0) {
        fprintf(stderr, "iothread is hung\n");
        fprintf(stderr, "FAILED\n");
        exit(1);
    }

    if (PR_JoinThread(iothread) == PR_FAILURE) {
        fprintf(stderr, "PR_JoinThread failed\n");
        exit(1);
    }
    printf("PASSED\n");
    return 0;
}  

#endif 
