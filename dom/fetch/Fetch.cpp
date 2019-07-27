




#include "Fetch.h"

#include "nsIDocument.h"
#include "nsIGlobalObject.h"
#include "nsIStreamLoader.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"

#include "nsDOMString.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsStringStream.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/EncodingUtils.h"
#include "mozilla/dom/FetchDriver.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/Headers.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/Request.h"
#include "mozilla/dom/Response.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/URLSearchParams.h"

#include "InternalResponse.h"

#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"
#include "Workers.h"

namespace mozilla {
namespace dom {

using namespace workers;

class WorkerFetchResolver MOZ_FINAL : public FetchDriverObserver,
                                      public WorkerFeature
{
  friend class MainThreadFetchRunnable;
  friend class WorkerFetchResponseEndRunnable;
  friend class WorkerFetchResponseRunnable;

  workers::WorkerPrivate* mWorkerPrivate;

  Mutex mCleanUpLock;
  bool mCleanedUp;
  
  nsRefPtr<Promise> mFetchPromise;
  nsRefPtr<Response> mResponse;
public:

  WorkerFetchResolver(workers::WorkerPrivate* aWorkerPrivate, Promise* aPromise)
    : mWorkerPrivate(aWorkerPrivate)
    , mCleanUpLock("WorkerFetchResolver")
    , mCleanedUp(false)
    , mFetchPromise(aPromise)
  {
  }

  void
  OnResponseAvailable(InternalResponse* aResponse) MOZ_OVERRIDE;

  void
  OnResponseEnd() MOZ_OVERRIDE;

  bool
  Notify(JSContext* aCx, Status aStatus) MOZ_OVERRIDE
  {
    if (aStatus > Running) {
      CleanUp(aCx);
    }
    return true;
  }

  void
  CleanUp(JSContext* aCx)
  {
    MutexAutoLock lock(mCleanUpLock);

    if (mCleanedUp) {
      return;
    }

    MOZ_ASSERT(mWorkerPrivate);
    mWorkerPrivate->AssertIsOnWorkerThread();
    MOZ_ASSERT(mWorkerPrivate->GetJSContext() == aCx);

    mWorkerPrivate->RemoveFeature(aCx, this);
    CleanUpUnchecked();
  }

  void
  CleanUpUnchecked()
  {
    mResponse = nullptr;
    if (mFetchPromise) {
      mFetchPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      mFetchPromise = nullptr;
    }
    mCleanedUp = true;
  }

  workers::WorkerPrivate*
  GetWorkerPrivate() const
  {
    
    
    MOZ_ASSERT(!mCleanedUp);
    return mWorkerPrivate;
  }

private:
  ~WorkerFetchResolver()
  {
    MOZ_ASSERT(mCleanedUp);
    MOZ_ASSERT(!mFetchPromise);
  }
};

class MainThreadFetchResolver MOZ_FINAL : public FetchDriverObserver
{
  nsRefPtr<Promise> mPromise;
  nsRefPtr<Response> mResponse;

  NS_DECL_OWNINGTHREAD
public:
  explicit MainThreadFetchResolver(Promise* aPromise);

  void
  OnResponseAvailable(InternalResponse* aResponse) MOZ_OVERRIDE;

  void
  OnResponseEnd() MOZ_OVERRIDE;

private:
  ~MainThreadFetchResolver();
};

class MainThreadFetchRunnable : public nsRunnable
{
  nsRefPtr<WorkerFetchResolver> mResolver;
  nsRefPtr<InternalRequest> mRequest;

public:
  MainThreadFetchRunnable(WorkerPrivate* aWorkerPrivate,
                          Promise* aPromise,
                          InternalRequest* aRequest)
    : mResolver(new WorkerFetchResolver(aWorkerPrivate, aPromise))
    , mRequest(aRequest)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
    if (!aWorkerPrivate->AddFeature(aWorkerPrivate->GetJSContext(), mResolver)) {
      NS_WARNING("Could not add WorkerFetchResolver feature to worker");
      mResolver->CleanUpUnchecked();
      mResolver = nullptr;
    }
  }

