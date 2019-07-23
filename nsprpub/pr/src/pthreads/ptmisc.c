









































#if defined(_PR_PTHREADS)

#include "primpl.h"

#include <stdio.h>
#ifdef SOLARIS
#include <thread.h>
#endif

#define PT_LOG(f)

void _PR_InitCPUs(void) {PT_LOG("_PR_InitCPUs")}
void _PR_InitStacks(void) {PT_LOG("_PR_InitStacks")}

PR_IMPLEMENT(void) PR_SetConcurrency(PRUintn numCPUs) 
{
#ifdef SOLARIS
	thr_setconcurrency(numCPUs);	
#else
	PT_LOG("PR_SetConcurrency");
#endif
}

PR_IMPLEMENT(void) PR_SetThreadRecycleMode(PRUint32 flag)
    {PT_LOG("PR_SetThreadRecycleMode")}

#endif 


