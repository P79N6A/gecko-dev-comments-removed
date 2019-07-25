



































#ifndef _NSNSSCERTCACHE_H_
#define _NSNSSCERTCACHE_H_

#include "nsINSSCertCache.h"
#include "nsIX509CertList.h"
#include "certt.h"
#include "nsNSSShutDown.h"
#include "nsCOMPtr.h"

class nsNSSCertCache : public nsINSSCertCache,
                       public nsNSSShutDownObject
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINSSCERTCACHE

  nsNSSCertCache();
  virtual ~nsNSSCertCache();

private:
  PRLock *mutex;
  nsCOMPtr<nsIX509CertList> mCertList;
  virtual void virtualDestroyNSSReference();
  void destructorSafeDestroyNSSReference();
};

#endif
