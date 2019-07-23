









































#include "primpl.h"

void
_PR_MD_INTERVAL_INIT()
{
}

PRIntervalTime 
_PR_MD_GET_INTERVAL()
{
    return timeGetTime();  
}

PRIntervalTime 
_PR_MD_INTERVAL_PER_SEC()
{
    return 1000;
}
