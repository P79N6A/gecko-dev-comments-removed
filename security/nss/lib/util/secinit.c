



































#include "nspr.h"
#include "secport.h"

static int sec_inited = 0;

void 
SEC_Init(void)
{
    
#if !defined(SERVER_BUILD)
    PORT_Assert(PR_Initialized() == PR_TRUE);
#endif
    if (sec_inited)
	return;

    sec_inited = 1;
}