  NS_IMETHODIMP
  Run()
  {
    AssertIsOnMainThread();
    
    if (!mResolver) {
      return NS_OK;
    }

    nsCOMPtr<nsIPrincipal> principal = mResolver->GetWorkerPrivate()->GetPrincipal();
    nsCOMPtr<nsILoadGroup> loadGroup = mResolver->GetWorkerPrivate()->GetLoadGroup();
    nsRefPtr<FetchDriver> fetch = new FetchDriver(mRequest, principal, loadGroup);
    nsresult rv = fetch->Fetch(mResolver);
    
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    return NS_OK;
  }
};

already_AddRefed<Promise>
FetchRequest(nsIGlobalObject* aGlobal, const RequestOrUSVString& aInput,
             const RequestInit& aInit, ErrorResult& aRv)
{
  nsRefPtr<Promise> p = Promise::Create(aGlobal, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  AutoJSAPI jsapi;
  jsapi.Init(aGlobal);
  JSContext* cx = jsapi.cx();

  JS::Rooted<JSObject*> jsGlobal(cx, aGlobal->GetGlobalJSObject());
  GlobalObject global(cx, jsGlobal);

  nsRefPtr<Request> request = Request::Constructor(global, aInput, aInit, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsRefPtr<InternalRequest> r = request->GetInternalRequest();
  if (!r->ReferrerIsNone()) {
    nsAutoCString ref;
    aRv = GetRequestReferrer(aGlobal, r, ref);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }
    r->SetReferrer(ref);
  }

  if (NS_IsMainThread()) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal);
    if (!window) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
    if (!doc) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    nsRefPtr<MainThreadFetchResolver> resolver = new MainThreadFetchResolver(p);
    nsCOMPtr<nsILoadGroup> loadGroup = doc->GetDocumentLoadGroup();
    nsRefPtr<FetchDriver> fetch =
      new FetchDriver(r, doc->NodePrincipal(), loadGroup);
    aRv = fetch->Fetch(resolver);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }
  } else {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    nsRefPtr<MainThreadFetchRunnable> run = new MainThreadFetchRunnable(worker, p, r);
    if (NS_FAILED(NS_DispatchToMainThread(run))) {
      NS_WARNING("MainThreadFetchRunnable dispatch failed!");
    }
  }

  return p.forget();
}

MainThreadFetchResolver::MainThreadFetchResolver(Promise* aPromise)
  : mPromise(aPromise)
{
}

void
MainThreadFetchResolver::OnResponseAvailable(InternalResponse* aResponse)
{
  NS_ASSERT_OWNINGTHREAD(MainThreadFetchResolver);
  AssertIsOnMainThread();

  nsCOMPtr<nsIGlobalObject> go = mPromise->GetParentObject();
  mResponse = new Response(go, aResponse);
  mPromise->MaybeResolve(mResponse);
}

void
MainThreadFetchResolver::OnResponseEnd()
{
  NS_ASSERT_OWNINGTHREAD(MainThreadFetchResolver);
  AssertIsOnMainThread();
  MOZ_ASSERT(mResponse);
}

MainThreadFetchResolver::~MainThreadFetchResolver()
{
  NS_ASSERT_OWNINGTHREAD(MainThreadFetchResolver);
}

class WorkerFetchResponseRunnable MOZ_FINAL : public WorkerRunnable
{
  nsRefPtr<WorkerFetchResolver> mResolver;
  
  nsRefPtr<InternalResponse> mInternalResponse;
public:
  WorkerFetchResponseRunnable(WorkerFetchResolver* aResolver, InternalResponse* aResponse)
    : WorkerRunnable(aResolver->GetWorkerPrivate(), WorkerThreadModifyBusyCount)
    , mResolver(aResolver)
    , mInternalResponse(aResponse)
  {
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
    MOZ_ASSERT(aWorkerPrivate == mResolver->GetWorkerPrivate());

    nsRefPtr<nsIGlobalObject> global = aWorkerPrivate->GlobalScope();
    mResolver->mResponse = new Response(global, mInternalResponse);

    nsRefPtr<Promise> promise = mResolver->mFetchPromise.forget();
    promise->MaybeResolve(mResolver->mResponse);

    return true;
  }
};

