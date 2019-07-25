







































#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/unused.h"
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
#include "nsIRedirectChannelRegistrar.h"

namespace mozilla {
namespace net {

HttpChannelParent::HttpChannelParent(PBrowserParent* iframeEmbedding)
  : mIPCClosed(false)
  , mStoredStatus(0)
  , mStoredProgress(0)
  , mStoredProgressMax(0)
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





NS_IMPL_ISUPPORTS6(HttpChannelParent,
                   nsIInterfaceRequestor,
                   nsIProgressEventSink,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIParentChannel,
                   nsIParentRedirectingChannel)





NS_IMETHODIMP
HttpChannelParent::GetInterface(const nsIID& aIID, void **result)
{
  if (aIID.Equals(NS_GET_IID(nsIAuthPromptProvider)) ||
      aIID.Equals(NS_GET_IID(nsISecureBrowserUI))) {
    if (!mTabParent)
      return NS_NOINTERFACE;

    return mTabParent->QueryInterface(aIID, result);
  }

  return QueryInterface(aIID, result);
}





bool 
HttpChannelParent::RecvAsyncOpen(const IPC::URI&            aURI,
                                 const IPC::URI&            aOriginalURI,
                                 const IPC::URI&            aDocURI,
                                 const IPC::URI&            aReferrerURI,
                                 const PRUint32&            loadFlags,
                                 const RequestHeaderTuples& requestHeaders,
                                 const nsHttpAtom&          requestMethod,
                                 const IPC::InputStream&    uploadStream,
                                 const PRBool&              uploadStreamHasHeaders,
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

  nsRefPtr<HttpChannelParentListener> channelListener =
      new HttpChannelParentListener(this);

  httpChan->SetNotificationCallbacks(channelListener);

  httpChan->SetRequestMethod(nsDependentCString(requestMethod.get()));

  nsCOMPtr<nsIInputStream> stream(uploadStream);
  if (stream) {
    httpChan->InternalSetUploadStream(stream);
    httpChan->SetUploadStreamHasHeaders(uploadStreamHasHeaders);
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

  rv = httpChan->AsyncOpen(channelListener, nsnull);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  return true;
}

bool
HttpChannelParent::RecvConnectChannel(const PRUint32& channelId)
{
  nsresult rv;

  LOG(("Looking for a registered channel [this=%p, id=%d]", this, channelId));
  rv = NS_LinkRedirectChannels(channelId, this, getter_AddRefs(mChannel));
  LOG(("  found channel %p, rv=%08x", mChannel.get(), rv));

  return true;
}

bool 
HttpChannelParent::RecvSetPriority(const PRUint16& priority)
{
  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  httpChan->SetPriority(priority);

  nsCOMPtr<nsISupportsPriority> priorityRedirectChannel =
      do_QueryInterface(mRedirectChannel);
  if (priorityRedirectChannel)
    priorityRedirectChannel->SetPriority(priority);

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
HttpChannelParent::RecvRedirect2Verify(const nsresult& result, 
                                       const RequestHeaderTuples& changedHeaders)
{
  if (NS_SUCCEEDED(result)) {
    nsCOMPtr<nsIHttpChannel> newHttpChannel =
        do_QueryInterface(mRedirectChannel);

    if (newHttpChannel) {
      for (PRUint32 i = 0; i < changedHeaders.Length(); i++) {
        newHttpChannel->SetRequestHeader(changedHeaders[i].mHeader,
                                         changedHeaders[i].mValue,
                                         changedHeaders[i].mMerge);
      }
    }
  }

  mRedirectCallback->OnRedirectVerifyCallback(result);
  mRedirectCallback = nsnull;
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





NS_IMETHODIMP
HttpChannelParent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  LOG(("HttpChannelParent::OnStartRequest [this=%x]\n", this));

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

NS_IMETHODIMP
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





NS_IMETHODIMP
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

  
  
  
  
  if (mIPCClosed || !SendOnTransportAndData(mStoredStatus, mStoredProgress,
                                            mStoredProgressMax, data, aOffset,
                                            aCount)) {
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelParent::OnProgress(nsIRequest *aRequest, 
                              nsISupports *aContext, 
                              PRUint64 aProgress, 
                              PRUint64 aProgressMax)
{
  
  
  if (mStoredStatus == nsISocketTransport::STATUS_RECEIVING_FROM ||
      mStoredStatus == nsITransport::STATUS_READING)
  {
    mStoredProgress = aProgress;
    mStoredProgressMax = aProgressMax;
  } else {
    
    
    
    if (mIPCClosed || !SendOnProgress(aProgress, aProgressMax))
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::OnStatus(nsIRequest *aRequest, 
                            nsISupports *aContext, 
                            nsresult aStatus, 
                            const PRUnichar *aStatusArg)
{
  
  if (aStatus == nsISocketTransport::STATUS_RECEIVING_FROM ||
      aStatus == nsITransport::STATUS_READING)
  {
    mStoredStatus = aStatus;
    return NS_OK;
  }
  
  if (mIPCClosed || !SendOnStatus(aStatus))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelParent::Delete()
{
  if (!mIPCClosed)
    unused << SendDeleteSelf();

  return NS_OK;
}





NS_IMETHODIMP
HttpChannelParent::StartRedirect(PRUint32 newChannelId,
                                 nsIChannel* newChannel,
                                 PRUint32 redirectFlags,
                                 nsIAsyncVerifyRedirectCallback* callback)
{
  if (mIPCClosed)
    return NS_BINDING_ABORTED;

  nsCOMPtr<nsIURI> newURI;
  newChannel->GetURI(getter_AddRefs(newURI));

  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  nsHttpResponseHead *responseHead = httpChan->GetResponseHead();
  bool result = SendRedirect1Begin(newChannelId,
                                   IPC::URI(newURI),
                                   redirectFlags,
                                   responseHead ? *responseHead
                                                : nsHttpResponseHead());
  if (!result)
    return NS_BINDING_ABORTED;

  

  mRedirectChannel = newChannel;
  mRedirectCallback = callback;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::CompleteRedirect(PRBool succeeded)
{
  if (succeeded && !mIPCClosed) {
    
    unused << SendRedirect3Complete();
  }

  mRedirectChannel = nsnull;
  return NS_OK;
}

}} 
