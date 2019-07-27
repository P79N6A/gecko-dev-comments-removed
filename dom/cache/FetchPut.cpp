





#include "mozilla/dom/cache/FetchPut.h"

#include "mozilla/dom/Fetch.h"
#include "mozilla/dom/FetchDriver.h"
#include "mozilla/dom/Headers.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseNativeHandler.h"
#include "mozilla/dom/Request.h"
#include "mozilla/dom/Response.h"
#include "mozilla/dom/ResponseBinding.h"
#include "mozilla/dom/UnionTypes.h"
#include "mozilla/dom/cache/ManagerId.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsCRT.h"
#include "nsHttp.h"

namespace mozilla {
namespace dom {
namespace cache {

class FetchPut::Runnable final : public nsRunnable
{
public:
  explicit Runnable(FetchPut* aFetchPut)
    : mFetchPut(aFetchPut)
  {
    MOZ_ASSERT(mFetchPut);
  }

  NS_IMETHOD Run() override
  {
    if (NS_IsMainThread())
    {
      mFetchPut->DoFetchOnMainThread();
      return NS_OK;
    }

    MOZ_ASSERT(mFetchPut->mInitiatingThread == NS_GetCurrentThread());

    mFetchPut->DoPutOnWorkerThread();

    
    
    
    mFetchPut = nullptr;

    return NS_OK;
  }

private:
  nsRefPtr<FetchPut> mFetchPut;
};

class FetchPut::FetchObserver final : public FetchDriverObserver
{
public:
  explicit FetchObserver(FetchPut* aFetchPut)
    : mFetchPut(aFetchPut)
  {
  }

  virtual void OnResponseAvailable(InternalResponse* aResponse) override
  {
    MOZ_ASSERT(!mInternalResponse);
    mInternalResponse = aResponse;
  }

  virtual void OnResponseEnd() override
  {
    mFetchPut->FetchComplete(this, mInternalResponse);
    if (mFetchPut->mInitiatingThread == NS_GetCurrentThread()) {
      mFetchPut = nullptr;
    } else {
      nsCOMPtr<nsIThread> initiatingThread(mFetchPut->mInitiatingThread);
      nsCOMPtr<nsIRunnable> runnable =
        NS_NewNonOwningRunnableMethod(mFetchPut.forget().take(), &FetchPut::Release);
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        initiatingThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL)));
    }
  }

protected:
  virtual ~FetchObserver() { }

private:
  nsRefPtr<FetchPut> mFetchPut;
  nsRefPtr<InternalResponse> mInternalResponse;
};


nsresult
FetchPut::Create(Listener* aListener, Manager* aManager, CacheId aCacheId,
                 const nsTArray<CacheRequest>& aRequests,
                 const nsTArray<nsCOMPtr<nsIInputStream>>& aRequestStreams,
                 FetchPut** aFetchPutOut)
{
  MOZ_ASSERT(aRequests.Length() == aRequestStreams.Length());

  
#ifdef DEBUG
  for (uint32_t i = 0; i < aRequests.Length(); ++i) {
    if (aRequests[i].referrer() == EmptyString()) {
      return NS_ERROR_UNEXPECTED;
    }
  }
#endif

  nsRefPtr<FetchPut> ref = new FetchPut(aListener, aManager, aCacheId,
                                        aRequests, aRequestStreams);

  nsresult rv = ref->DispatchToMainThread();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  ref.forget(aFetchPutOut);

  return NS_OK;
}

void
FetchPut::ClearListener()
{
  MOZ_ASSERT(mListener);
  mListener = nullptr;
}

FetchPut::FetchPut(Listener* aListener, Manager* aManager, CacheId aCacheId,
                   const nsTArray<CacheRequest>& aRequests,
                   const nsTArray<nsCOMPtr<nsIInputStream>>& aRequestStreams)
  : mListener(aListener)
  , mManager(aManager)
  , mCacheId(aCacheId)
  , mInitiatingThread(NS_GetCurrentThread())
  , mStateList(aRequests.Length())
  , mPendingCount(0)
{
  MOZ_ASSERT(mListener);
  MOZ_ASSERT(mManager);
  MOZ_ASSERT(aRequests.Length() == aRequestStreams.Length());

  for (uint32_t i = 0; i < aRequests.Length(); ++i) {
    State* s = mStateList.AppendElement();
    s->mCacheRequest = aRequests[i];
    s->mRequestStream = aRequestStreams[i];
  }

  mManager->AddRefCacheId(mCacheId);
}

FetchPut::~FetchPut()
{
  MOZ_ASSERT(mInitiatingThread == NS_GetCurrentThread());
  MOZ_ASSERT(!mListener);
  mManager->RemoveListener(this);
  mManager->ReleaseCacheId(mCacheId);
  mResult.ClearMessage(); 
}

