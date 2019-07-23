







































#include "nsIdleServiceWin.h"
#include <windows.h>

NS_IMPL_ISUPPORTS1(nsIdleServiceWin, nsIIdleService)

NS_IMETHODIMP
nsIdleServiceWin::GetIdleTime(PRUint32 *aTimeDiff)
{
    LASTINPUTINFO inputInfo;
    inputInfo.cbSize = sizeof(inputInfo);
    if (!::GetLastInputInfo(&inputInfo))
        return NS_ERROR_FAILURE;

    *aTimeDiff = GetTickCount() - inputInfo.dwTime;
    return NS_OK;
}
