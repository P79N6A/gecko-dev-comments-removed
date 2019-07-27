





#include "mozilla/dom/cache/TypeUtils.h"

#include "mozilla/unused.h"
#include "mozilla/dom/CacheBinding.h"
#include "mozilla/dom/InternalRequest.h"
#include "mozilla/dom/Request.h"
#include "mozilla/dom/Response.h"
#include "mozilla/dom/cache/CachePushStreamChild.h"
#include "mozilla/dom/cache/PCacheTypes.h"
#include "mozilla/dom/cache/ReadStream.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/FileDescriptorSetChild.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/ipc/PFileDescriptorSetChild.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "nsCOMPtr.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIIPCSerializableInputStream.h"
#include "nsStreamUtils.h"
#include "nsString.h"
#include "nsURLParsers.h"

namespace {

using mozilla::ErrorResult;
using mozilla::unused;
using mozilla::void_t;
using mozilla::dom::cache::PCacheReadStream;
using mozilla::ipc::BackgroundChild;
using mozilla::ipc::FileDescriptor;
using mozilla::ipc::PBackgroundChild;
using mozilla::ipc::PFileDescriptorSetChild;




static void
ProcessURL(nsAString& aUrl, bool* aSchemeValidOut,
           nsAString* aUrlWithoutQueryOut, ErrorResult& aRv)
{
  NS_ConvertUTF16toUTF8 flatURL(aUrl);
  const char* url = flatURL.get();

  
  nsCOMPtr<nsIURLParser> urlParser = new nsStdURLParser();

  uint32_t pathPos;
  int32_t pathLen;
  uint32_t schemePos;
  int32_t schemeLen;
  aRv = urlParser->ParseURL(url, flatURL.Length(), &schemePos, &schemeLen,
                            nullptr, nullptr,       
                            &pathPos, &pathLen);
  if (NS_WARN_IF(aRv.Failed())) { return; }

  if (aSchemeValidOut) {
    nsAutoCString scheme(Substring(flatURL, schemePos, schemeLen));
    *aSchemeValidOut = scheme.LowerCaseEqualsLiteral("http") ||
                       scheme.LowerCaseEqualsLiteral("https");
  }

  uint32_t queryPos;
  int32_t queryLen;
  uint32_t refPos;
  int32_t refLen;

  aRv = urlParser->ParsePath(url + pathPos, flatURL.Length() - pathPos,
                             nullptr, nullptr,               
                             &queryPos, &queryLen,
                             &refPos, &refLen);
  if (NS_WARN_IF(aRv.Failed())) {
    return;
  }

  
  if (refLen >= 0) {
    
    refPos += pathPos;

    aUrl = Substring(aUrl, 0, refPos - 1);
  }

  if (!aUrlWithoutQueryOut) {
    return;
  }

  if (queryLen < 0) {
    *aUrlWithoutQueryOut = aUrl;
    return;
  }

  
  queryPos += pathPos;

  
  
  *aUrlWithoutQueryOut = Substring(aUrl, 0, queryPos - 1);
}

void
SerializeNormalStream(nsIInputStream* aStream, PCacheReadStream& aReadStreamOut)
{
  nsAutoTArray<FileDescriptor, 4> fds;
  SerializeInputStream(aStream, aReadStreamOut.params(), fds);

  PFileDescriptorSetChild* fdSet = nullptr;
  if (!fds.IsEmpty()) {
    
    PBackgroundChild* manager = BackgroundChild::GetForCurrentThread();
    MOZ_ASSERT(manager);

    fdSet = manager->SendPFileDescriptorSetConstructor(fds[0]);
    for (uint32_t i = 1; i < fds.Length(); ++i) {
      unused << fdSet->SendAddFileDescriptor(fds[i]);
    }
  }

  if (fdSet) {
    aReadStreamOut.fds() = fdSet;
  } else {
    aReadStreamOut.fds() = void_t();
  }
}

} 

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::ipc::BackgroundChild;
using mozilla::ipc::FileDescriptor;
using mozilla::ipc::FileDescriptorSetChild;
using mozilla::ipc::PFileDescriptorSetChild;
using mozilla::ipc::PBackgroundChild;
using mozilla::ipc::OptionalFileDescriptorSet;


