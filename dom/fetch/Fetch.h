




#ifndef mozilla_dom_Fetch_h
#define mozilla_dom_Fetch_h

#include "mozilla/dom/UnionTypes.h"

class nsIInputStream;

namespace mozilla {
namespace dom {

class Promise;






nsresult
ExtractByteStreamFromBody(const OwningArrayBufferOrArrayBufferViewOrScalarValueStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType);




nsresult
ExtractByteStreamFromBody(const ArrayBufferOrArrayBufferViewOrScalarValueStringOrURLSearchParams& aBodyInit,
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
