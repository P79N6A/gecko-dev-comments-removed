



#include "nsNativeConnectionHelper.h"
#include "nsAutodialWin.h"
#include "nsIOService.h"





bool
nsNativeConnectionHelper::OnConnectionFailed(const char16_t* hostName)
{
    if (gIOService->IsLinkUp())
        return false;

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
