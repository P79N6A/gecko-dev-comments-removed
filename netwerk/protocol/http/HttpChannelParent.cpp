








































#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/net/NeckoParent.h"
#include "HttpChannelParentListener.h"
#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsNetUtil.h"
#include "nsISupportsPriority.h"
#include "nsIAuthPromptProvider.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBadCertListener2.h"
#include "nsICacheEntryDescriptor.h"
#include "nsSerializationHelper.h"
#include "nsISerializable.h"
#include "nsIAssociatedContentSecurity.h"
#include "nsIApplicationCacheService.h"
#include "nsIOfflineCacheUpdate.h"

namespace mozilla {
namespace net {

HttpChannelParent::HttpChannelParent(PBrowserParent* iframeEmbedding)
: mIPCClosed(false)
{
  
  nsIHttpProtocolHandler* handler;
  CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &handler);
  NS_ASSERTION(handler, "no http handler");

  mTabParent = do_QueryInterface(static_cast<nsITabParent*>(
      static_cast<TabParent*>(iframeEmbedding)));
}

HttpChannelParent::~HttpChannelParent()
{
  gHttpHandler->Release();
}

void
HttpChannelParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  mIPCClosed = true;
}





NS_IMPL_ISUPPORTS1(HttpChannelParent,
                   nsIProgressEventSink)





bool 
HttpChannelParent::RecvAsyncOpen(const IPC::URI&            aURI,
                                 const IPC::URI&            aOriginalURI,
                                 const IPC::URI&            aDocURI,
                                 const IPC::URI&            aReferrerURI,
                                 const PRUint32&            loadFlags,
                                 const RequestHeaderTuples& requestHeaders,
                                 const nsHttpAtom&          requestMethod,
                                 const nsCString&           uploadStreamData,
                                 const PRInt32&             uploadStreamInfo,
                                 const PRUint16&            priority,
                                 const PRUint8&             redirectionLimit,
                                 const PRBool&              allowPipelining,
                                 const PRBool&              forceAllowThirdPartyCookie,
                                 const bool&                doResumeAt,
                                 const PRUint64&            startPos,
                                 const nsCString&           entityID,
                                 const bool&                chooseApplicationCache,
                                 const nsCString&           appCacheClientID)
{
  nsCOMPtr<nsIURI> uri(aURI);
  nsCOMPtr<nsIURI> originalUri(aOriginalURI);
  nsCOMPtr<nsIURI> docUri(aDocURI);
  nsCOMPtr<nsIURI> referrerUri(aReferrerURI);

  nsCString uriSpec;
  uri->GetSpec(uriSpec);
  LOG(("HttpChannelParent RecvAsyncOpen [this=%x uri=%s]\n", 
       this, uriSpec.get()));

  nsresult rv;

  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  rv = NS_NewChannel(getter_AddRefs(mChannel), uri, ios, nsnull, nsnull, loadFlags);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());

  if (doResumeAt)
    httpChan->ResumeAt(startPos, entityID);

  if (originalUri)
    httpChan->SetOriginalURI(originalUri);
  if (docUri)
    httpChan->SetDocumentURI(docUri);
  if (referrerUri)
    httpChan->SetReferrerInternal(referrerUri);
  if (loadFlags != nsIRequest::LOAD_NORMAL)
    httpChan->SetLoadFlags(loadFlags);

  for (PRUint32 i = 0; i < requestHeaders.Length(); i++) {
    httpChan->SetRequestHeader(requestHeaders[i].mHeader,
                               requestHeaders[i].mValue,
                               requestHeaders[i].mMerge);
  }

  mChannelListener = new HttpChannelParentListener(this);

  httpChan->SetNotificationCallbacks(mChannelListener);

  httpChan->SetRequestMethod(nsDependentCString(requestMethod.get()));

  if (uploadStreamInfo != eUploadStream_null) {
    nsCOMPtr<nsIInputStream> stream;
    rv = NS_NewPostDataStream(getter_AddRefs(stream), false, uploadStreamData, 0);
    if (NS_FAILED(rv))
      return SendCancelEarly(rv);

    httpChan->InternalSetUploadStream(stream);
    
    
    httpChan->SetUploadStreamHasHeaders((PRBool) uploadStreamInfo);
  }

  if (priority != nsISupportsPriority::PRIORITY_NORMAL)
    httpChan->SetPriority(priority);
  httpChan->SetRedirectionLimit(redirectionLimit);
  httpChan->SetAllowPipelining(allowPipelining);
  httpChan->SetForceAllowThirdPartyCookie(forceAllowThirdPartyCookie);

  nsCOMPtr<nsIApplicationCacheChannel> appCacheChan =
    do_QueryInterface(mChannel);
  nsCOMPtr<nsIApplicationCacheService> appCacheService =
    do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID);

  PRBool setChooseApplicationCache = chooseApplicationCache;
  if (appCacheChan && appCacheService) {
    
    
    
    appCacheChan->SetInheritApplicationCache(PR_FALSE);
    if (!appCacheClientID.IsEmpty()) {
      nsCOMPtr<nsIApplicationCache> appCache;
      rv = appCacheService->GetApplicationCache(appCacheClientID,
                                                getter_AddRefs(appCache));
      if (NS_SUCCEEDED(rv)) {
        appCacheChan->SetApplicationCache(appCache);
        setChooseApplicationCache = PR_FALSE;
      }
    }

    if (setChooseApplicationCache) {
      nsCOMPtr<nsIOfflineCacheUpdateService> offlineUpdateService =
        do_GetService("@mozilla.org/offlinecacheupdate-service;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        rv = offlineUpdateService->OfflineAppAllowedForURI(uri,
                                                           nsnull,
                                                           &setChooseApplicationCache);

        if (setChooseApplicationCache && NS_SUCCEEDED(rv))
          appCacheChan->SetChooseApplicationCache(PR_TRUE);
      }
    }
  }

  rv = httpChan->AsyncOpen(mChannelListener, nsnull);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  return true;
}