class WorkerFetchResponseEndRunnable MOZ_FINAL : public WorkerRunnable
{
  nsRefPtr<WorkerFetchResolver> mResolver;
public:
  explicit WorkerFetchResponseEndRunnable(WorkerFetchResolver* aResolver)
    : WorkerRunnable(aResolver->GetWorkerPrivate(), WorkerThreadModifyBusyCount)
    , mResolver(aResolver)
  {
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
    MOZ_ASSERT(aWorkerPrivate == mResolver->GetWorkerPrivate());
    MOZ_ASSERT(mResolver->mResponse);

    mResolver->CleanUp(aCx);
    return true;
  }
};

void
WorkerFetchResolver::OnResponseAvailable(InternalResponse* aResponse)
{
  AssertIsOnMainThread();

  MutexAutoLock lock(mCleanUpLock);
  if (mCleanedUp) {
    return;
  }

  nsRefPtr<WorkerFetchResponseRunnable> r =
    new WorkerFetchResponseRunnable(this, aResponse);

  AutoSafeJSContext cx;
  if (!r->Dispatch(cx)) {
    NS_WARNING("Could not dispatch fetch resolve");
  }
}

void
WorkerFetchResolver::OnResponseEnd()
{
  AssertIsOnMainThread();
  MutexAutoLock lock(mCleanUpLock);
  if (mCleanedUp) {
    return;
  }

  nsRefPtr<WorkerFetchResponseEndRunnable> r =
    new WorkerFetchResponseEndRunnable(this);

  AutoSafeJSContext cx;
  if (!r->Dispatch(cx)) {
    NS_WARNING("Could not dispatch fetch resolve end");
  }
}





nsresult
GetRequestReferrer(nsIGlobalObject* aGlobal, const InternalRequest* aRequest, nsCString& aReferrer)
{
  if (aRequest->ReferrerIsURL()) {
    aReferrer = aRequest->ReferrerAsURL();
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal);
  if (window) {
    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
    if (doc) {
      nsCOMPtr<nsIURI> docURI = doc->GetDocumentURI();
      nsAutoCString origin;
      nsresult rv = nsContentUtils::GetASCIIOrigin(docURI, origin);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      nsAutoString referrer;
      doc->GetReferrer(referrer);
      aReferrer = NS_ConvertUTF16toUTF8(referrer);
    }
  } else {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();
    aReferrer = worker->GetLocationInfo().mHref;
    
    
  }

  return NS_OK;
}