already_AddRefed<InternalRequest>
TypeUtils::ToInternalRequest(const RequestOrUSVString& aIn,
                             BodyAction aBodyAction, ErrorResult& aRv)
{
  if (aIn.IsRequest()) {
    Request& request = aIn.GetAsRequest();

    
    
    CheckAndSetBodyUsed(&request, aBodyAction, aRv);
    if (aRv.Failed()) { return nullptr; }

    return request.GetInternalRequest();
  }

  return ToInternalRequest(aIn.GetAsUSVString(), aRv);
}

already_AddRefed<InternalRequest>
TypeUtils::ToInternalRequest(const OwningRequestOrUSVString& aIn,
                             BodyAction aBodyAction, ErrorResult& aRv)
{

  if (aIn.IsRequest()) {
    nsRefPtr<Request> request = aIn.GetAsRequest().get();

    
    
    CheckAndSetBodyUsed(request, aBodyAction, aRv);
    if (aRv.Failed()) { return nullptr; }

    return request->GetInternalRequest();
  }

  return ToInternalRequest(aIn.GetAsUSVString(), aRv);
}

void
TypeUtils::ToPCacheRequest(PCacheRequest& aOut, InternalRequest* aIn,
                           BodyAction aBodyAction,
                           ReferrerAction aReferrerAction,
                           SchemeAction aSchemeAction, ErrorResult& aRv)
{
  MOZ_ASSERT(aIn);

  aIn->GetMethod(aOut.method());

  nsAutoCString url;
  aIn->GetURL(url);
  CopyUTF8toUTF16(url, aOut.url());

  bool schemeValid;
  ProcessURL(aOut.url(), &schemeValid, &aOut.urlWithoutQuery(), aRv);
  if (aRv.Failed()) {
    return;
  }

  if (!schemeValid) {
    if (aSchemeAction == TypeErrorOnInvalidScheme) {
      NS_NAMED_LITERAL_STRING(label, "Request");
      aRv.ThrowTypeError(MSG_INVALID_URL_SCHEME, &label, &aOut.url());
      return;
    }

    if (aSchemeAction == NetworkErrorOnInvalidScheme) {
      aRv.Throw(NS_ERROR_DOM_NETWORK_ERR);
      return;
    }
  }

  if (aReferrerAction == ExpandReferrer) {
    UpdateRequestReferrer(GetGlobalObject(), aIn);
  }
  aIn->GetReferrer(aOut.referrer());

  nsRefPtr<InternalHeaders> headers = aIn->Headers();
  MOZ_ASSERT(headers);
  headers->GetPHeaders(aOut.headers());
  aOut.headersGuard() = headers->Guard();
  aOut.mode() = aIn->Mode();
  aOut.credentials() = aIn->GetCredentialsMode();
  aOut.contentPolicyType() = aIn->ContentPolicyType();
  aOut.context() = aIn->Context();
  aOut.requestCache() = aIn->GetCacheMode();

  if (aBodyAction == IgnoreBody) {
    aOut.body() = void_t();
    return;
  }

  

  nsCOMPtr<nsIInputStream> stream;
  aIn->GetBody(getter_AddRefs(stream));
  SerializeCacheStream(stream, &aOut.body(), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return;
  }
}

void
TypeUtils::ToPCacheResponseWithoutBody(PCacheResponse& aOut,
                                       InternalResponse& aIn, ErrorResult& aRv)
{
  aOut.type() = aIn.Type();

  nsAutoCString url;
  aIn.GetUrl(url);
  CopyUTF8toUTF16(url, aOut.url());

  if (aOut.url() != EmptyString()) {
    
    
    ProcessURL(aOut.url(), nullptr, nullptr, aRv);
    if (aRv.Failed()) {
      return;
    }
  }

  aOut.status() = aIn.GetStatus();
  aOut.statusText() = aIn.GetStatusText();
  nsRefPtr<InternalHeaders> headers = aIn.UnfilteredHeaders();
  MOZ_ASSERT(headers);
  headers->GetPHeaders(aOut.headers());
  aOut.headersGuard() = headers->Guard();
  aOut.securityInfo() = aIn.GetSecurityInfo();
}

