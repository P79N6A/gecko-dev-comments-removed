







































#include "nsIdleServiceWin.h"
#include <windows.h>

NS_IMPL_ISUPPORTS2(nsIdleServiceWin, nsIIdleService, nsIdleService)

bool
nsIdleServiceWin::PollIdleTime(PRUint32 *aIdleTime)
{
    LASTINPUTINFO inputInfo;
    inputInfo.cbSize = sizeof(inputInfo);
    if (!::GetLastInputInfo(&inputInfo))
        return false;

    *aIdleTime = SAFE_COMPARE_EVEN_WITH_WRAPPING(GetTickCount(), inputInfo.dwTime);

    return true;
}

bool
nsIdleServiceWin::UsePollMode()
{
    return true;
}
