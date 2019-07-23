





































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
nsProxyInfo::GetPort(PRInt32 *result)
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
nsProxyInfo::GetFlags(PRUint32 *result)
{
  *result = mFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsProxyInfo::GetFailoverTimeout(PRUint32 *result)
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
