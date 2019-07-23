




































#include "nsPermission.h"



NS_IMPL_ISUPPORTS1(nsPermission, nsIPermission)

nsPermission::nsPermission(const nsACString &aHost,
                           const nsACString &aType,
                           PRUint32         aCapability)
 : mHost(aHost)
 , mType(aType)
 , mCapability(aCapability)
{
}

nsPermission::~nsPermission()
{
}

NS_IMETHODIMP
nsPermission::GetHost(nsACString &aHost)
{
  aHost = mHost;
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::GetType(nsACString &aType)
{
  aType = mType;
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::GetCapability(PRUint32 *aCapability)
{
  *aCapability = mCapability;
  return NS_OK;
}
