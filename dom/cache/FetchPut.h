





#ifndef mozilla_dom_cache_FetchPut_h
#define mozilla_dom_cache_FetchPut_h

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/cache/Manager.h"
#include "mozilla/dom/cache/PCacheTypes.h"
#include "mozilla/dom/cache/Types.h"
#include "mozilla/dom/cache/TypeUtils.h"
#include "nsRefPtr.h"
#include "nsTArray.h"
#include <utility>

class nsIInputStream;
class nsIRunnable;
class nsIThread;

namespace mozilla {
namespace dom {

class Request;
class Response;

namespace cache {

class FetchPut final : public Manager::Listener
                     , public TypeUtils
{
public:
  typedef std::pair<nsRefPtr<Request>, nsRefPtr<Response>> PutPair;

  class Listener
  {
  public:
    virtual void
    OnFetchPut(FetchPut* aFetchPut, RequestId aRequestId, nsresult aRv) = 0;
  };

  static nsresult
  Create(Listener* aListener, Manager* aManager,
         RequestId aRequestId, CacheId aCacheId,
         const nsTArray<PCacheRequest>& aRequests,
         const nsTArray<nsCOMPtr<nsIInputStream>>& aRequestStreams,
         FetchPut** aFetchPutOut);

  void ClearListener();

private:
  class Runnable;
  class FetchObserver;
  friend class FetchObserver;
  struct State
  {
    PCacheRequest mPCacheRequest;
    nsCOMPtr<nsIInputStream> mRequestStream;
    nsRefPtr<FetchObserver> mFetchObserver;
    PCacheResponse mPCacheResponse;
    nsCOMPtr<nsIInputStream> mResponseStream;

    nsRefPtr<Request> mRequest;
    nsRefPtr<Response> mResponse;
  };

  FetchPut(Listener* aListener, Manager* aManager,
           RequestId aRequestId, CacheId aCacheId,
           const nsTArray<PCacheRequest>& aRequests,
           const nsTArray<nsCOMPtr<nsIInputStream>>& aRequestStreams);
  ~FetchPut();

  nsresult DispatchToMainThread();
  void DispatchToInitiatingThread();

  void DoFetchOnMainThread();
  void FetchComplete(FetchObserver* aObserver,
                     InternalResponse* aInternalResponse);
  void MaybeCompleteOnMainThread();

  void DoPutOnWorkerThread();
  static bool MatchInPutList(const PCacheRequest& aRequest,
                             const nsTArray<CacheRequestResponse>& aPutList);
  virtual void OnCachePutAll(RequestId aRequestId, nsresult aRv) override;

  void MaybeSetError(nsresult aRv);
  void MaybeNotifyListener();

  
  virtual nsIGlobalObject* GetGlobalObject() const override;
#ifdef DEBUG
  virtual void AssertOwningThread() const override;
#endif

  virtual CachePushStreamChild*
  CreatePushStream(nsIAsyncInputStream* aStream) override;

  Listener* mListener;
  nsRefPtr<Manager> mManager;
  const RequestId mRequestId;
  const CacheId mCacheId;
  nsCOMPtr<nsIThread> mInitiatingThread;
  nsTArray<State> mStateList;
  uint32_t mPendingCount;
  nsresult mResult;
  nsCOMPtr<nsIRunnable> mRunnable;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(mozilla::dom::cache::FetchPut)
};

} 
} 
} 

#endif
