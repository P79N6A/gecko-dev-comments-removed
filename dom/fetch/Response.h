




#ifndef mozilla_dom_Response_h
#define mozilla_dom_Response_h

#include "nsWrapperCache.h"
#include "nsISupportsImpl.h"

#include "mozilla/dom/Fetch.h"
#include "mozilla/dom/ResponseBinding.h"

#include "InternalResponse.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class ArrayBufferOrArrayBufferViewOrScalarValueStringOrURLSearchParams;
class Headers;
class InternalHeaders;
class Promise;

class Response MOZ_FINAL : public nsISupports
                         , public nsWrapperCache
                         , public FetchBody<Response>
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Response)

public:
  Response(nsIGlobalObject* aGlobal, InternalResponse* aInternalResponse);

  Response(const Response& aOther) MOZ_DELETE;

  JSObject*
  WrapObject(JSContext* aCx)
  {
    return ResponseBinding::Wrap(aCx, this);
  }

  ResponseType
  Type() const
  {
    return mInternalResponse->Type();
  }

  void
  GetUrl(DOMString& aUrl) const
  {
    aUrl.AsAString() = NS_ConvertUTF8toUTF16(mInternalResponse->GetUrl());
  }

  uint16_t
  Status() const
  {
    return mInternalResponse->GetStatus();
  }

  void
  GetStatusText(nsCString& aStatusText) const
  {
    aStatusText = mInternalResponse->GetStatusText();
  }

  InternalHeaders*
  GetInternalHeaders() const
  {
    return mInternalResponse->Headers();
  }

  Headers* Headers_();

  void
  GetBody(nsIInputStream** aStream) { return mInternalResponse->GetBody(aStream); }

  static already_AddRefed<Response>
  Error(const GlobalObject& aGlobal);

  static already_AddRefed<Response>
  Redirect(const GlobalObject& aGlobal, const nsAString& aUrl, uint16_t aStatus);

  static already_AddRefed<Response>
  Constructor(const GlobalObject& aGlobal,
              const Optional<ArrayBufferOrArrayBufferViewOrBlobOrScalarValueStringOrURLSearchParams>& aBody,
              const ResponseInit& aInit, ErrorResult& rv);

  nsIGlobalObject* GetParentObject() const
  {
    return mOwner;
  }

  already_AddRefed<Response>
  Clone();

  void
  SetBody(nsIInputStream* aBody);
private:
  ~Response();

  nsCOMPtr<nsIGlobalObject> mOwner;
  nsRefPtr<InternalResponse> mInternalResponse;
  
  nsRefPtr<Headers> mHeaders;
};

} 
} 

#endif 