void
TypeUtils::ToPCacheResponse(PCacheResponse& aOut, Response& aIn, ErrorResult& aRv)
{
  if (aIn.BodyUsed()) {
    aRv.ThrowTypeError(MSG_FETCH_BODY_CONSUMED_ERROR);
    return;
  }

  nsRefPtr<InternalResponse> ir = aIn.GetInternalResponse();
  ToPCacheResponseWithoutBody(aOut, *ir, aRv);

  nsCOMPtr<nsIInputStream> stream;
  aIn.GetBody(getter_AddRefs(stream));
  if (stream) {
    aIn.SetBodyUsed();
  }

  SerializeCacheStream(stream, &aOut.body(), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return;
  }
}


void
TypeUtils::ToPCacheQueryParams(PCacheQueryParams& aOut,
                               const CacheQueryOptions& aIn)
{
  aOut.ignoreSearch() = aIn.mIgnoreSearch;
  aOut.ignoreMethod() = aIn.mIgnoreMethod;
  aOut.ignoreVary() = aIn.mIgnoreVary;
  aOut.cacheNameSet() = aIn.mCacheName.WasPassed();
  if (aOut.cacheNameSet()) {
    aOut.cacheName() = aIn.mCacheName.Value();
  } else {
    aOut.cacheName() = NS_LITERAL_STRING("");
  }
}

already_AddRefed<Response>
TypeUtils::ToResponse(const PCacheResponse& aIn)
{
  if (aIn.type() == ResponseType::Error) {
    nsRefPtr<InternalResponse> error = InternalResponse::NetworkError();
    nsRefPtr<Response> r = new Response(GetGlobalObject(), error);
    return r.forget();
  }

  nsRefPtr<InternalResponse> ir = new InternalResponse(aIn.status(),
                                                       aIn.statusText());
  ir->SetUrl(NS_ConvertUTF16toUTF8(aIn.url()));

  nsRefPtr<InternalHeaders> internalHeaders =
    new InternalHeaders(aIn.headers(), aIn.headersGuard());
  ErrorResult result;
  ir->Headers()->SetGuard(aIn.headersGuard(), result);
  MOZ_ASSERT(!result.Failed());
  ir->Headers()->Fill(*internalHeaders, result);
  MOZ_ASSERT(!result.Failed());

  ir->SetSecurityInfo(aIn.securityInfo());

  nsCOMPtr<nsIInputStream> stream = ReadStream::Create(aIn.body());
  ir->SetBody(stream);

  switch (aIn.type())
  {
    case ResponseType::Default:
      break;
    case ResponseType::Opaque:
      ir = ir->OpaqueResponse();
      break;
    case ResponseType::Basic:
      ir = ir->BasicResponse();
      break;
    case ResponseType::Cors:
      ir = ir->CORSResponse();
      break;
    default:
      MOZ_CRASH("Unexpected ResponseType!");
  }
  MOZ_ASSERT(ir);

  nsRefPtr<Response> ref = new Response(GetGlobalObject(), ir);
  return ref.forget();
}

already_AddRefed<InternalRequest>
TypeUtils::ToInternalRequest(const PCacheRequest& aIn)
{
  nsRefPtr<InternalRequest> internalRequest = new InternalRequest();

  internalRequest->SetMethod(aIn.method());
  internalRequest->SetURL(NS_ConvertUTF16toUTF8(aIn.url()));
  internalRequest->SetReferrer(aIn.referrer());
  internalRequest->SetMode(aIn.mode());
  internalRequest->SetCredentialsMode(aIn.credentials());
  internalRequest->SetContentPolicyType(aIn.contentPolicyType());
  DebugOnly<RequestContext> contextAfterSetContentPolicyType = internalRequest->Context();
  internalRequest->SetContext(aIn.context());
  MOZ_ASSERT(contextAfterSetContentPolicyType.value == internalRequest->Context(),
             "The RequestContext and nsContentPolicyType values should not get out of sync");
  internalRequest->SetCacheMode(aIn.requestCache());

  nsRefPtr<InternalHeaders> internalHeaders =
    new InternalHeaders(aIn.headers(), aIn.headersGuard());
  ErrorResult result;
  internalRequest->Headers()->SetGuard(aIn.headersGuard(), result);
  MOZ_ASSERT(!result.Failed());
  internalRequest->Headers()->Fill(*internalHeaders, result);
  MOZ_ASSERT(!result.Failed());

  nsCOMPtr<nsIInputStream> stream = ReadStream::Create(aIn.body());

  internalRequest->SetBody(stream);

  return internalRequest.forget();
}

