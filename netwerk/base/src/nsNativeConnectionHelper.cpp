





































#include "nsNativeConnectionHelper.h"

#if defined(MOZ_ENABLE_LIBCONIC)
#include "nsAutodialMaemo.h"
#elif defined(WINCE)
#include "nsAutodialWinCE.h"
#else
#include "nsAutodialWin.h"
#endif

#include "nsIOService.h"





PRBool
nsNativeConnectionHelper::OnConnectionFailed(const PRUnichar* hostName)
{
  
  
  
  
#if !defined(MOZ_ENABLE_LIBCONIC) && !defined(WINCE_WINDOWS_MOBILE)
    if (gIOService->IsLinkUp())
        return PR_FALSE;
#endif

    nsAutodial autodial;
    if (autodial.ShouldDialOnNetworkError())
        return NS_SUCCEEDED(autodial.DialDefault(hostName));

    return PR_FALSE;
}

PRBool
nsNativeConnectionHelper::IsAutodialEnabled()
{
    nsAutodial autodial;
    return autodial.Init() == NS_OK && autodial.ShouldDialOnNetworkError();
}