bool 
HttpChannelParent::RecvSetPriority(const PRUint16& priority)
{
  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  httpChan->SetPriority(priority);
  return true;
}

bool
HttpChannelParent::RecvSuspend()
{
  mChannel->Suspend();
  return true;
}

bool
HttpChannelParent::RecvResume()
{
  mChannel->Resume();
  return true;
}

bool
HttpChannelParent::RecvCancel(const nsresult& status)
{
  
  if (mChannel) {
    nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
    httpChan->Cancel(status);
  }
  return true;
}


bool
HttpChannelParent::RecvSetCacheTokenCachedCharset(const nsCString& charset)
{
  if (mCacheDescriptor)
    mCacheDescriptor->SetMetaDataElement("charset",
                                         PromiseFlatCString(charset).get());
  return true;
}

bool
HttpChannelParent::RecvUpdateAssociatedContentSecurity(const PRInt32& high,
                                                       const PRInt32& low,
                                                       const PRInt32& broken,
                                                       const PRInt32& no)
{
  nsHttpChannel *chan = static_cast<nsHttpChannel *>(mChannel.get());

  nsCOMPtr<nsISupports> secInfo;
  chan->GetSecurityInfo(getter_AddRefs(secInfo));

  nsCOMPtr<nsIAssociatedContentSecurity> assoc = do_QueryInterface(secInfo);
  if (!assoc)
    return true;

  assoc->SetCountSubRequestsHighSecurity(high);
  assoc->SetCountSubRequestsLowSecurity(low);
  assoc->SetCountSubRequestsBrokenSecurity(broken);
  assoc->SetCountSubRequestsNoSecurity(no);

  return true;
}

bool
HttpChannelParent::RecvRedirect2Result(const nsresult& result, 
                                       const RequestHeaderTuples& changedHeaders)
{
  if (mChannelListener)
    mChannelListener->OnContentRedirectResultReceived(result, changedHeaders);
  return true;
}

bool
HttpChannelParent::RecvDocumentChannelCleanup()
{
  
  
  mCacheDescriptor = 0;

  return true;
}

bool 
HttpChannelParent::RecvMarkOfflineCacheEntryAsForeign()
{
  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  httpChan->MarkOfflineCacheEntryAsForeign();
  return true;
}





