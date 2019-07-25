





































#include "nsNativeConnectionHelper.h"

#if defined(MOZ_PLATFORM_MAEMO)
#include "nsAutodialMaemo.h"
#else
#include "nsAutodialWin.h"
#endif

#include "nsIOService.h"





bool
nsNativeConnectionHelper::OnConnectionFailed(const PRUnichar* hostName)
{
  
  
  
  
#if !defined(MOZ_PLATFORM_MAEMO)
    if (gIOService->IsLinkUp())
        return false;
#endif

    nsAutodial autodial;
    if (autodial.ShouldDialOnNetworkError())
        return NS_SUCCEEDED(autodial.DialDefault(hostName));

    return false;
}

bool
nsNativeConnectionHelper::IsAutodialEnabled()
{
    nsAutodial autodial;
    return autodial.Init() == NS_OK && autodial.ShouldDialOnNetworkError();
}
