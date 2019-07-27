




#include "mozilla/dom/FetchDriver.h"

#include "nsIDocument.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIScriptSecurityManager.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsIUploadChannel2.h"

#include "nsContentPolicyUtils.h"
#include "nsCORSListenerProxy.h"
#include "nsDataHandler.h"
#include "nsHostObjectProtocolHandler.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsStringStream.h"

#include "mozilla/dom/File.h"
#include "mozilla/dom/workers/Workers.h"

#include "Fetch.h"
#include "InternalRequest.h"
#include "InternalResponse.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(FetchDriver,
                  nsIStreamListener, nsIChannelEventSink, nsIInterfaceRequestor,
                  nsIAsyncVerifyRedirectCallback)

FetchDriver::FetchDriver(InternalRequest* aRequest, nsIPrincipal* aPrincipal,
                         nsILoadGroup* aLoadGroup)
  : mPrincipal(aPrincipal)
  , mLoadGroup(aLoadGroup)
  , mRequest(aRequest)
  , mFetchRecursionCount(0)
  , mResponseAvailableCalled(false)
{
}

FetchDriver::~FetchDriver()
{
  
  
  MOZ_ASSERT(mResponseAvailableCalled);
}

nsresult
FetchDriver::Fetch(FetchDriverObserver* aObserver)
{
  workers::AssertIsOnMainThread();
  mObserver = aObserver;

  Telemetry::Accumulate(Telemetry::SERVICE_WORKER_REQUEST_PASSTHROUGH,
                        mRequest->WasCreatedByFetchEvent());

  return Fetch(false );
}

nsresult
FetchDriver::Fetch(bool aCORSFlag)
{
  
  MOZ_ASSERT(mFetchRecursionCount == 0);
  mFetchRecursionCount++;

  

  if (!mRequest->IsSynchronous() && mFetchRecursionCount <= 1) {
    nsCOMPtr<nsIRunnable> r =
      NS_NewRunnableMethodWithArg<bool>(this, &FetchDriver::ContinueFetch, aCORSFlag);
    nsresult rv = NS_DispatchToCurrentThread(r);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      FailWithNetworkError();
    }
    return rv;
  }

  MOZ_CRASH("Synchronous fetch not supported");
}

nsresult
FetchDriver::ContinueFetch(bool aCORSFlag)
{
  workers::AssertIsOnMainThread();

  nsAutoCString url;
  mRequest->GetURL(url);
  nsCOMPtr<nsIURI> requestURI;
  nsresult rv = NS_NewURI(getter_AddRefs(requestURI), url,
                          nullptr, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return FailWithNetworkError();
  }

  
  int16_t shouldLoad;
  rv = NS_CheckContentLoadPolicy(mRequest->ContentPolicyType(),
                                 requestURI,
                                 mPrincipal,
                                 mDocument,
                                 
                                 
                                 EmptyCString(), 
                                 nullptr, 
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_WARN_IF(NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad))) {
    
    return FailWithNetworkError();
  }

  
  

  nsAutoCString scheme;
  rv = requestURI->GetScheme(scheme);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return FailWithNetworkError();
  }

  rv = mPrincipal->CheckMayLoad(requestURI, false , false );
  if ((!aCORSFlag && NS_SUCCEEDED(rv)) ||
      (scheme.EqualsLiteral("data") && mRequest->SameOriginDataURL()) ||
      scheme.EqualsLiteral("about")) {
    return BasicFetch();
  }

  if (mRequest->Mode() == RequestMode::Same_origin) {
    return FailWithNetworkError();
  }

  if (mRequest->Mode() == RequestMode::No_cors) {
    mRequest->SetResponseTainting(InternalRequest::RESPONSETAINT_OPAQUE);
    return BasicFetch();
  }

  if (!scheme.EqualsLiteral("http") && !scheme.EqualsLiteral("https")) {
    return FailWithNetworkError();
  }

  bool corsPreflight = false;
  if (mRequest->Mode() == RequestMode::Cors_with_forced_preflight ||
      (mRequest->UnsafeRequest() && (!mRequest->HasSimpleMethod() || !mRequest->Headers()->HasOnlySimpleHeaders()))) {
    corsPreflight = true;
  }

  mRequest->SetResponseTainting(InternalRequest::RESPONSETAINT_CORS);
  return HttpFetch(true , corsPreflight);
}

