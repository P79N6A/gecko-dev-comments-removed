





































#include "nsNativeConnectionHelper.h"
#include "nsAutodialWin.h"





PRBool
nsNativeConnectionHelper::OnConnectionFailed(const char* hostName)
{
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
