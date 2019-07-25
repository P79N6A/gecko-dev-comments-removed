







































#include "nsIdleServiceAndroid.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS2(nsIdleServiceAndroid, nsIIdleService, nsIdleService)

bool
nsIdleServiceAndroid::PollIdleTime(PRUint32 *aIdleTime)
{
    return false;
}

bool
nsIdleServiceAndroid::UsePollMode()
{
    return false;
}
