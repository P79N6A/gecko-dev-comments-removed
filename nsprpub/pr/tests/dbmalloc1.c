


























































#include <stdio.h>
#include <stdlib.h>
#include "nspr.h"

PRIntn failed_already=0;
PRIntn debug_mode;


int should_fail =0;
int actually_failed=0;


void
r1
(
    void
)
{
    int i;
	actually_failed=0;
    for(  i = 0; i < 5; i++ )
    {
        void *x = PR_MALLOC(128);
        if( (void *)0 == x ) {
			if (debug_mode) printf("\tMalloc %d failed.\n", i+1);
			actually_failed = 1;
		}
        PR_DELETE(x);
    }

	if (((should_fail != actually_failed) & (!debug_mode))) failed_already=1;


    return;
}

void
r2
(
    void
)
{
    int i;

    for( i = 0; i <= 5; i++ )
    {
		should_fail =0;
        if( 0 == i ) {
			if (debug_mode) printf("No malloc should fail:\n");
		}
        else {
			if (debug_mode) printf("Malloc %d should fail:\n", i);
			should_fail = 1;
		}
        PR_SetMallocCountdown(i);
        r1();
        PR_ClearMallocCountdown();
    }
}

int main(int argc, char **argv)
{

 
	
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();
    r2();

    if(failed_already)    
        return 1;
    else
        return 0;

    
}