namespace {
nsresult
ExtractFromArrayBuffer(const ArrayBuffer& aBuffer,
                       nsIInputStream** aStream)
{
  aBuffer.ComputeLengthAndData();
  
  return NS_NewByteInputStream(aStream,
                               reinterpret_cast<char*>(aBuffer.Data()),
                               aBuffer.Length(), NS_ASSIGNMENT_COPY);
}

nsresult
ExtractFromArrayBufferView(const ArrayBufferView& aBuffer,
                           nsIInputStream** aStream)
{
  aBuffer.ComputeLengthAndData();
  
  return NS_NewByteInputStream(aStream,
                               reinterpret_cast<char*>(aBuffer.Data()),
                               aBuffer.Length(), NS_ASSIGNMENT_COPY);
}

nsresult
ExtractFromBlob(const File& aFile, nsIInputStream** aStream,
                nsCString& aContentType)
{
  nsRefPtr<FileImpl> impl = aFile.Impl();
  nsresult rv = impl->GetInternalStream(aStream);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoString type;
  impl->GetType(type);
  aContentType = NS_ConvertUTF16toUTF8(type);
  return NS_OK;
}

nsresult
ExtractFromUSVString(const nsString& aStr,
                     nsIInputStream** aStream,
                     nsCString& aContentType)
{
  nsCOMPtr<nsIUnicodeEncoder> encoder = EncodingUtils::EncoderForEncoding("UTF-8");
  if (!encoder) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  int32_t destBufferLen;
  nsresult rv = encoder->GetMaxLength(aStr.get(), aStr.Length(), &destBufferLen);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCString encoded;
  if (!encoded.SetCapacity(destBufferLen, fallible_t())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  char* destBuffer = encoded.BeginWriting();
  int32_t srcLen = (int32_t) aStr.Length();
  int32_t outLen = destBufferLen;
  rv = encoder->Convert(aStr.get(), &srcLen, destBuffer, &outLen);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(outLen <= destBufferLen);
  encoded.SetLength(outLen);

  aContentType = NS_LITERAL_CSTRING("text/plain;charset=UTF-8");

  return NS_NewCStringInputStream(aStream, encoded);
}

nsresult
ExtractFromURLSearchParams(const URLSearchParams& aParams,
                           nsIInputStream** aStream,
                           nsCString& aContentType)
{
  nsAutoString serialized;
  aParams.Stringify(serialized);
  aContentType = NS_LITERAL_CSTRING("application/x-www-form-urlencoded;charset=UTF-8");
  return NS_NewStringInputStream(aStream, serialized);
}
} 

nsresult
ExtractByteStreamFromBody(const OwningArrayBufferOrArrayBufferViewOrBlobOrUSVStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType)
{
  MOZ_ASSERT(aStream);

  if (aBodyInit.IsArrayBuffer()) {
    const ArrayBuffer& buf = aBodyInit.GetAsArrayBuffer();
    return ExtractFromArrayBuffer(buf, aStream);
  } else if (aBodyInit.IsArrayBufferView()) {
    const ArrayBufferView& buf = aBodyInit.GetAsArrayBufferView();
    return ExtractFromArrayBufferView(buf, aStream);
  } else if (aBodyInit.IsBlob()) {
    const File& blob = aBodyInit.GetAsBlob();
    return ExtractFromBlob(blob, aStream, aContentType);
  } else if (aBodyInit.IsUSVString()) {
    nsAutoString str;
    str.Assign(aBodyInit.GetAsUSVString());
    return ExtractFromUSVString(str, aStream, aContentType);
  } else if (aBodyInit.IsURLSearchParams()) {
    URLSearchParams& params = aBodyInit.GetAsURLSearchParams();
    return ExtractFromURLSearchParams(params, aStream, aContentType);
  }

  NS_NOTREACHED("Should never reach here");
  return NS_ERROR_FAILURE;
}

nsresult
ExtractByteStreamFromBody(const ArrayBufferOrArrayBufferViewOrBlobOrUSVStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType)
{
  MOZ_ASSERT(aStream);

  if (aBodyInit.IsArrayBuffer()) {
    const ArrayBuffer& buf = aBodyInit.GetAsArrayBuffer();
    return ExtractFromArrayBuffer(buf, aStream);
  } else if (aBodyInit.IsArrayBufferView()) {
    const ArrayBufferView& buf = aBodyInit.GetAsArrayBufferView();
    return ExtractFromArrayBufferView(buf, aStream);
  } else if (aBodyInit.IsBlob()) {
    const File& blob = aBodyInit.GetAsBlob();
    return ExtractFromBlob(blob, aStream, aContentType);
  } else if (aBodyInit.IsUSVString()) {
    nsAutoString str;
    str.Assign(aBodyInit.GetAsUSVString());
    return ExtractFromUSVString(str, aStream, aContentType);
  } else if (aBodyInit.IsURLSearchParams()) {
    URLSearchParams& params = aBodyInit.GetAsURLSearchParams();
    return ExtractFromURLSearchParams(params, aStream, aContentType);
  }

  NS_NOTREACHED("Should never reach here");
  return NS_ERROR_FAILURE;
}

namespace {
class StreamDecoder MOZ_FINAL
{
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  nsString mDecoded;

public:
  StreamDecoder()
    : mDecoder(EncodingUtils::DecoderForEncoding("UTF-8"))
  {
    MOZ_ASSERT(mDecoder);
  }

  nsresult
  AppendText(const char* aSrcBuffer, uint32_t aSrcBufferLen)
  {
    int32_t destBufferLen;
    nsresult rv =
      mDecoder->GetMaxLength(aSrcBuffer, aSrcBufferLen, &destBufferLen);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!mDecoded.SetCapacity(mDecoded.Length() + destBufferLen, fallible_t())) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    char16_t* destBuffer = mDecoded.BeginWriting() + mDecoded.Length();
    int32_t totalChars = mDecoded.Length();