nsresult
FetchDriver::BasicFetch()
{
  nsAutoCString url;
  mRequest->GetURL(url);
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri),
                 url,
                 nullptr,
                 nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    return rv;
  }

  nsAutoCString scheme;
  rv = uri->GetScheme(scheme);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    return rv;
  }

  if (scheme.LowerCaseEqualsLiteral("about")) {
    if (url.EqualsLiteral("about:blank")) {
      nsRefPtr<InternalResponse> response =
        new InternalResponse(200, NS_LITERAL_CSTRING("OK"));
      ErrorResult result;
      response->Headers()->Append(NS_LITERAL_CSTRING("content-type"),
                                  NS_LITERAL_CSTRING("text/html;charset=utf-8"),
                                  result);
      MOZ_ASSERT(!result.Failed());
      nsCOMPtr<nsIInputStream> body;
      rv = NS_NewCStringInputStream(getter_AddRefs(body), EmptyCString());
      if (NS_WARN_IF(NS_FAILED(rv))) {
        FailWithNetworkError();
        return rv;
      }

      response->SetBody(body);
      BeginResponse(response);
      return SucceedWithResponse();
    }
    return FailWithNetworkError();
  }

  if (scheme.LowerCaseEqualsLiteral("blob")) {
    nsRefPtr<FileImpl> blobImpl;
    rv = NS_GetBlobForBlobURI(uri, getter_AddRefs(blobImpl));
    FileImpl* blob = static_cast<FileImpl*>(blobImpl.get());
    if (NS_WARN_IF(NS_FAILED(rv))) {
      FailWithNetworkError();
      return rv;
    }

    nsRefPtr<InternalResponse> response = new InternalResponse(200, NS_LITERAL_CSTRING("OK"));
    {
      ErrorResult result;
      uint64_t size = blob->GetSize(result);
      if (NS_WARN_IF(result.Failed())) {
        FailWithNetworkError();
        return result.ErrorCode();
      }

      nsAutoString sizeStr;
      sizeStr.AppendInt(size);
      response->Headers()->Append(NS_LITERAL_CSTRING("Content-Length"), NS_ConvertUTF16toUTF8(sizeStr), result);
      if (NS_WARN_IF(result.Failed())) {
        FailWithNetworkError();
        return result.ErrorCode();
      }

      nsAutoString type;
      blob->GetType(type);
      response->Headers()->Append(NS_LITERAL_CSTRING("Content-Type"), NS_ConvertUTF16toUTF8(type), result);
      if (NS_WARN_IF(result.Failed())) {
        FailWithNetworkError();
        return result.ErrorCode();
      }
    }

    nsCOMPtr<nsIInputStream> stream;
    rv = blob->GetInternalStream(getter_AddRefs(stream));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      FailWithNetworkError();
      return rv;
    }

    response->SetBody(stream);
    BeginResponse(response);
    return SucceedWithResponse();
  }

  if (scheme.LowerCaseEqualsLiteral("data")) {
    nsAutoCString method;
    mRequest->GetMethod(method);
    if (method.LowerCaseEqualsASCII("get")) {
      
      
      
      
      nsAutoCString contentType, contentCharset, dataBuffer, hashRef;
      bool isBase64;
      rv = nsDataHandler::ParseURI(url,
                                   contentType,
                                   contentCharset,
                                   isBase64,
                                   dataBuffer,
                                   hashRef);
      if (NS_SUCCEEDED(rv)) {
        ErrorResult result;
        nsRefPtr<InternalResponse> response = new InternalResponse(200, NS_LITERAL_CSTRING("OK"));
        if (!contentCharset.IsEmpty()) {
          contentType.Append(";charset=");
          contentType.Append(contentCharset);
        }

        response->Headers()->Append(NS_LITERAL_CSTRING("Content-Type"), contentType, result);
        if (!result.Failed()) {
          nsCOMPtr<nsIInputStream> stream;
          rv = NS_NewCStringInputStream(getter_AddRefs(stream), dataBuffer);
          if (NS_SUCCEEDED(rv)) {
            response->SetBody(stream);
            BeginResponse(response);
            return SucceedWithResponse();
          }
        }
      }
    }

    return FailWithNetworkError();
  }

  if (scheme.LowerCaseEqualsLiteral("file")) {
  } else if (scheme.LowerCaseEqualsLiteral("http") ||
             scheme.LowerCaseEqualsLiteral("https")) {
    return HttpFetch();
  }

  return FailWithNetworkError();
}




