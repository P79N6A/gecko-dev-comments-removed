





#include "nsProxyInfo.h"
#include "nsCOMPtr.h"


NS_IMPL_ISUPPORTS(nsProxyInfo, nsProxyInfo, nsIProxyInfo) 

using namespace mozilla;

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



namespace mozilla {
  extern const char kProxyType_HTTP[];
  extern const char kProxyType_HTTPS[];
  extern const char kProxyType_SOCKS[];
  extern const char kProxyType_SOCKS4[];
  extern const char kProxyType_SOCKS5[];
  extern const char kProxyType_DIRECT[];
}

bool
nsProxyInfo::IsDirect()
{
  if (!mType)
    return true;
  return mType == kProxyType_DIRECT;
}

bool
nsProxyInfo::IsHTTP()
{
  return mType == kProxyType_HTTP;
}

bool
nsProxyInfo::IsHTTPS()
{
  return mType == kProxyType_HTTPS;
}

bool
nsProxyInfo::IsSOCKS()
{
  return mType == kProxyType_SOCKS ||
    mType == kProxyType_SOCKS4 || mType == kProxyType_SOCKS5;
}

