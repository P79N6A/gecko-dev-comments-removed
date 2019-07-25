






#include "nsIdleServiceAndroid.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsIdleServiceAndroid, nsIdleService)

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