nsresult
FetchPut::DispatchToMainThread()
{
  MOZ_ASSERT(!mRunnable);

  nsCOMPtr<nsIRunnable> runnable = new Runnable(this);

  nsresult rv = NS_DispatchToMainThread(runnable, nsIThread::DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(!mRunnable);
  mRunnable = runnable.forget();

  return NS_OK;
}

void
FetchPut::DispatchToInitiatingThread()
{
  MOZ_ASSERT(mRunnable);

  nsresult rv = mInitiatingThread->Dispatch(mRunnable,
                                            nsIThread::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    MOZ_CRASH("Failed to dispatch to worker thread after fetch completion.");
  }

  mRunnable = nullptr;
}

void
FetchPut::DoFetchOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<ManagerId> managerId = mManager->GetManagerId();
  nsCOMPtr<nsIPrincipal> principal = managerId->Principal();
  mPendingCount = mStateList.Length();

  nsCOMPtr<nsILoadGroup> loadGroup;
  nsresult rv = NS_NewLoadGroup(getter_AddRefs(loadGroup), principal);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    MaybeSetError(ErrorResult(rv));
    MaybeCompleteOnMainThread();
    return;
  }

  for (uint32_t i = 0; i < mStateList.Length(); ++i) {
    nsRefPtr<InternalRequest> internalRequest =
      ToInternalRequest(mStateList[i].mCacheRequest);

    
    
    if (mStateList[i].mRequestStream) {
      internalRequest->SetBody(mStateList[i].mRequestStream);
      nsRefPtr<InternalRequest> clone = internalRequest->Clone();

      
      
      internalRequest->GetBody(getter_AddRefs(mStateList[i].mRequestStream));

      internalRequest = clone;
    }

    nsRefPtr<FetchDriver> fetchDriver = new FetchDriver(internalRequest,
                                                        principal,
                                                        loadGroup);

    mStateList[i].mFetchObserver = new FetchObserver(this);
    rv = fetchDriver->Fetch(mStateList[i].mFetchObserver);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      MaybeSetError(ErrorResult(rv));
      mStateList[i].mFetchObserver = nullptr;
      mPendingCount -= 1;
      continue;
    }
  }

  
  MaybeCompleteOnMainThread();
}

void
FetchPut::FetchComplete(FetchObserver* aObserver,
                        InternalResponse* aInternalResponse)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aInternalResponse->IsError() && !mResult.Failed()) {
    MaybeSetError(ErrorResult(NS_ERROR_FAILURE));
  }

  for (uint32_t i = 0; i < mStateList.Length(); ++i) {
    if (mStateList[i].mFetchObserver == aObserver) {
      ErrorResult rv;
      ToCacheResponseWithoutBody(mStateList[i].mCacheResponse,
                                  *aInternalResponse, rv);
      if (rv.Failed()) {
        MaybeSetError(Move(rv));
      } else {
        aInternalResponse->GetBody(getter_AddRefs(mStateList[i].mResponseStream));
      }
      mStateList[i].mFetchObserver = nullptr;
      MOZ_ASSERT(mPendingCount > 0);
      mPendingCount -= 1;
      MaybeCompleteOnMainThread();
      return;
    }
  }

  MOZ_ASSERT_UNREACHABLE("Should never get called by unknown fetch observer.");
}

void
FetchPut::MaybeCompleteOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mPendingCount > 0) {
    return;
  }

  DispatchToInitiatingThread();
}

void
FetchPut::DoPutOnWorkerThread()
{
  MOZ_ASSERT(mInitiatingThread == NS_GetCurrentThread());

  if (mResult.Failed()) {
    MaybeNotifyListener();
    return;
  }

  
  nsAutoTArray<CacheRequestResponse, 16> putList;
  nsAutoTArray<nsCOMPtr<nsIInputStream>, 16> requestStreamList;
  nsAutoTArray<nsCOMPtr<nsIInputStream>, 16> responseStreamList;

  putList.SetCapacity(mStateList.Length());
  requestStreamList.SetCapacity(mStateList.Length());
  responseStreamList.SetCapacity(mStateList.Length());

  for (uint32_t i = 0; i < mStateList.Length(); ++i) {
    
    
    if (MatchInPutList(mStateList[i].mCacheRequest, putList)) {
      MaybeSetError(ErrorResult(NS_ERROR_DOM_INVALID_STATE_ERR));
      MaybeNotifyListener();
      return;
    }

    CacheRequestResponse* entry = putList.AppendElement();
    entry->request() = mStateList[i].mCacheRequest;
    entry->response() = mStateList[i].mCacheResponse;
    requestStreamList.AppendElement(mStateList[i].mRequestStream.forget());
    responseStreamList.AppendElement(mStateList[i].mResponseStream.forget());
  }
  mStateList.Clear();

  mManager->ExecutePutAll(this, mCacheId, putList, requestStreamList,
                          responseStreamList);
}


