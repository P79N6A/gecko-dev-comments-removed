



































#include "nsChannelPolicy.h"

nsChannelPolicy::nsChannelPolicy()
  : mLoadType(0)
{
}

nsChannelPolicy::~nsChannelPolicy()
{
}

NS_IMPL_ISUPPORTS1(nsChannelPolicy, nsIChannelPolicy)

NS_IMETHODIMP
nsChannelPolicy::GetLoadType(PRUint32 *aLoadType)
{
    *aLoadType = mLoadType;
    return NS_OK;
}

NS_IMETHODIMP
nsChannelPolicy::SetLoadType(PRUint32 aLoadType)
{
    mLoadType = aLoadType;
    return NS_OK;
}

NS_IMETHODIMP
nsChannelPolicy::GetContentSecurityPolicy(nsISupports **aCSP)
{
    *aCSP = mCSP;
    NS_IF_ADDREF(*aCSP);
    return NS_OK;
}

NS_IMETHODIMP
nsChannelPolicy::SetContentSecurityPolicy(nsISupports *aCSP)
{
    mCSP = aCSP;
    return NS_OK;
}
