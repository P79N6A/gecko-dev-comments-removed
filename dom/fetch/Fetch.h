




#ifndef mozilla_dom_Fetch_h
#define mozilla_dom_Fetch_h

#include "nsIInputStreamPump.h"
#include "nsIStreamLoader.h"

#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsProxyRelease.h"
#include "nsString.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/RequestBinding.h"
#include "mozilla/dom/workers/bindings/WorkerFeature.h"

class nsIOutputStream;
class nsIGlobalObject;

namespace mozilla {
namespace dom {

class ArrayBufferOrArrayBufferViewOrBlobOrFormDataOrUSVStringOrURLSearchParams;
class InternalRequest;
class OwningArrayBufferOrArrayBufferViewOrBlobOrFormDataOrUSVStringOrURLSearchParams;
class RequestOrUSVString;

namespace workers {
class WorkerPrivate;
} 

already_AddRefed<Promise>
FetchRequest(nsIGlobalObject* aGlobal, const RequestOrUSVString& aInput,
             const RequestInit& aInit, ErrorResult& aRv);

nsresult
UpdateRequestReferrer(nsIGlobalObject* aGlobal, InternalRequest* aRequest);






nsresult
ExtractByteStreamFromBody(const OwningArrayBufferOrArrayBufferViewOrBlobOrFormDataOrUSVStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType);




nsresult
ExtractByteStreamFromBody(const ArrayBufferOrArrayBufferViewOrBlobOrFormDataOrUSVStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType);

template <class Derived> class FetchBodyFeature;


































template <class Derived>
class FetchBody {
public:
  bool
  BodyUsed() const { return mBodyUsed; }

  already_AddRefed<Promise>
  ArrayBuffer(ErrorResult& aRv)
  {
    return ConsumeBody(CONSUME_ARRAYBUFFER, aRv);
  }

  already_AddRefed<Promise>
  Blob(ErrorResult& aRv)
  {
    return ConsumeBody(CONSUME_BLOB, aRv);
  }

  already_AddRefed<Promise>
  FormData(ErrorResult& aRv)
  {
    return ConsumeBody(CONSUME_FORMDATA, aRv);
  }

  already_AddRefed<Promise>
  Json(ErrorResult& aRv)
  {
    return ConsumeBody(CONSUME_JSON, aRv);
  }

  already_AddRefed<Promise>
  Text(ErrorResult& aRv)
  {
    return ConsumeBody(CONSUME_TEXT, aRv);
  }

  
  void
  BeginConsumeBodyMainThread();

  void
  ContinueConsumeBody(nsresult aStatus, uint32_t aLength, uint8_t* aResult);

  void
  CancelPump();

  void
  SetBodyUsed()
  {
    mBodyUsed = true;
  }

  
  workers::WorkerPrivate* mWorkerPrivate;

  
  
  nsAutoPtr<workers::WorkerFeature> mFeature;

protected:
  FetchBody();

  virtual ~FetchBody();

  void
  SetMimeType();
private:
  enum ConsumeType
  {
    CONSUME_ARRAYBUFFER,
    CONSUME_BLOB,
    CONSUME_FORMDATA,
    CONSUME_JSON,
    CONSUME_TEXT,
  };

  Derived*
  DerivedClass() const
  {
    return static_cast<Derived*>(const_cast<FetchBody*>(this));
  }

  nsresult
  BeginConsumeBody();

  already_AddRefed<Promise>
  ConsumeBody(ConsumeType aType, ErrorResult& aRv);

  bool
  AddRefObject();

  void
  ReleaseObject();

  bool
  RegisterFeature();

  void
  UnregisterFeature();

  bool
  IsOnTargetThread()
  {
    return NS_IsMainThread() == !mWorkerPrivate;
  }

  void
  AssertIsOnTargetThread()
  {
    MOZ_ASSERT(IsOnTargetThread());
  }

  
  bool mBodyUsed;
  nsCString mMimeType;

  
  ConsumeType mConsumeType;
  nsRefPtr<Promise> mConsumePromise;
  DebugOnly<bool> mReadDone;

  nsMainThreadPtrHandle<nsIInputStreamPump> mConsumeBodyPump;
};

} 
} 

#endif 