nsresult
FetchDriver::HttpFetch(bool aCORSFlag, bool aCORSPreflightFlag, bool aAuthenticationFlag)
{
  
  mResponse = nullptr;
  nsresult rv;

  nsCOMPtr<nsIIOService> ios = do_GetIOService(&rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    return rv;
  }

  nsAutoCString url;
  mRequest->GetURL(url);
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri),
                          url,
                          nullptr,
                          nullptr,
                          ios);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    return rv;
  }

  
  
  
  

  
  
  
  
  
  
  
  
  
  

  
  
  
  bool useCredentials = false;
  if (mRequest->GetCredentialsMode() == RequestCredentials::Include ||
      (mRequest->GetCredentialsMode() == RequestCredentials::Same_origin && !aCORSFlag)) {
    useCredentials = true;
  }

  
  
  
  
  const nsLoadFlags credentialsFlag = useCredentials ? 0 : nsIRequest::LOAD_ANONYMOUS;

  
  
  MOZ_ASSERT(mLoadGroup);
  nsCOMPtr<nsIChannel> chan;
  rv = NS_NewChannel(getter_AddRefs(chan),
                     uri,
                     mPrincipal,
                     nsILoadInfo::SEC_NORMAL,
                     mRequest->ContentPolicyType(),
                     mLoadGroup,
                     nullptr, 
                     nsIRequest::LOAD_NORMAL | credentialsFlag,
                     ios);
  mLoadGroup = nullptr;
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    return rv;
  }

  
  
  chan->GetNotificationCallbacks(getter_AddRefs(mNotificationCallbacks));
  chan->SetNotificationCallbacks(this);

  
  
  
  
  

  
  
  
  
  nsCOMPtr<nsIHttpChannel> httpChan = do_QueryInterface(chan);
  if (httpChan) {
    
    nsAutoCString method;
    mRequest->GetMethod(method);
    rv = httpChan->SetRequestMethod(method);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      FailWithNetworkError();
      return rv;
    }

    
    nsAutoTArray<InternalHeaders::Entry, 5> headers;
    mRequest->Headers()->GetEntries(headers);
    for (uint32_t i = 0; i < headers.Length(); ++i) {
      httpChan->SetRequestHeader(headers[i].mName, headers[i].mValue, false );
    }

    
    nsAutoString referrer;
    mRequest->GetReferrer(referrer);
    
    MOZ_ASSERT(!referrer.EqualsLiteral(kFETCH_CLIENT_REFERRER_STR));
    if (!referrer.IsEmpty()) {
      nsCOMPtr<nsIURI> refURI;
      rv = NS_NewURI(getter_AddRefs(refURI), referrer, nullptr, nullptr);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return FailWithNetworkError();
      }

      net::ReferrerPolicy referrerPolicy = net::RP_Default;
      if (mDocument) {
        referrerPolicy = mDocument->GetReferrerPolicy();
      }

      rv = httpChan->SetReferrerWithPolicy(refURI, referrerPolicy);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return FailWithNetworkError();
      }
    }

    
    if (mRequest->ForceOriginHeader()) {
      nsAutoString origin;
      rv = nsContentUtils::GetUTFOrigin(mPrincipal, origin);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return FailWithNetworkError();
      }
      httpChan->SetRequestHeader(NS_LITERAL_CSTRING("origin"),
                                 NS_ConvertUTF16toUTF8(origin),
                                 false );
    }
    
    
    
    
  }

  
  
  

  
  nsCOMPtr<nsIUploadChannel2> uploadChan = do_QueryInterface(chan);
  if (uploadChan) {
    nsAutoCString contentType;
    ErrorResult result;
    mRequest->Headers()->Get(NS_LITERAL_CSTRING("content-type"), contentType, result);
    
    
    if (result.Failed()) {
      return FailWithNetworkError();
    }

    nsCOMPtr<nsIInputStream> bodyStream;
    mRequest->GetBody(getter_AddRefs(bodyStream));
    if (bodyStream) {
      nsAutoCString method;
      mRequest->GetMethod(method);
      rv = uploadChan->ExplicitSetUploadStream(bodyStream, contentType, -1, method, false );
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return FailWithNetworkError();
      }
    }
  }

  
  
  
  if (mRequest->SkipServiceWorker()) {
    nsCOMPtr<nsIHttpChannelInternal> internalChan = do_QueryInterface(httpChan);
    internalChan->ForceNoIntercept();
  }

  nsCOMPtr<nsIStreamListener> listener = this;

  
  
  
  if (mRequest->Mode() != RequestMode::No_cors) {
    
    
    
    
    
    nsRefPtr<nsCORSListenerProxy> corsListener =
      new nsCORSListenerProxy(this, mPrincipal, useCredentials);
    rv = corsListener->Init(chan, DataURIHandling::Allow);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return FailWithNetworkError();
    }
    listener = corsListener.forget();
  }

  
  
  
  
  
  if (aCORSPreflightFlag) {
    nsCOMPtr<nsIChannel> preflightChannel;
    nsAutoTArray<nsCString, 5> unsafeHeaders;
    mRequest->Headers()->GetUnsafeHeaders(unsafeHeaders);

    rv = NS_StartCORSPreflight(chan, listener, mPrincipal,
                               useCredentials,
                               unsafeHeaders,
                               getter_AddRefs(preflightChannel));
  } else {
   rv = chan->AsyncOpen(listener, nullptr);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return FailWithNetworkError();
  }

  
  return NS_OK;
}

