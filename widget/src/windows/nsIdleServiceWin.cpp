







































#include "nsIdleServiceWin.h"
#include <windows.h>

NS_IMPL_ISUPPORTS1(nsIdleServiceWin, nsIIdleService)


#ifdef WINCE




extern PRUint32 gLastInputEventTime;
#endif


NS_IMETHODIMP
nsIdleServiceWin::GetIdleTime(PRUint32 *aTimeDiff)
{
#ifndef WINCE
  LASTINPUTINFO inputInfo;
  inputInfo.cbSize = sizeof(inputInfo);
  if (!::GetLastInputInfo(&inputInfo))
    return NS_ERROR_FAILURE;

  *aTimeDiff = SAFE_COMPARE_EVEN_WITH_WRAPPING(GetTickCount(), inputInfo.dwTime);
#else
  
  
  PRUint32 nowTime = PR_IntervalToMicroseconds(PR_IntervalNow());

  *aTimeDiff = SAFE_COMPARE_EVEN_WITH_WRAPPING(nowTime, gLastInputEventTime);
#endif

  return NS_OK;
}
