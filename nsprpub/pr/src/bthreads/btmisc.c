




































#include "primpl.h"
#include <stdio.h>






struct protoent* getprotobyname(const char* name)
{
    return 0;
}

struct protoent* getprotobynumber(int number)
{
    return 0;
}


void
_PR_InitStacks (void)
{
}


void
_PR_InitTPD (void)
{
}




PR_IMPLEMENT(void)
    PR_SetConcurrency (PRUintn numCPUs)
{
}




PR_IMPLEMENT(void)
    PR_SetThreadRecycleMode (PRUint32 flag)
{
}





PR_IMPLEMENT(PRWord *)
_MD_HomeGCRegisters( PRThread *t, int isCurrent, int *np )
{
     return 0;
}

PR_IMPLEMENT(void *)
PR_GetSP( PRThread *t )
{
    return 0;
}

PR_IMPLEMENT(PRStatus)
PR_EnumerateThreads( PREnumerator func, void *arg )
{
    return PR_FAILURE;
}
