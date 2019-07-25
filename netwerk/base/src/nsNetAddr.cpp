




































#include "nsNetAddr.h"
#include "nsString.h"

#include "prnetdb.h"

NS_IMPL_ISUPPORTS1(nsNetAddr, nsINetAddr)


nsNetAddr::nsNetAddr(PRNetAddr* addr)
{
  NS_ASSERTION(addr, "null addr");
  mAddr = *addr;
}


NS_IMETHODIMP nsNetAddr::GetFamily(PRUint16 *aFamily)
{
  switch(mAddr.raw.family) {
  case PR_AF_INET: 
    *aFamily = nsINetAddr::FAMILY_INET;
    break;
  case PR_AF_INET6:
    *aFamily = nsINetAddr::FAMILY_INET6;
    break;
  case PR_AF_LOCAL:
    *aFamily = nsINetAddr::FAMILY_LOCAL;
    break;
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetAddress(nsACString & aAddress)
{
  switch(mAddr.raw.family) {
  
  case PR_AF_INET: 
    if(!aAddress.SetCapacity(16))
      return NS_ERROR_OUT_OF_MEMORY;
    PR_NetAddrToString(&mAddr, aAddress.BeginWriting(), 16);
    aAddress.SetLength(strlen(aAddress.BeginReading()));
    break;
  case PR_AF_INET6:
    if(!aAddress.SetCapacity(46))
      return NS_ERROR_OUT_OF_MEMORY;
    PR_NetAddrToString(&mAddr, aAddress.BeginWriting(), 46);
    aAddress.SetLength(strlen(aAddress.BeginReading()));
    break;
#if defined(XP_UNIX) || defined(XP_OS2)
  case PR_AF_LOCAL:
    aAddress.Assign(mAddr.local.path);
    break;
#endif
  
  default:
    return NS_ERROR_UNEXPECTED;
  }
  
  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetPort(PRUint16 *aPort)
{
  switch(mAddr.raw.family) {
  case PR_AF_INET: 
    *aPort = PR_ntohs(mAddr.inet.port);
    break;
  case PR_AF_INET6:
    *aPort = PR_ntohs(mAddr.ipv6.port);
    break;
  case PR_AF_LOCAL:
    
    return NS_ERROR_NOT_AVAILABLE;
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetFlow(PRUint32 *aFlow)
{
  switch(mAddr.raw.family) {
  case PR_AF_INET6:
    *aFlow = PR_ntohl(mAddr.ipv6.flowinfo);
    break;
  case PR_AF_INET: 
  case PR_AF_LOCAL:
    
    return NS_ERROR_NOT_AVAILABLE;
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


NS_IMETHODIMP nsNetAddr::GetScope(PRUint32 *aScope)
{
  switch(mAddr.raw.family) {
  case PR_AF_INET6:
    *aScope = PR_ntohl(mAddr.ipv6.scope_id);
    break;
  case PR_AF_INET: 
  case PR_AF_LOCAL:
    
    return NS_ERROR_NOT_AVAILABLE;
  default:
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