already_AddRefed<Request>
TypeUtils::ToRequest(const PCacheRequest& aIn)
{
  nsRefPtr<InternalRequest> internalRequest = ToInternalRequest(aIn);
  nsRefPtr<Request> request = new Request(GetGlobalObject(), internalRequest);
  return request.forget();
}

void
TypeUtils::CheckAndSetBodyUsed(Request* aRequest, BodyAction aBodyAction,
                               ErrorResult& aRv)
{
  MOZ_ASSERT(aRequest);

  if (aBodyAction == IgnoreBody) {
    return;
  }

  if (aRequest->BodyUsed()) {
    aRv.ThrowTypeError(MSG_FETCH_BODY_CONSUMED_ERROR);
    return;
  }

  nsCOMPtr<nsIInputStream> stream;
  aRequest->GetBody(getter_AddRefs(stream));
  if (stream) {
    aRequest->SetBodyUsed();
  }
}

already_AddRefed<InternalRequest>
TypeUtils::ToInternalRequest(const nsAString& aIn, ErrorResult& aRv)
{
  RequestOrUSVString requestOrString;
  requestOrString.SetAsUSVString().Rebind(aIn.Data(), aIn.Length());

  
  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(GetGlobalObject()))) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }
  JSContext* cx = jsapi.cx();
  GlobalObject global(cx, GetGlobalObject()->GetGlobalJSObject());
  MOZ_ASSERT(!global.Failed());

  nsRefPtr<Request> request = Request::Constructor(global, requestOrString,
                                                   RequestInit(), aRv);
  if (NS_WARN_IF(aRv.Failed())) { return nullptr; }

  return request->GetInternalRequest();
}

void
TypeUtils::SerializeCacheStream(nsIInputStream* aStream,
                                PCacheReadStreamOrVoid* aStreamOut,
                                ErrorResult& aRv)
{
  *aStreamOut = void_t();
  if (!aStream) {
    return;
  }

  
  nsRefPtr<ReadStream> controlled = do_QueryObject(aStream);
  if (controlled) {
    controlled->Serialize(aStreamOut);
    return;
  }

  PCacheReadStream readStream;
  readStream.controlChild() = nullptr;
  readStream.controlParent() = nullptr;
  readStream.pushStreamChild() = nullptr;
  readStream.pushStreamParent() = nullptr;

  
  nsCOMPtr<nsIIPCSerializableInputStream> serial = do_QueryInterface(aStream);
  if (serial) {
    SerializeNormalStream(aStream, readStream);

  
  
  
  } else {
    SerializePushStream(aStream, readStream, aRv);
    if (NS_WARN_IF(aRv.Failed())) { return; }
  }

  *aStreamOut = readStream;
}

void
TypeUtils::SerializePushStream(nsIInputStream* aStream,
                               PCacheReadStream& aReadStreamOut,
                               ErrorResult& aRv)
{
  nsCOMPtr<nsIAsyncInputStream> asyncStream = do_QueryInterface(aStream);
  if (NS_WARN_IF(!asyncStream)) {
    aRv = NS_ERROR_FAILURE;
    return;
  }

  bool nonBlocking = false;
  aRv = asyncStream->IsNonBlocking(&nonBlocking);
  if (NS_WARN_IF(aRv.Failed())) { return; }
  if (NS_WARN_IF(!nonBlocking)) {
    aRv = NS_ERROR_FAILURE;
    return;
  }

  aReadStreamOut.pushStreamChild() = CreatePushStream(asyncStream);
  MOZ_ASSERT(aReadStreamOut.pushStreamChild());
  aReadStreamOut.params() = void_t();
  aReadStreamOut.fds() = void_t();

  
  
}

} 
} 
} 
