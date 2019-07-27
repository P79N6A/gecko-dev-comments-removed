




#include "Response.h"

#include "nsISupportsImpl.h"
#include "nsIURI.h"
#include "nsPIDOMWindow.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/FetchBinding.h"
#include "mozilla/dom/Headers.h"
#include "mozilla/dom/Promise.h"

#include "nsDOMString.h"

#include "InternalResponse.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(Response)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Response)
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(Response, mOwner, mHeaders)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Response)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Response::Response(nsIGlobalObject* aGlobal, InternalResponse* aInternalResponse)
  : FetchBody<Response>()
  , mOwner(aGlobal)
  , mInternalResponse(aInternalResponse)
{
}

Response::~Response()
{
}

 already_AddRefed<Response>
Response::Error(const GlobalObject& aGlobal)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<InternalResponse> error = InternalResponse::NetworkError();
  nsRefPtr<Response> r = new Response(global, error);
  return r.forget();
}

 already_AddRefed<Response>
Response::Redirect(const GlobalObject& aGlobal, const nsAString& aUrl,
                   uint16_t aStatus)
{
  ErrorResult result;
  ResponseInit init;
  Optional<ArrayBufferOrArrayBufferViewOrBlobOrUSVStringOrURLSearchParams> body;
  nsRefPtr<Response> r = Response::Constructor(aGlobal, body, init, result);
  return r.forget();
}

 already_AddRefed<Response>
Response::Constructor(const GlobalObject& aGlobal,
                      const Optional<ArrayBufferOrArrayBufferViewOrBlobOrUSVStringOrURLSearchParams>& aBody,
                      const ResponseInit& aInit, ErrorResult& aRv)
{
  if (aInit.mStatus < 200 || aInit.mStatus > 599) {
    aRv.Throw(NS_ERROR_RANGE_ERR);
    return nullptr;
  }

  nsCString statusText;
  if (aInit.mStatusText.WasPassed()) {
    statusText = aInit.mStatusText.Value();
    nsACString::const_iterator start, end;
    statusText.BeginReading(start);
    statusText.EndReading(end);
    if (FindCharInReadable('\r', start, end)) {
      aRv.ThrowTypeError(MSG_RESPONSE_INVALID_STATUSTEXT_ERROR);
      return nullptr;
    }
    
    statusText.BeginReading(start);
    if (FindCharInReadable('\n', start, end)) {
      aRv.ThrowTypeError(MSG_RESPONSE_INVALID_STATUSTEXT_ERROR);
      return nullptr;
    }
  } else {
    
    statusText = NS_LITERAL_CSTRING("OK");
  }

  nsRefPtr<InternalResponse> internalResponse =
    new InternalResponse(aInit.mStatus, statusText);

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<Response> r = new Response(global, internalResponse);

  if (aInit.mHeaders.WasPassed()) {
    internalResponse->Headers()->Clear();

    
    
    nsRefPtr<Headers> headers =
      Headers::Constructor(aGlobal, aInit.mHeaders.Value(), aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    internalResponse->Headers()->Fill(*headers->GetInternalHeaders(), aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }
  }

  if (aBody.WasPassed()) {
    nsCOMPtr<nsIInputStream> bodyStream;
    nsCString contentType;
    aRv = ExtractByteStreamFromBody(aBody.Value(), getter_AddRefs(bodyStream), contentType);
    internalResponse->SetBody(bodyStream);

    if (!contentType.IsVoid() &&
        !internalResponse->Headers()->Has(NS_LITERAL_CSTRING("Content-Type"), aRv)) {
      internalResponse->Headers()->Append(NS_LITERAL_CSTRING("Content-Type"), contentType, aRv);
    }

    if (aRv.Failed()) {
      return nullptr;
    }
  }

  r->SetMimeType(aRv);
  return r.forget();
}


already_AddRefed<Response>
Response::Clone()
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(mOwner);
  nsRefPtr<Response> response = new Response(global, mInternalResponse);
  return response.forget();
}

void
Response::SetBody(nsIInputStream* aBody)
{
  
  mInternalResponse->SetBody(aBody);
}

Headers*
Response::Headers_()
{
  if (!mHeaders) {
    mHeaders = new Headers(mOwner, mInternalResponse->Headers());
  }

  return mHeaders;
}
} 
} 
