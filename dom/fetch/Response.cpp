




#include "Response.h"

#include "nsISupportsImpl.h"
#include "nsIURI.h"
#include "nsPIDOMWindow.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/FetchBinding.h"
#include "mozilla/dom/Headers.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/URL.h"
#include "mozilla/dom/workers/bindings/URL.h"

#include "nsDOMString.h"

#include "InternalResponse.h"
#include "WorkerPrivate.h"

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
                   uint16_t aStatus, ErrorResult& aRv)
{
  nsAutoString parsedURL;

  if (NS_IsMainThread()) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
    nsCOMPtr<nsIURI> docURI = window->GetDocumentURI();
    nsAutoCString spec;
    aRv = docURI->GetSpec(spec);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }

    nsRefPtr<mozilla::dom::URL> url =
      dom::URL::Constructor(aGlobal, aUrl, NS_ConvertUTF8toUTF16(spec), aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    url->Stringify(parsedURL, aRv);
  } else {
    workers::WorkerPrivate* worker = workers::GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    NS_ConvertUTF8toUTF16 baseURL(worker->GetLocationInfo().mHref);
    nsRefPtr<workers::URL> url =
      workers::URL::Constructor(aGlobal, aUrl, baseURL, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }

    url->Stringify(parsedURL, aRv);
  }

  if (aRv.Failed()) {
    return nullptr;
  }

  if (aStatus != 301 && aStatus != 302 && aStatus != 303 && aStatus != 307 && aStatus != 308) {
    aRv.ThrowRangeError(MSG_INVALID_REDIRECT_STATUSCODE_ERROR);
    return nullptr;
  }

  Optional<ArrayBufferOrArrayBufferViewOrBlobOrUSVStringOrURLSearchParams> body;
  ResponseInit init;
  init.mStatus = aStatus;
  nsRefPtr<Response> r = Response::Constructor(aGlobal, body, init, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  r->GetInternalHeaders()->Set(NS_LITERAL_CSTRING("Location"),
                               NS_ConvertUTF16toUTF8(parsedURL), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return r.forget();
}

 already_AddRefed<Response>
Response::Constructor(const GlobalObject& aGlobal,
                      const Optional<ArrayBufferOrArrayBufferViewOrBlobOrUSVStringOrURLSearchParams>& aBody,
                      const ResponseInit& aInit, ErrorResult& aRv)
{
  if (aInit.mStatus < 200 || aInit.mStatus > 599) {
    aRv.ThrowRangeError(MSG_INVALID_RESPONSE_STATUSCODE_ERROR);
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

void
Response::SetFinalURL(bool aFinalURL, ErrorResult& aRv)
{
  nsCString url;
  mInternalResponse->GetUrl(url);
  if (url.IsEmpty()) {
    aRv.ThrowTypeError(MSG_RESPONSE_URL_IS_NULL);
    return;
  }

  mInternalResponse->SetFinalURL(aFinalURL);
}
} 
} 
