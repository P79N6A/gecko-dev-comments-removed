





































#include "nsNativeConnectionHelper.h"
#include "nsAutodialWin.h"
#include "nsIOService.h"





PRBool
nsNativeConnectionHelper::OnConnectionFailed(const PRUnichar* hostName)
{
    if (gIOService->IsLinkUp())
        return PR_FALSE;

    nsRASAutodial autodial;

    if (autodial.ShouldDialOnNetworkError()) 
        return NS_SUCCEEDED(autodial.DialDefault(hostName));
    else
        return PR_FALSE;
}

PRBool
nsNativeConnectionHelper::IsAutodialEnabled()
{
    nsRASAutodial autodial;

    return autodial.Init() == NS_OK && autodial.ShouldDialOnNetworkError();
}