nsresult
FetchDriver::ContinueHttpFetchAfterNetworkFetch()
{
  workers::AssertIsOnMainThread();
  MOZ_ASSERT(mResponse);
  MOZ_ASSERT(!mResponse->IsError());

  return SucceedWithResponse();
}

already_AddRefed<InternalResponse>
FetchDriver::BeginAndGetFilteredResponse(InternalResponse* aResponse)
{
  MOZ_ASSERT(aResponse);
  if (!aResponse->FinalURL()) {
    nsAutoCString reqURL;
    mRequest->GetURL(reqURL);
    aResponse->SetUrl(reqURL);
  }

  

  nsRefPtr<InternalResponse> filteredResponse;
  switch (mRequest->GetResponseTainting()) {
    case InternalRequest::RESPONSETAINT_BASIC:
      filteredResponse = aResponse->BasicResponse();
      break;
    case InternalRequest::RESPONSETAINT_CORS:
      filteredResponse = aResponse->CORSResponse();
      break;
    case InternalRequest::RESPONSETAINT_OPAQUE:
      filteredResponse = aResponse->OpaqueResponse();
      break;
    default:
      MOZ_CRASH("Unexpected case");
  }

  MOZ_ASSERT(filteredResponse);
  MOZ_ASSERT(mObserver);
  mObserver->OnResponseAvailable(filteredResponse);
  mResponseAvailableCalled = true;
  return filteredResponse.forget();
}

void
FetchDriver::BeginResponse(InternalResponse* aResponse)
{
  nsRefPtr<InternalResponse> r = BeginAndGetFilteredResponse(aResponse);
  
}

nsresult
FetchDriver::SucceedWithResponse()
{
  workers::AssertIsOnMainThread();
  if (mObserver) {
    mObserver->OnResponseEnd();
    mObserver = nullptr;
  }
  return NS_OK;
}

