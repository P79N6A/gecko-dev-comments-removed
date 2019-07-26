





#include "nsNetAddr.h"
#include "nsString.h"
#include "prnetdb.h"
#include "mozilla/net/DNS.h"

using namespace mozilla::net;

NS_IMPL_ISUPPORTS1(nsNetAddr, nsINetAddr)


nsNetAddr::nsNetAddr(NetAddr* addr)
{
  NS_ASSERTION(addr, "null addr");
  mAddr = *addr;
}


NS_IMETHODIMP nsNetAddr::GetFamily(uint16_t *aFamily)
{
  switch(mAddr.raw.family) {
  case AF_INET:
    *aFamily = nsINetAddr::FAMILY_INET;
    break;
  case AF_INET6:
    *aFamily = nsINetAddr::FAMILY_INET6;
    break;
#if defined(XP_UNIX) || defined(XP_OS2)
  case AF_LOCAL:
    *aFamily = nsINetAddr::FAMILY_LOCAL;
    break;
#endif
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetAddress(nsACString & aAddress)
{
  switch(mAddr.raw.family) {
  
  case AF_INET:
    aAddress.SetCapacity(kIPv4CStrBufSize);
    NetAddrToString(&mAddr, aAddress.BeginWriting(), kIPv4CStrBufSize);
    aAddress.SetLength(strlen(aAddress.BeginReading()));
    break;
  case AF_INET6:
    aAddress.SetCapacity(kIPv6CStrBufSize);
    NetAddrToString(&mAddr, aAddress.BeginWriting(), kIPv6CStrBufSize);
    aAddress.SetLength(strlen(aAddress.BeginReading()));
    break;
#if defined(XP_UNIX) || defined(XP_OS2)
  case AF_LOCAL:
    aAddress.Assign(mAddr.local.path);
    break;
#endif
  
  default:
    return NS_ERROR_UNEXPECTED;
  }
  
  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetPort(uint16_t *aPort)
{
  switch(mAddr.raw.family) {
  case AF_INET:
    *aPort = ntohs(mAddr.inet.port);
    break;
  case AF_INET6:
    *aPort = ntohs(mAddr.inet6.port);
    break;
#if defined(XP_UNIX) || defined(XP_OS2)
  case AF_LOCAL:
    
    return NS_ERROR_NOT_AVAILABLE;
#endif
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetFlow(uint32_t *aFlow)
{
  switch(mAddr.raw.family) {
  case AF_INET6:
    *aFlow = ntohl(mAddr.inet6.flowinfo);
    break;
  case AF_INET:
#if defined(XP_UNIX) || defined(XP_OS2)
  case AF_LOCAL:
#endif
    
    return NS_ERROR_NOT_AVAILABLE;
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetScope(uint32_t *aScope)
{
  switch(mAddr.raw.family) {
  case AF_INET6:
    *aScope = ntohl(mAddr.inet6.scope_id);
    break;
  case AF_INET:
#if defined(XP_UNIX) || defined(XP_OS2)
  case AF_LOCAL:
#endif
    
    return NS_ERROR_NOT_AVAILABLE;
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetIsV4Mapped(bool *aIsV4Mapped)
{
  switch(mAddr.raw.family) {
  case AF_INET6:
    *aIsV4Mapped = IPv6ADDR_IS_V4MAPPED(&mAddr.inet6.ip);
    break;
  case AF_INET:
#if defined(XP_UNIX) || defined(XP_OS2)
  case AF_LOCAL:
#endif
    
    return NS_ERROR_NOT_AVAILABLE;
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

