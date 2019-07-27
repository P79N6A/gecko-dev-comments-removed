




#include "nsChannelPolicy.h"

nsChannelPolicy::nsChannelPolicy()
  : mLoadType(0)
{
}

nsChannelPolicy::~nsChannelPolicy()
{
}

NS_IMPL_ISUPPORTS(nsChannelPolicy, nsIChannelPolicy)

NS_IMETHODIMP
nsChannelPolicy::GetLoadType(uint32_t *aLoadType)
{
    *aLoadType = mLoadType;
    return NS_OK;
}

NS_IMETHODIMP
nsChannelPolicy::SetLoadType(uint32_t aLoadType)
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
