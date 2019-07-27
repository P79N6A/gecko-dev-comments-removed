



#ifndef _NSNSSCERTCACHE_H_
#define _NSNSSCERTCACHE_H_

#include "nsINSSCertCache.h"
#include "nsIX509CertList.h"
#include "certt.h"
#include "mozilla/Mutex.h"
#include "nsNSSShutDown.h"
#include "nsCOMPtr.h"

class nsNSSCertCache : public nsINSSCertCache,
                       public nsNSSShutDownObject
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSINSSCERTCACHE

  nsNSSCertCache();

protected:
  virtual ~nsNSSCertCache();

private:
  mozilla::Mutex mutex;
  nsCOMPtr<nsIX509CertList> mCertList;
  virtual void virtualDestroyNSSReference() MOZ_OVERRIDE;
  void destructorSafeDestroyNSSReference();
};

#endif
