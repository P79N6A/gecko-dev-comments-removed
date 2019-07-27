




#ifndef mozilla_dom_Request_h
#define mozilla_dom_Request_h

#include "nsISupportsImpl.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/RequestBinding.h"
#include "mozilla/dom/UnionTypes.h"


class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class Promise;

class Request MOZ_FINAL : public nsISupports
                        , public nsWrapperCache
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Request)

public:
  Request(nsISupports* aOwner);

  JSObject*
  WrapObject(JSContext* aCx)
  {
    return RequestBinding::Wrap(aCx, this);
  }

  void
  GetUrl(DOMString& aUrl) const
  {
    aUrl.AsAString() = EmptyString();
  }

  void
  GetMethod(nsCString& aMethod) const
  {
    aMethod = EmptyCString();
  }

  RequestMode
  Mode() const
  {
    return RequestMode::Same_origin;
  }

  RequestCredentials
  Credentials() const
  {
    return RequestCredentials::Omit;
  }

  void
  GetReferrer(DOMString& aReferrer) const
  {
    aReferrer.AsAString() = EmptyString();
  }

  Headers* Headers_() const { return mHeaders; }

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
  BodyUsed();
private:
  ~Request();

  nsCOMPtr<nsISupports> mOwner;
  nsRefPtr<Headers> mHeaders;
};

} 
} 

#endif 