nsresult
HttpChannelParent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  LOG(("HttpChannelParent::OnStartRequest [this=%x]\n", this));

  
  
  mChannelListener = nsnull;

  nsHttpChannel *chan = static_cast<nsHttpChannel *>(aRequest);
  nsHttpResponseHead *responseHead = chan->GetResponseHead();
  nsHttpRequestHead  *requestHead = chan->GetRequestHead();
  PRBool isFromCache = false;
  chan->IsFromCache(&isFromCache);
  PRUint32 expirationTime = nsICache::NO_EXPIRATION_TIME;
  chan->GetCacheTokenExpirationTime(&expirationTime);
  nsCString cachedCharset;
  chan->GetCacheTokenCachedCharset(cachedCharset);

  PRBool loadedFromApplicationCache;
  chan->GetLoadedFromApplicationCache(&loadedFromApplicationCache);
  if (loadedFromApplicationCache) {
    nsCOMPtr<nsIApplicationCache> appCache;
    chan->GetApplicationCache(getter_AddRefs(appCache));
    nsCString appCacheGroupId;
    nsCString appCacheClientId;
    appCache->GetGroupID(appCacheGroupId);
    appCache->GetClientID(appCacheClientId);
    if (mIPCClosed || 
        !SendAssociateApplicationCache(appCacheGroupId, appCacheClientId))
    {
      return NS_ERROR_UNEXPECTED;
    }
  }

  nsCOMPtr<nsIEncodedChannel> encodedChannel = do_QueryInterface(aRequest);
  if (encodedChannel)
    encodedChannel->SetApplyConversion(PR_FALSE);

  
  
  chan->GetCacheToken(getter_AddRefs(mCacheDescriptor));

  nsCString secInfoSerialization;
  nsCOMPtr<nsISupports> secInfoSupp;
  chan->GetSecurityInfo(getter_AddRefs(secInfoSupp));
  if (secInfoSupp) {
    nsCOMPtr<nsISerializable> secInfoSer = do_QueryInterface(secInfoSupp);
    if (secInfoSer)
      NS_SerializeToString(secInfoSer, secInfoSerialization);
  }

  RequestHeaderTuples headers;
  nsHttpHeaderArray harray = requestHead->Headers();

  for (PRUint32 i = 0; i < harray.Count(); i++) {
    RequestHeaderTuple* tuple = headers.AppendElement();
    tuple->mHeader = harray.Headers()[i].header;
    tuple->mValue  = harray.Headers()[i].value;
    tuple->mMerge  = false;
  }

  if (mIPCClosed || 
      !SendOnStartRequest(responseHead ? *responseHead : nsHttpResponseHead(), 
                          !!responseHead,
                          headers,
                          isFromCache,
                          mCacheDescriptor ? PR_TRUE : PR_FALSE,
                          expirationTime, cachedCharset, secInfoSerialization)) 
  {
    return NS_ERROR_UNEXPECTED; 
  }
  return NS_OK;
}

nsresult
HttpChannelParent::OnStopRequest(nsIRequest *aRequest, 
                                 nsISupports *aContext, 
                                 nsresult aStatusCode)
{
  LOG(("HttpChannelParent::OnStopRequest: [this=%x status=%ul]\n", 
       this, aStatusCode));

  if (mIPCClosed || !SendOnStopRequest(aStatusCode))
    return NS_ERROR_UNEXPECTED; 
  return NS_OK;
}

nsresult
HttpChannelParent::OnDataAvailable(nsIRequest *aRequest, 
                                   nsISupports *aContext, 
                                   nsIInputStream *aInputStream, 
                                   PRUint32 aOffset, 
                                   PRUint32 aCount)
{
  LOG(("HttpChannelParent::OnDataAvailable [this=%x]\n", this));
 
  nsCString data;
  nsresult rv = NS_ReadInputStreamToString(aInputStream, data, aCount);
  if (NS_FAILED(rv))
    return rv;

  if (mIPCClosed || !SendOnDataAvailable(data, aOffset, aCount))
    return NS_ERROR_UNEXPECTED; 

  return NS_OK;
}





NS_IMETHODIMP
HttpChannelParent::OnProgress(nsIRequest *aRequest, 
                              nsISupports *aContext, 
                              PRUint64 aProgress, 
                              PRUint64 aProgressMax)
{
  if (mIPCClosed || !SendOnProgress(aProgress, aProgressMax))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::OnStatus(nsIRequest *aRequest, 
                            nsISupports *aContext, 
                            nsresult aStatus, 
                            const PRUnichar *aStatusArg)
{
  if (mIPCClosed || !SendOnStatus(aStatus, nsString(aStatusArg)))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

}} 

