






#include "nsIdleServiceGonk.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsIdleServiceGonk, nsIdleService)

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
