

















































#include "prinit.h"
#include <stdio.h>

#if !defined(IRIX)

int main(int argc, char **argv)
{
    printf("This test applies to IRIX only.\n");
    return 0;
}

#else  

#include "prthread.h"
#include <sys/types.h>
#include <unistd.h>

void SegFault(void *unused)
{
    int *p = 0;

    printf("The grandchild sproc has pid %d.\n", getpid());
    printf("The grandchild sproc will get a segmentation fault and die.\n");
    printf("The parent and child sprocs should be killed after the "
            "grandchild sproc dies.\n");
    printf("Use 'ps' to make sure this is so.\n");
    fflush(stdout);
    
    *p = 0;
}

void NeverStops(void *unused)
{
    int i = 0;

    printf("The child sproc has pid %d.\n", getpid());
    printf("The child sproc won't stop on its own.\n");
    fflush(stdout);

    
    PR_CreateThread(PR_USER_THREAD, SegFault, NULL,
	    PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_UNJOINABLE_THREAD, 0);

    while (1) {
	i++;
    }
}

int main()
{
    int i= 0;

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);

    printf("The parent sproc has pid %d.\n", getpid());
    printf("The parent sproc won't stop on its own.\n");
    fflush(stdout);

    
    PR_CreateThread(PR_USER_THREAD, NeverStops, NULL,
	    PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_UNJOINABLE_THREAD, 0);

    while (1) {
	i++;
    }
    return 0;
}

#endif  