    int32_t srcLen = (int32_t) aSrcBufferLen;
    int32_t outLen = destBufferLen;
    rv = mDecoder->Convert(aSrcBuffer, &srcLen, destBuffer, &outLen);
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    totalChars += outLen;
    mDecoded.SetLength(totalChars);

    return NS_OK;
  }

  nsString&
  GetText()
  {
    return mDecoded;
  }
};




template <class Derived>
class ContinueConsumeBodyRunnable MOZ_FINAL : public WorkerRunnable
{
  
  
  FetchBody<Derived>* mFetchBody;
  nsresult mStatus;
  uint32_t mLength;
  uint8_t* mResult;

public:
  ContinueConsumeBodyRunnable(FetchBody<Derived>* aFetchBody, nsresult aStatus,
                              uint32_t aLength, uint8_t* aResult)
    : WorkerRunnable(aFetchBody->mWorkerPrivate, WorkerThreadModifyBusyCount)
    , mFetchBody(aFetchBody)
    , mStatus(aStatus)
    , mLength(aLength)
    , mResult(aResult)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    mFetchBody->ContinueConsumeBody(mStatus, mLength, mResult);
    return true;
  }
};



class MOZ_STACK_CLASS AutoFreeBuffer MOZ_FINAL {
  uint8_t* mBuffer;

public:
  explicit AutoFreeBuffer(uint8_t* aBuffer)
    : mBuffer(aBuffer)
  {}

  ~AutoFreeBuffer()
  {
    moz_free(mBuffer);
  }

  void
  Reset()
  {
    mBuffer= nullptr;
  }
};

template <class Derived>
class FailConsumeBodyWorkerRunnable : public MainThreadWorkerControlRunnable
{
  FetchBody<Derived>* mBody;
public:
  explicit FailConsumeBodyWorkerRunnable(FetchBody<Derived>* aBody)
    : MainThreadWorkerControlRunnable(aBody->mWorkerPrivate)
    , mBody(aBody)
  {
    AssertIsOnMainThread();
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) MOZ_OVERRIDE
  {
    mBody->ContinueConsumeBody(NS_ERROR_FAILURE, 0, nullptr);
    return true;
  }
};





template <class Derived>
class MOZ_STACK_CLASS AutoFailConsumeBody MOZ_FINAL
{
  FetchBody<Derived>* mBody;
public:
  explicit AutoFailConsumeBody(FetchBody<Derived>* aBody)
    : mBody(aBody)
  { }

  ~AutoFailConsumeBody()
  {
    AssertIsOnMainThread();
    if (mBody) {
      if (mBody->mWorkerPrivate) {
        nsRefPtr<FailConsumeBodyWorkerRunnable<Derived>> r =
          new FailConsumeBodyWorkerRunnable<Derived>(mBody);
        AutoSafeJSContext cx;
        if (!r->Dispatch(cx)) {
          MOZ_CRASH("We are going to leak");
        }
      } else {
        mBody->ContinueConsumeBody(NS_ERROR_FAILURE, 0, nullptr);
      }
    }
  }

  void
  DontFail()
  {
    mBody = nullptr;
  }
};

template <class Derived>
class ConsumeBodyDoneObserver : public nsIStreamLoaderObserver
{
  FetchBody<Derived>* mFetchBody;

public:
  NS_DECL_THREADSAFE_ISUPPORTS

  explicit ConsumeBodyDoneObserver(FetchBody<Derived>* aFetchBody)
    : mFetchBody(aFetchBody)
  { }

  NS_IMETHOD
  OnStreamComplete(nsIStreamLoader* aLoader,
                   nsISupports* aCtxt,
                   nsresult aStatus,
                   uint32_t aResultLength,
                   const uint8_t* aResult)
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    
    if (aStatus == NS_BINDING_ABORTED) {
      return NS_OK;
    }