nsresult
FetchDriver::FailWithNetworkError()
{
  workers::AssertIsOnMainThread();
  nsRefPtr<InternalResponse> error = InternalResponse::NetworkError();
  if (mObserver) {
    mObserver->OnResponseAvailable(error);
    mResponseAvailableCalled = true;
    mObserver->OnResponseEnd();
    mObserver = nullptr;
  }
  return NS_OK;
}

namespace {
class FillResponseHeaders final : public nsIHttpHeaderVisitor {
  InternalResponse* mResponse;

  ~FillResponseHeaders()
  { }
public:
  NS_DECL_ISUPPORTS

  explicit FillResponseHeaders(InternalResponse* aResponse)
    : mResponse(aResponse)
  {
  }

  NS_IMETHOD
  VisitHeader(const nsACString & aHeader, const nsACString & aValue) override
  {
    ErrorResult result;
    mResponse->Headers()->Append(aHeader, aValue, result);
    return result.ErrorCode();
  }
};

NS_IMPL_ISUPPORTS(FillResponseHeaders, nsIHttpHeaderVisitor)
} 

NS_IMETHODIMP
FetchDriver::OnStartRequest(nsIRequest* aRequest,
                            nsISupports* aContext)
{
  workers::AssertIsOnMainThread();
  MOZ_ASSERT(!mPipeOutputStream);
  MOZ_ASSERT(mObserver);
  nsresult rv;
  aRequest->GetStatus(&rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    return rv;
  }

  nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aRequest);
  
  MOZ_ASSERT(channel);

  aRequest->GetStatus(&rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    return rv;
  }

  uint32_t responseStatus;
  channel->GetResponseStatus(&responseStatus);

  nsAutoCString statusText;
  channel->GetResponseStatusText(statusText);

  nsRefPtr<InternalResponse> response = new InternalResponse(responseStatus, statusText);

  nsRefPtr<FillResponseHeaders> visitor = new FillResponseHeaders(response);
  rv = channel->VisitResponseHeaders(visitor);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    NS_WARNING("Failed to visit all headers.");
  }

  
  
  
  
  
  
  nsCOMPtr<nsIInputStream> pipeInputStream;
  rv = NS_NewPipe(getter_AddRefs(pipeInputStream),
                  getter_AddRefs(mPipeOutputStream),
                  0, 
                  UINT32_MAX ,
                  true ,
                  false  );
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    
    return rv;
  }
  response->SetBody(pipeInputStream);

  nsCOMPtr<nsISupports> securityInfo;
  rv = channel->GetSecurityInfo(getter_AddRefs(securityInfo));
  if (securityInfo) {
    response->SetSecurityInfo(securityInfo);
  }

  
  
  mResponse = BeginAndGetFilteredResponse(response);

  nsCOMPtr<nsIEventTarget> sts = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailWithNetworkError();
    
    return rv;
  }

  
  nsCOMPtr<nsIThreadRetargetableRequest> rr = do_QueryInterface(aRequest);
  if (rr) {
    rr->RetargetDeliveryTo(sts);
  }
  return NS_OK;
}

NS_IMETHODIMP
FetchDriver::OnDataAvailable(nsIRequest* aRequest,
                             nsISupports* aContext,
                             nsIInputStream* aInputStream,
                             uint64_t aOffset,
                             uint32_t aCount)
{
  uint32_t aRead;
  MOZ_ASSERT(mResponse);
  MOZ_ASSERT(mPipeOutputStream);

  nsresult rv = aInputStream->ReadSegments(NS_CopySegmentToStream,
                                           mPipeOutputStream,
                                           aCount, &aRead);
  return rv;
}

NS_IMETHODIMP
FetchDriver::OnStopRequest(nsIRequest* aRequest,
                           nsISupports* aContext,
                           nsresult aStatusCode)
{
  workers::AssertIsOnMainThread();
  if (mPipeOutputStream) {
    mPipeOutputStream->Close();
  }

  if (NS_FAILED(aStatusCode)) {
    FailWithNetworkError();
    return aStatusCode;
  }

  ContinueHttpFetchAfterNetworkFetch();
  return NS_OK;
}


