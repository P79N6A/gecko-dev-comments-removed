




#ifndef mozilla_dom_Request_h
#define mozilla_dom_Request_h

#include "nsIContentPolicy.h"
#include "nsISupportsImpl.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/Fetch.h"
#include "mozilla/dom/InternalRequest.h"


#include "mozilla/dom/RequestBinding.h"

namespace mozilla {
namespace dom {

class Headers;
class InternalHeaders;
class RequestOrUSVString;

class Request final : public nsISupports
                    , public FetchBody<Request>
                    , public nsWrapperCache
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Request)

public:
  Request(nsIGlobalObject* aOwner, InternalRequest* aRequest);

  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return RequestBinding::Wrap(aCx, this, aGivenProto);
  }

  void
  GetUrl(nsAString& aUrl) const
  {
    CopyUTF8toUTF16(mRequest->mURL, aUrl);
  }

  void
  GetMethod(nsCString& aMethod) const
  {
    aMethod = mRequest->mMethod;
  }

  RequestMode
  Mode() const
  {
    if (mRequest->mMode == RequestMode::Cors_with_forced_preflight) {
      return RequestMode::Cors;
    }
    return mRequest->mMode;
  }

  RequestCredentials
  Credentials() const
  {
    return mRequest->mCredentialsMode;
  }

  RequestCache
  Cache() const
  {
    return mRequest->GetCacheMode();
  }

  RequestContext
  Context() const
  {
    return mRequest->Context();
  }

  
  void
  SetContext(RequestContext aContext)
  {
    mRequest->SetContext(aContext);
  }

  void
  SetContentPolicyType(nsContentPolicyType aContentPolicyType)
  {
    mRequest->SetContentPolicyType(aContentPolicyType);
  }

  void
  GetReferrer(nsAString& aReferrer) const
  {
    mRequest->GetReferrer(aReferrer);
  }

  InternalHeaders*
  GetInternalHeaders() const
  {
    return mRequest->Headers();
  }

  Headers* Headers_();

  void
  GetBody(nsIInputStream** aStream) { return mRequest->GetBody(aStream); }

  static already_AddRefed<Request>
  Constructor(const GlobalObject& aGlobal, const RequestOrUSVString& aInput,
              const RequestInit& aInit, ErrorResult& rv);

  nsIGlobalObject* GetParentObject() const
  {
    return mOwner;
  }

  already_AddRefed<Request>
  Clone(ErrorResult& aRv) const;

  already_AddRefed<InternalRequest>
  GetInternalRequest();
private:
  ~Request();

  nsCOMPtr<nsIGlobalObject> mOwner;
  nsRefPtr<InternalRequest> mRequest;
  
  nsRefPtr<Headers> mHeaders;
};

} 
} 

#endif 
