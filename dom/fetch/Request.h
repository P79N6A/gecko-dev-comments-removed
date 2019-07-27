




#ifndef mozilla_dom_Request_h
#define mozilla_dom_Request_h

#include "nsISupportsImpl.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/InternalRequest.h"


#include "mozilla/dom/RequestBinding.h"
#include "mozilla/dom/UnionTypes.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Headers;
class Promise;

class Request MOZ_FINAL : public nsISupports
                        , public nsWrapperCache
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Request)

public:
  Request(nsIGlobalObject* aOwner, InternalRequest* aRequest);

  JSObject*
  WrapObject(JSContext* aCx)
  {
    return RequestBinding::Wrap(aCx, this);
  }

  void
  GetUrl(DOMString& aUrl) const
  {
    aUrl.AsAString() = NS_ConvertUTF8toUTF16(mRequest->mURL);
  }

  void
  GetMethod(nsCString& aMethod) const
  {
    aMethod = mRequest->mMethod;
  }

  RequestMode
  Mode() const
  {
    return mRequest->mMode;
  }

  RequestCredentials
  Credentials() const
  {
    return mRequest->mCredentialsMode;
  }

  void
  GetReferrer(DOMString& aReferrer) const
  {
    if (mRequest->ReferrerIsNone()) {
      aReferrer.AsAString() = EmptyString();
      return;
    }

    
    aReferrer.AsAString() = NS_ConvertUTF8toUTF16(mRequest->mReferrerURL);
  }

  Headers* Headers_() const { return mRequest->Headers_(); }

  static already_AddRefed<Request>
  Constructor(const GlobalObject& aGlobal, const RequestOrScalarValueString& aInput,
              const RequestInit& aInit, ErrorResult& rv);

  nsISupports* GetParentObject() const
  {
    return mOwner;
  }

  already_AddRefed<Request>
  Clone() const;

  already_AddRefed<Promise>
  ArrayBuffer(ErrorResult& aRv);

  already_AddRefed<Promise>
  Blob(ErrorResult& aRv);

  already_AddRefed<Promise>
  Json(ErrorResult& aRv);

  already_AddRefed<Promise>
  Text(ErrorResult& aRv);

  bool
  BodyUsed() const
  {
    return mBodyUsed;
  }

  already_AddRefed<InternalRequest>
  GetInternalRequest();
private:
  enum ConsumeType
  {
    CONSUME_ARRAYBUFFER,
    CONSUME_BLOB,
    
    CONSUME_JSON,
    CONSUME_TEXT,
  };

  ~Request();

  already_AddRefed<Promise>
  ConsumeBody(ConsumeType aType, ErrorResult& aRv);

  void
  SetBodyUsed()
  {
    mBodyUsed = true;
  }

  nsCOMPtr<nsIGlobalObject> mOwner;
  nsRefPtr<InternalRequest> mRequest;
  bool mBodyUsed;
  nsCString mMimeType;
};

} 
} 

#endif