    uint8_t* nonconstResult = const_cast<uint8_t*>(aResult);
    if (mFetchBody->mWorkerPrivate) {
      
      AutoFailConsumeBody<Derived> autoFail(mFetchBody);
      nsRefPtr<ContinueConsumeBodyRunnable<Derived>> r =
        new ContinueConsumeBodyRunnable<Derived>(mFetchBody,
                                        aStatus,
                                        aResultLength,
                                        nonconstResult);
      AutoSafeJSContext cx;
      if (r->Dispatch(cx)) {
        autoFail.DontFail();
      } else {
        NS_WARNING("Could not dispatch ConsumeBodyRunnable");
        
        return NS_ERROR_FAILURE;
      }
    } else {
      mFetchBody->ContinueConsumeBody(aStatus, aResultLength, nonconstResult);
    }

    
    return NS_SUCCESS_ADOPTED_DATA;
  }

private:
  virtual ~ConsumeBodyDoneObserver()
  { }
};

template <class Derived>
NS_IMPL_ADDREF(ConsumeBodyDoneObserver<Derived>)
template <class Derived>
NS_IMPL_RELEASE(ConsumeBodyDoneObserver<Derived>)
template <class Derived>
NS_INTERFACE_MAP_BEGIN(ConsumeBodyDoneObserver<Derived>)
  NS_INTERFACE_MAP_ENTRY(nsIStreamLoaderObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStreamLoaderObserver)
NS_INTERFACE_MAP_END

template <class Derived>
class BeginConsumeBodyRunnable MOZ_FINAL : public nsRunnable
{
  FetchBody<Derived>* mFetchBody;
public:
  explicit BeginConsumeBodyRunnable(FetchBody<Derived>* aBody)
    : mFetchBody(aBody)
  { }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    mFetchBody->BeginConsumeBodyMainThread();
    return NS_OK;
  }
};

template <class Derived>
class CancelPumpRunnable MOZ_FINAL : public WorkerMainThreadRunnable
{
  FetchBody<Derived>* mBody;
public:
  explicit CancelPumpRunnable(FetchBody<Derived>* aBody)
    : WorkerMainThreadRunnable(aBody->mWorkerPrivate)
    , mBody(aBody)
  { }

  bool
  MainThreadRun() MOZ_OVERRIDE
  {
    mBody->CancelPump();
    return true;
  }
};
} 

template <class Derived>
class FetchBodyFeature MOZ_FINAL : public workers::WorkerFeature
{
  
  
  FetchBody<Derived>* mBody;

public:
  explicit FetchBodyFeature(FetchBody<Derived>* aBody)
    : mBody(aBody)
  { }

  ~FetchBodyFeature()
  { }

  bool Notify(JSContext* aCx, workers::Status aStatus) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aStatus > workers::Running);
    mBody->ContinueConsumeBody(NS_BINDING_ABORTED, 0, nullptr);
    return true;
  }
};

template <class Derived>
FetchBody<Derived>::FetchBody()
  : mFeature(nullptr)
  , mBodyUsed(false)
  , mReadDone(false)
{
  if (!NS_IsMainThread()) {
    mWorkerPrivate = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(mWorkerPrivate);
  } else {
    mWorkerPrivate = nullptr;
  }
}

template
FetchBody<Request>::FetchBody();

template
FetchBody<Response>::FetchBody();





template <class Derived>
bool
FetchBody<Derived>::AddRefObject()
{
  AssertIsOnTargetThread();
  DerivedClass()->AddRef();

  if (mWorkerPrivate && !mFeature) {
    if (!RegisterFeature()) {
      ReleaseObject();
      return false;
    }
  }
  return true;
}

template <class Derived>
void
FetchBody<Derived>::ReleaseObject()
{
  AssertIsOnTargetThread();

  if (mWorkerPrivate && mFeature) {
    UnregisterFeature();
  }

  DerivedClass()->Release();
}

template <class Derived>
bool
FetchBody<Derived>::RegisterFeature()
{
  MOZ_ASSERT(mWorkerPrivate);
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(!mFeature);
  mFeature = new FetchBodyFeature<Derived>(this);

  if (!mWorkerPrivate->AddFeature(mWorkerPrivate->GetJSContext(), mFeature)) {
    NS_WARNING("Failed to add feature");
    mFeature = nullptr;
    return false;
  }

  return true;
}

