




#include "nspr.h"
#include "pprthred.h"
#include "plgetopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XP_BEOS






static void PR_CALLBACK thread_start(void *arg)
{
PRUint32 mask = 0;

	if (PR_GetThreadAffinityMask(PR_GetCurrentThread(), &mask)) 
		printf("\tthread_start: PR_GetCurrentThreadAffinityMask failed\n");
	else
		printf("\tthread_start: AffinityMask = 0x%x\n",mask);

}

int main(int argc, char **argv)
{
	PRThread *t;

	printf("main: creating local thread\n");

	t = PR_CreateThread(PR_USER_THREAD,
				  thread_start, 0, 
				  PR_PRIORITY_NORMAL,
				  PR_LOCAL_THREAD,
				  PR_JOINABLE_THREAD,
				  0);

	if (NULL == t) {
		printf("main: cannot create local thread\n");
		exit(1);
	}

	PR_JoinThread(t);

	printf("main: creating global thread\n");
	t = PR_CreateThread(PR_USER_THREAD,
				  thread_start, 0, 
				  PR_PRIORITY_NORMAL,
				  PR_GLOBAL_THREAD,
				  PR_JOINABLE_THREAD,
				  0);

	if (NULL == t) {
		printf("main: cannot create global thread\n");
		exit(1);
	}

	PR_JoinThread(t);

	printf("main: creating global bound thread\n");
	t = PR_CreateThread(PR_USER_THREAD,
				  thread_start, 0, 
				  PR_PRIORITY_NORMAL,
				  PR_GLOBAL_BOUND_THREAD,
				  PR_JOINABLE_THREAD,
				  0);

	if (NULL == t) {
		printf("main: cannot create global bound thread\n");
		exit(1);
	}

	PR_JoinThread(t);

    return 0;
}

#else 

int main()
{
	printf( "This test is not supported on the BeOS\n" );
	return 0;
}
#endif 
