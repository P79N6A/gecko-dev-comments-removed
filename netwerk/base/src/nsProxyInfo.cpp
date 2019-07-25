





#include "nsProxyInfo.h"
#include "nsCOMPtr.h"


NS_IMPL_THREADSAFE_ISUPPORTS2(nsProxyInfo, nsProxyInfo, nsIProxyInfo) 

NS_IMETHODIMP
nsProxyInfo::GetHost(nsACString &result)
{
  result = mHost;
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::GetPort(int32_t *result)
{
  *result = mPort;
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::GetType(nsACString &result)
{
  result = mType;
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::GetFlags(uint32_t *result)
{
  *result = mFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::GetResolveFlags(uint32_t *result)
{
  *result = mResolveFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::GetFailoverTimeout(uint32_t *result)
{
  *result = mTimeout;
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::GetFailoverProxy(nsIProxyInfo **result)
{
  NS_IF_ADDREF(*result = mNext);
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::SetFailoverProxy(nsIProxyInfo *proxy)
{
  nsCOMPtr<nsProxyInfo> pi = do_QueryInterface(proxy);
  NS_ENSURE_ARG(pi);

  pi.swap(mNext);
  return NS_OK;
}