NS_IMETHODIMP
FetchDriver::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                    nsIChannel* aNewChannel,
                                    uint32_t aFlags,
                                    nsIAsyncVerifyRedirectCallback *aCallback)
{
  NS_PRECONDITION(aNewChannel, "Redirect without a channel?");

  nsresult rv;

  
  
  
  
  
  
  mRequest->UnsetSameOriginDataURL();

  
  
  
  
  
  
  
  
  
  
  if (!NS_IsInternalSameURIRedirect(aOldChannel, aNewChannel, aFlags)) {
    rv = DoesNotRequirePreflight(aNewChannel);
    if (NS_FAILED(rv)) {
      NS_WARNING("FetchDriver::OnChannelRedirect: "
                 "DoesNotRequirePreflight returned failure");
      return rv;
    }
  }

  mRedirectCallback = aCallback;
  mOldRedirectChannel = aOldChannel;
  mNewRedirectChannel = aNewChannel;

  nsCOMPtr<nsIChannelEventSink> outer =
    do_GetInterface(mNotificationCallbacks);
  if (outer) {
    
    
    
    rv = outer->AsyncOnChannelRedirect(aOldChannel, aNewChannel, aFlags, this);
    if (NS_FAILED(rv)) {
      aOldChannel->Cancel(rv);
      mRedirectCallback = nullptr;
      mOldRedirectChannel = nullptr;
      mNewRedirectChannel = nullptr;
    }
    return rv;
  }

  (void) OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}


nsresult
FetchDriver::DoesNotRequirePreflight(nsIChannel* aChannel)
{
  
  
  if (nsContentUtils::CheckMayLoad(mPrincipal, aChannel, true)) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  NS_ENSURE_TRUE(httpChannel, NS_ERROR_DOM_BAD_URI);

  nsAutoCString method;
  httpChannel->GetRequestMethod(method);
  if (mRequest->Mode() == RequestMode::Cors_with_forced_preflight ||
      !mRequest->Headers()->HasOnlySimpleHeaders() ||
      (!method.LowerCaseEqualsLiteral("get") &&
       !method.LowerCaseEqualsLiteral("post") &&
       !method.LowerCaseEqualsLiteral("head"))) {
    return NS_ERROR_DOM_BAD_URI;
  }

  return NS_OK;
}

NS_IMETHODIMP
FetchDriver::GetInterface(const nsIID& aIID, void **aResult)
{
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    *aResult = static_cast<nsIChannelEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  nsresult rv;

  if (mNotificationCallbacks) {
    rv = mNotificationCallbacks->GetInterface(aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(*aResult, "Lying nsIInterfaceRequestor implementation!");
      return rv;
    }
  }
  else if (aIID.Equals(NS_GET_IID(nsIStreamListener))) {
    *aResult = static_cast<nsIStreamListener*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if (aIID.Equals(NS_GET_IID(nsIRequestObserver))) {
    *aResult = static_cast<nsIRequestObserver*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
FetchDriver::OnRedirectVerifyCallback(nsresult aResult)
{
  
  
  if (NS_SUCCEEDED(aResult)) {
    
    
    
    nsCOMPtr<nsIURI> newURI;
    nsresult rv = NS_GetFinalChannelURI(mNewRedirectChannel, getter_AddRefs(newURI));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      aResult = rv;
    } else {
      
      nsAutoCString newUrl;
      newURI->GetSpec(newUrl);
      mRequest->SetURL(newUrl);
    }
  }

  if (NS_FAILED(aResult)) {
    mOldRedirectChannel->Cancel(aResult);
  }

  mOldRedirectChannel = nullptr;
  mNewRedirectChannel = nullptr;
  mRedirectCallback->OnRedirectVerifyCallback(aResult);
  mRedirectCallback = nullptr;
  return NS_OK;
}

void
FetchDriver::SetDocument(nsIDocument* aDocument)
{
  
  MOZ_ASSERT(mFetchRecursionCount == 0);
  mDocument = aDocument;
}
} 
} 
