


























































#include "plgetopt.h"

#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XP_MAC
#include "prlog.h"
#define printf PR_LogPrint
#endif

PRIntn failed_already=0;
PRIntn debug_mode;







static void lowPriority(void *arg)
{
}

static void highPriority(void *arg)
{
}

void runTest(PRThreadScope scope1, PRThreadScope scope2)
{
	PRThread *low,*high;

	
	
	low = PR_CreateThread(PR_USER_THREAD,
				      lowPriority, 0, 
				      PR_PRIORITY_LOW,
				      scope1,
    				  PR_JOINABLE_THREAD,
				      0);
	if (!low) {
		if (debug_mode) printf("\tcannot create low priority thread\n");
		else failed_already=1;
		return;
	}

	high = PR_CreateThread(PR_USER_THREAD,
				      highPriority, 0, 
				      PR_PRIORITY_HIGH,
				      scope2,
    				  PR_JOINABLE_THREAD,
				      0);
	if (!high) {
		if (debug_mode) printf("\tcannot create high priority thread\n");
		else failed_already=1;
		return;
	}

	
	if (PR_JoinThread(low) == PR_FAILURE) {
		if (debug_mode) printf("\tcannot join low priority thread\n");
		else  failed_already=1;
		return;
	} else {
    	if (debug_mode) printf("\tjoined low priority thread\n");
    }
	if (PR_JoinThread(high) == PR_FAILURE) {
		if (debug_mode) printf("\tcannot join high priority thread\n");
		else failed_already=1;
		return;
	} else {
    	if (debug_mode) printf("\tjoined high priority thread\n");
    }
}

static PRIntn PR_CALLBACK RealMain( PRIntn argc, char **argv )
{
	





	
	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  
			debug_mode = 1;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

#ifdef XP_MAC
	SetupMacPrintfLog("join.log");
#endif

	
	
 

    if (debug_mode) printf("Kernel-Kernel test\n");
    runTest(PR_GLOBAL_THREAD, PR_GLOBAL_THREAD);

	if(failed_already)	
	{
        printf("FAIL\n");
		return 1;
	}
	else
	{
        printf("PASS\n");
		return 0;
	}

}

PRIntn main(PRIntn argc, char **argv)
{
    PRIntn rv;
    
    PR_STDIO_INIT();
    rv = PR_Initialize(RealMain, argc, argv, 0);
    return rv;
}  
