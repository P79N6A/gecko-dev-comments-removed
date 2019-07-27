





#include "nsAutoPtr.h"
#include "nsIObserverService.h"
#include "nsIUUIDGenerator.h"
#include "SchedulingContextService.h"

#include "mozilla/Atomics.h"
#include "mozilla/Services.h"

#include "mozilla/net/PSpdyPush.h"

namespace mozilla {
namespace net {


class SchedulingContext final : public nsISchedulingContext
                              , public nsSupportsWeakReference
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISCHEDULINGCONTEXT

  explicit SchedulingContext(const nsID& id);
private:
  virtual ~SchedulingContext() {}

  nsID mID;
  Atomic<uint32_t>       mBlockingTransactionCount;
  nsAutoPtr<SpdyPushCache> mSpdyCache;
};

NS_IMPL_ISUPPORTS(SchedulingContext, nsISchedulingContext, nsISupportsWeakReference)

SchedulingContext::SchedulingContext(const nsID& aID)
  : mBlockingTransactionCount(0)
{
  mID = aID;
}

NS_IMETHODIMP
SchedulingContext::GetBlockingTransactionCount(uint32_t *aBlockingTransactionCount)
{
  NS_ENSURE_ARG_POINTER(aBlockingTransactionCount);
  *aBlockingTransactionCount = mBlockingTransactionCount;
  return NS_OK;
}

NS_IMETHODIMP
SchedulingContext::AddBlockingTransaction()
{
  mBlockingTransactionCount++;
  return NS_OK;
}

NS_IMETHODIMP
SchedulingContext::RemoveBlockingTransaction(uint32_t *outval)
{
  NS_ENSURE_ARG_POINTER(outval);
  mBlockingTransactionCount--;
  *outval = mBlockingTransactionCount;
  return NS_OK;
}


NS_IMETHODIMP
SchedulingContext::GetSpdyPushCache(mozilla::net::SpdyPushCache **aSpdyPushCache)
{
  *aSpdyPushCache = mSpdyCache.get();
  return NS_OK;
}

NS_IMETHODIMP
SchedulingContext::SetSpdyPushCache(mozilla::net::SpdyPushCache *aSpdyPushCache)
{
  mSpdyCache = aSpdyPushCache;
  return NS_OK;
}


NS_IMETHODIMP
SchedulingContext::GetID(nsID *outval)
{
  NS_ENSURE_ARG_POINTER(outval);
  *outval = mID;
  return NS_OK;
}


SchedulingContextService *SchedulingContextService::sSelf = nullptr;

NS_IMPL_ISUPPORTS(SchedulingContextService, nsISchedulingContextService, nsIObserver)

SchedulingContextService::SchedulingContextService()
{
  MOZ_ASSERT(!sSelf, "multiple scs instances!");
  sSelf = this;
}

SchedulingContextService::~SchedulingContextService()
{
  Shutdown();
  sSelf = nullptr;
}

nsresult
SchedulingContextService::Init()
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}

void
SchedulingContextService::Shutdown()
{
  mTable.Clear();
}

 nsresult
SchedulingContextService::Create(nsISupports *aOuter, const nsIID& aIID, void **aResult)
{
  if (aOuter != nullptr) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsRefPtr<SchedulingContextService> svc = new SchedulingContextService();
  nsresult rv = svc->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return svc->QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
SchedulingContextService::GetSchedulingContext(const nsID& scID, nsISchedulingContext **sc)
{
  NS_ENSURE_ARG_POINTER(sc);
  *sc = nullptr;

  nsWeakPtr weakSC;
  nsCOMPtr<nsISchedulingContext> strongSC;
  if (mTable.Get(scID, getter_AddRefs(weakSC))) {
    strongSC = do_QueryReferent(weakSC);
  }

  if (!strongSC) {
    
    
    strongSC = new SchedulingContext(scID);
    weakSC = do_GetWeakReference(strongSC);
    if (!weakSC) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTable.Put(scID, weakSC);
  }

  strongSC.swap(*sc);

  return NS_OK;
}

NS_IMETHODIMP
SchedulingContextService::NewSchedulingContextID(nsID *scID)
{
  if (!mUUIDGen) {
    nsresult rv;
    mUUIDGen = do_GetService("@mozilla.org/uuid-generator;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return mUUIDGen->GenerateUUIDInPlace(scID);
}

NS_IMETHODIMP
SchedulingContextService::Observe(nsISupports *subject, const char *topic,
                                  const char16_t *data_unicode)
{
  if (!strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, topic)) {
    Shutdown();
  }

  return NS_OK;
}

} 
} 