bool
FetchPut::MatchInPutList(const CacheRequest& aRequest,
                         const nsTArray<CacheRequestResponse>& aPutList)
{
  
  
  
  
  

  if (!aRequest.method().LowerCaseEqualsLiteral("get") &&
      !aRequest.method().LowerCaseEqualsLiteral("head")) {
    return false;
  }

  nsRefPtr<InternalHeaders> requestHeaders =
    ToInternalHeaders(aRequest.headers());

  for (uint32_t i = 0; i < aPutList.Length(); ++i) {
    const CacheRequest& cachedRequest = aPutList[i].request();
    const CacheResponse& cachedResponse = aPutList[i].response();

    
    if (aRequest.url() != cachedRequest.url()) {
      continue;
    }

    nsRefPtr<InternalHeaders> cachedRequestHeaders =
      ToInternalHeaders(cachedRequest.headers());

    nsRefPtr<InternalHeaders> cachedResponseHeaders =
      ToInternalHeaders(cachedResponse.headers());

    nsAutoTArray<nsCString, 16> varyHeaders;
    ErrorResult rv;
    cachedResponseHeaders->GetAll(NS_LITERAL_CSTRING("vary"), varyHeaders, rv);
    MOZ_ALWAYS_TRUE(!rv.Failed());

    
    bool varyHeadersMatch = true;

    for (uint32_t j = 0; j < varyHeaders.Length(); ++j) {
      
      nsAutoCString varyValue(varyHeaders[j]);
      char* rawBuffer = varyValue.BeginWriting();
      char* token = nsCRT::strtok(rawBuffer, NS_HTTP_HEADER_SEPS, &rawBuffer);
      bool bailOut = false;
      for (; token;
           token = nsCRT::strtok(rawBuffer, NS_HTTP_HEADER_SEPS, &rawBuffer)) {
        nsDependentCString header(token);
        MOZ_ASSERT(!header.EqualsLiteral("*"),
                   "We should have already caught this in "
                   "TypeUtils::ToPCacheResponseWithoutBody()");

        ErrorResult headerRv;
        nsAutoCString value;
        requestHeaders->Get(header, value, headerRv);
        if (NS_WARN_IF(headerRv.Failed())) {
          headerRv.ClearMessage();
          MOZ_ASSERT(value.IsEmpty());
        }

        nsAutoCString cachedValue;
        cachedRequestHeaders->Get(header, value, headerRv);
        if (NS_WARN_IF(headerRv.Failed())) {
          headerRv.ClearMessage();
          MOZ_ASSERT(cachedValue.IsEmpty());
        }

        if (value != cachedValue) {
          varyHeadersMatch = false;
          bailOut = true;
          break;
        }
      }

      if (bailOut) {
        break;
      }
    }

    
    if (varyHeadersMatch) {
      return true;
    }
  }

  return false;
}

void
FetchPut::OnOpComplete(ErrorResult&& aRv, const CacheOpResult& aResult,
                       CacheId aOpenedCacheId,
                       const nsTArray<SavedResponse>& aSavedResponseList,
                       const nsTArray<SavedRequest>& aSavedRequestList,
                       StreamList* aStreamList)
{
  MOZ_ASSERT(mInitiatingThread == NS_GetCurrentThread());
  MOZ_ASSERT(aResult.type() == CacheOpResult::TCachePutAllResult);
  MaybeSetError(Move(aRv));
  MaybeNotifyListener();
}

void
FetchPut::MaybeSetError(ErrorResult&& aRv)
{
  if (mResult.Failed() || !aRv.Failed()) {
    return;
  }
  mResult = Move(aRv);
}

void
FetchPut::MaybeNotifyListener()
{
  MOZ_ASSERT(mInitiatingThread == NS_GetCurrentThread());
  if (!mListener) {
    return;
  }
  
  
  
  nsRefPtr<FetchPut> kungFuDeathGrip(this);
  mListener->OnFetchPut(this, Move(mResult));
}

nsIGlobalObject*
FetchPut::GetGlobalObject() const
{
  MOZ_CRASH("No global object in parent-size FetchPut operation!");
}

#ifdef DEBUG
void
FetchPut::AssertOwningThread() const
{
  MOZ_ASSERT(mInitiatingThread == NS_GetCurrentThread());
}
#endif

CachePushStreamChild*
FetchPut::CreatePushStream(nsIAsyncInputStream* aStream)
{
  MOZ_CRASH("FetchPut should never create a push stream!");
}

} 
} 
} 
