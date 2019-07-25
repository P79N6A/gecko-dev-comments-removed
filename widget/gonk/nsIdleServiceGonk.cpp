








































#include "nsIdleServiceGonk.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS2(nsIdleServiceGonk, nsIIdleService, nsIdleService)

bool
nsIdleServiceGonk::PollIdleTime(PRUint32 *aIdleTime)
{
    return false;
}

bool
nsIdleServiceGonk::UsePollMode()
{
    return false;
}
