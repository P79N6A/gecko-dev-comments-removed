




#ifndef mozilla_dom_Fetch_h
#define mozilla_dom_Fetch_h

#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsString.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/RequestBinding.h"

class nsIInputStream;
class nsIGlobalObject;

namespace mozilla {
namespace dom {

class ArrayBufferOrArrayBufferViewOrBlobOrScalarValueStringOrURLSearchParams;
class InternalRequest;
class OwningArrayBufferOrArrayBufferViewOrBlobOrScalarValueStringOrURLSearchParams;
class Promise;
class RequestOrScalarValueString;

namespace workers {
class WorkerPrivate;
} 

already_AddRefed<Promise>
FetchRequest(nsIGlobalObject* aGlobal, const RequestOrScalarValueString& aInput,
             const RequestInit& aInit, ErrorResult& aRv);

nsresult
GetRequestReferrer(nsIGlobalObject* aGlobal, const InternalRequest* aRequest, nsCString& aReferrer);






nsresult
ExtractByteStreamFromBody(const OwningArrayBufferOrArrayBufferViewOrBlobOrScalarValueStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType);




nsresult
ExtractByteStreamFromBody(const ArrayBufferOrArrayBufferViewOrBlobOrScalarValueStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType);

template <class Derived>
class FetchBody {
public:
  bool
  BodyUsed() { return mBodyUsed; }

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
  Json(ErrorResult& aRv)
  {
    return ConsumeBody(CONSUME_JSON, aRv);
  }

  already_AddRefed<Promise>
  Text(ErrorResult& aRv)
  {
    return ConsumeBody(CONSUME_TEXT, aRv);
  }

protected:
  FetchBody()
    : mBodyUsed(false)
  {
  }

  void
  SetBodyUsed()
  {
    mBodyUsed = true;
  }

  void
  SetMimeType(ErrorResult& aRv);

private:
  enum ConsumeType
  {
    CONSUME_ARRAYBUFFER,
    CONSUME_BLOB,
    
    CONSUME_JSON,
    CONSUME_TEXT,
  };

  Derived*
  DerivedClass() const
  {
    return static_cast<Derived*>(const_cast<FetchBody*>(this));
  }

  already_AddRefed<Promise>
  ConsumeBody(ConsumeType aType, ErrorResult& aRv);

  bool mBodyUsed;
  nsCString mMimeType;
};

} 
} 

#endif 