template <class Derived>
void
FetchBody<Derived>::UnregisterFeature()
{
  MOZ_ASSERT(mWorkerPrivate);
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(mFeature);

  mWorkerPrivate->RemoveFeature(mWorkerPrivate->GetJSContext(), mFeature);
  mFeature = nullptr;
}

template <class Derived>
void
FetchBody<Derived>::CancelPump()
{
  AssertIsOnMainThread();
  MOZ_ASSERT(mConsumeBodyPump);
  mConsumeBodyPump->Cancel(NS_BINDING_ABORTED);
}



template <class Derived>
nsresult
FetchBody<Derived>::BeginConsumeBody()
{
  AssertIsOnTargetThread();
  MOZ_ASSERT(!mFeature);
  MOZ_ASSERT(mConsumePromise);

  
  
  if (!AddRefObject()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIRunnable> r = new BeginConsumeBodyRunnable<Derived>(this);
  nsresult rv = NS_DispatchToMainThread(r);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    ReleaseObject();
    return rv;
  }
  return NS_OK;
}






template <class Derived>
void
FetchBody<Derived>::BeginConsumeBodyMainThread()
{
  AssertIsOnMainThread();
  AutoFailConsumeBody<Derived> autoReject(DerivedClass());
  nsresult rv;
  nsCOMPtr<nsIInputStream> stream;
  DerivedClass()->GetBody(getter_AddRefs(stream));
  if (!stream) {
    rv = NS_NewCStringInputStream(getter_AddRefs(stream), EmptyCString());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }
  }

  nsCOMPtr<nsIInputStreamPump> pump;
  rv = NS_NewInputStreamPump(getter_AddRefs(pump),
                             stream);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  nsRefPtr<ConsumeBodyDoneObserver<Derived>> p = new ConsumeBodyDoneObserver<Derived>(this);
  nsCOMPtr<nsIStreamLoader> loader;
  rv = NS_NewStreamLoader(getter_AddRefs(loader), p);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  rv = pump->AsyncRead(loader, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  
  
  mConsumeBodyPump = new nsMainThreadPtrHolder<nsIInputStreamPump>(pump);
  
  autoReject.DontFail();

  
  nsCOMPtr<nsIThreadRetargetableRequest> rr = do_QueryInterface(pump);
  if (rr) {
    nsCOMPtr<nsIEventTarget> sts = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
    rv = rr->RetargetDeliveryTo(sts);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      NS_WARNING("Retargeting failed");
    }
  }
}

