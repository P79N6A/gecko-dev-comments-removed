






































#include "nsIThreadPool.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"

#include "mozStorageCID.h"
#include "mozStorageBackground.h"

namespace {
  class ThreadShutdownObserver : public nsIObserver
  {
  public:
    NS_DECL_ISUPPORTS

    ThreadShutdownObserver(nsIThreadPool *aThreadPool) :
      mThreadPool(aThreadPool)
    {
    }

    NS_IMETHOD Observe(nsISupports *, const char *aTopic, const PRUnichar *)
    {
      if (!strcmp(aTopic, "xpcom-shutdown-threads")) {
        (void)mThreadPool->Shutdown();
        mThreadPool = nsnull;
      }
      return NS_OK;
    }
  private:
    ThreadShutdownObserver() { }
    nsCOMPtr<nsIThreadPool> mThreadPool;
  };
  NS_IMPL_ISUPPORTS1(ThreadShutdownObserver, nsIObserver)
}




mozStorageBackground *mozStorageBackground::mSingleton = nsnull;

mozStorageBackground *
mozStorageBackground::getService()
{
  return mozStorageBackground::mSingleton;
}

mozStorageBackground::mozStorageBackground()
{
  mozStorageBackground::mSingleton = this;
}

mozStorageBackground::~mozStorageBackground()
{
  (void)mThreadPool->Shutdown();
  mozStorageBackground::mSingleton = nsnull;
}

nsIEventTarget *
mozStorageBackground::target()
{
  return mThreadPool;
}

nsresult
mozStorageBackground::initialize()
{
  
  mThreadPool = do_CreateInstance(NS_THREADPOOL_CONTRACTID);
  NS_ENSURE_TRUE(mThreadPool, NS_ERROR_OUT_OF_MEMORY);

  
  mObserver = new ThreadShutdownObserver(mThreadPool);
  NS_ENSURE_TRUE(mObserver, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIObserverService> os =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_TRUE(os, NS_ERROR_UNEXPECTED);

  nsresult rv = os->AddObserver(mObserver, "xpcom-shutdown-threads", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
