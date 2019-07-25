



































#include "nsNSSCertCache.h"
#include "nsNSSCertificate.h"
#include "cert.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsNSSHelper.h"

using namespace mozilla;

NS_IMPL_THREADSAFE_ISUPPORTS1(nsNSSCertCache, nsINSSCertCache)

nsNSSCertCache::nsNSSCertCache()
:mutex("nsNSSCertCache.mutex"), mCertList(nsnull)
{
}

nsNSSCertCache::~nsNSSCertCache()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

void nsNSSCertCache::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void nsNSSCertCache::destructorSafeDestroyNSSReference()
{
  if (isAlreadyShutDown())
    return;
}

NS_IMETHODIMP
nsNSSCertCache::CacheAllCerts()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIInterfaceRequestor> cxt = new PipUIContext();
  
  CERTCertList *newList = PK11_ListCerts(PK11CertListUnique, cxt);

  if (newList) {
    MutexAutoLock lock(mutex);
    mCertList = new nsNSSCertList(newList, PR_TRUE); 
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertCache::CacheCertList(nsIX509CertList *list)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  {
    MutexAutoLock lock(mutex);
    mCertList = list;
    
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertCache::GetX509CachedCerts(nsIX509CertList **list)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  {
    MutexAutoLock lock(mutex);
    if (!mCertList) {
      return NS_ERROR_NOT_AVAILABLE;
    }
    *list = mCertList;
    NS_ADDREF(*list);
  }
  
  return NS_OK;
}



void* nsNSSCertCache::GetCachedCerts()
{
  if (isAlreadyShutDown())
    return nsnull;

  MutexAutoLock lock(mutex);
  return mCertList->GetRawCertList();
}