template <class Derived>
void
FetchBody<Derived>::ContinueConsumeBody(nsresult aStatus, uint32_t aResultLength, uint8_t* aResult)
{
  AssertIsOnTargetThread();
  
  
  MOZ_ASSERT(mBodyUsed);
  MOZ_ASSERT(!mReadDone);
  MOZ_ASSERT_IF(mWorkerPrivate, mFeature);
  mReadDone = true;

  AutoFreeBuffer autoFree(aResult);

  MOZ_ASSERT(mConsumePromise);
  nsRefPtr<Promise> localPromise = mConsumePromise.forget();

  nsRefPtr<Derived> kungfuDeathGrip = DerivedClass();
  ReleaseObject();

  if (NS_WARN_IF(NS_FAILED(aStatus))) {
    localPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);

    
    
    
    
    
    
    if (aStatus == NS_BINDING_ABORTED && !!mConsumeBodyPump) {
      if (NS_IsMainThread()) {
        CancelPump();
      } else {
        MOZ_ASSERT(mWorkerPrivate);
        
        
        
        
        nsRefPtr<CancelPumpRunnable<Derived>> r =
          new CancelPumpRunnable<Derived>(this);
        if (!r->Dispatch(mWorkerPrivate->GetJSContext())) {
          NS_WARNING("Could not dispatch CancelPumpRunnable. Nothing we can do here");
        }
      }
    }
  }

  
  
  mConsumeBodyPump = nullptr;

  
  if (NS_FAILED(aStatus)) {
    return;
  }

  
  MOZ_ASSERT(aResult);

  AutoJSAPI api;
  api.Init(DerivedClass()->GetParentObject());
  JSContext* cx = api.cx();

  switch (mConsumeType) {
    case CONSUME_ARRAYBUFFER: {
      JS::Rooted<JSObject*> arrayBuffer(cx);
      arrayBuffer = JS_NewArrayBufferWithContents(cx, aResultLength, reinterpret_cast<void *>(aResult));
      if (!arrayBuffer) {
        JS_ClearPendingException(cx);
        localPromise->MaybeReject(NS_ERROR_DOM_UNKNOWN_ERR);
        NS_WARNING("OUT OF MEMORY");
        return;
      }

      JS::Rooted<JS::Value> val(cx);
      val.setObjectOrNull(arrayBuffer);
      localPromise->MaybeResolve(cx, val);
      
      autoFree.Reset();
      return;
    }
    case CONSUME_BLOB: {
      nsRefPtr<File> blob =
        File::CreateMemoryFile(DerivedClass()->GetParentObject(),
                               reinterpret_cast<void *>(aResult), aResultLength, NS_ConvertUTF8toUTF16(mMimeType));

      if (!blob) {
        localPromise->MaybeReject(NS_ERROR_DOM_UNKNOWN_ERR);
        return;
      }

      localPromise->MaybeResolve(blob);
      
      autoFree.Reset();
      return;
    }
    case CONSUME_TEXT:
      
    case CONSUME_JSON: {
      StreamDecoder decoder;
      decoder.AppendText(reinterpret_cast<char*>(aResult), aResultLength);

      nsString& decoded = decoder.GetText();
      if (mConsumeType == CONSUME_TEXT) {
        localPromise->MaybeResolve(decoded);
        return;
      }

      JS::Rooted<JS::Value> json(cx);
      if (!JS_ParseJSON(cx, decoded.get(), decoded.Length(), &json)) {
        JS::Rooted<JS::Value> exn(cx);
        if (JS_GetPendingException(cx, &exn)) {
          JS_ClearPendingException(cx);
          localPromise->MaybeReject(cx, exn);
        }
        return;
      }

      localPromise->MaybeResolve(cx, json);
      return;
    }
  }

  NS_NOTREACHED("Unexpected consume body type");
}

template <class Derived>
already_AddRefed<Promise>
FetchBody<Derived>::ConsumeBody(ConsumeType aType, ErrorResult& aRv)
{
  mConsumeType = aType;
  if (BodyUsed()) {
    aRv.ThrowTypeError(MSG_REQUEST_BODY_CONSUMED_ERROR);
    return nullptr;
  }

  SetBodyUsed();

  mConsumePromise = Promise::Create(DerivedClass()->GetParentObject(), aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  aRv = BeginConsumeBody();
  if (NS_WARN_IF(aRv.Failed())) {
    mConsumePromise = nullptr;
    return nullptr;
  }

  nsRefPtr<Promise> promise = mConsumePromise;
  return promise.forget();
}

template
already_AddRefed<Promise>
FetchBody<Request>::ConsumeBody(ConsumeType aType, ErrorResult& aRv);

template
already_AddRefed<Promise>
FetchBody<Response>::ConsumeBody(ConsumeType aType, ErrorResult& aRv);

template <class Derived>
void
FetchBody<Derived>::SetMimeType(ErrorResult& aRv)
{
  
  nsTArray<nsCString> contentTypeValues;
  MOZ_ASSERT(DerivedClass()->GetInternalHeaders());
  DerivedClass()->GetInternalHeaders()->GetAll(NS_LITERAL_CSTRING("Content-Type"), contentTypeValues, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return;
  }

  
  
  if (contentTypeValues.Length() == 1) {
    mMimeType = contentTypeValues[0];
    ToLowerCase(mMimeType);
  }
}

template
void
FetchBody<Request>::SetMimeType(ErrorResult& aRv);

template
void
FetchBody<Response>::SetMimeType(ErrorResult& aRv);
} 
} 
