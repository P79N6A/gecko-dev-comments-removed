






#include "mozilla/net/FTPChannelParent.h"
#include "nsFTPChannel.h"
#include "nsNetUtil.h"
#include "nsFtpProtocolHandler.h"
#include "nsIEncodedChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIForcePendingChannel.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "mozilla/unused.h"
#include "SerializedLoadContext.h"
#include "nsIContentPolicy.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "nsIOService.h"
#include "mozilla/LoadInfo.h"

using namespace mozilla::ipc;

#undef LOG
#define LOG(args) PR_LOG(gFTPLog, PR_LOG_DEBUG, args)

namespace mozilla {
namespace net {

FTPChannelParent::FTPChannelParent(nsILoadContext* aLoadContext, PBOverrideStatus aOverrideStatus)
  : mIPCClosed(false)
  , mLoadContext(aLoadContext)
  , mPBOverride(aOverrideStatus)
  , mStatus(NS_OK)
  , mDivertingFromChild(false)
  , mDivertedOnStartRequest(false)
  , mSuspendedForDiversion(false)
{
  nsIProtocolHandler* handler;
  CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "ftp", &handler);
  NS_ASSERTION(handler, "no ftp handler");
  
  mObserver = new OfflineObserver(this);
}

FTPChannelParent::~FTPChannelParent()
{
  gFtpHandler->Release();
  if (mObserver) {
    mObserver->RemoveObserver();
  }
}

void
FTPChannelParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  mIPCClosed = true;
}





NS_IMPL_ISUPPORTS(FTPChannelParent,
                  nsIStreamListener,
                  nsIParentChannel,
                  nsIInterfaceRequestor,
                  nsIRequestObserver,
                  nsIChannelEventSink)









bool
FTPChannelParent::Init(const FTPChannelCreationArgs& aArgs)
{
  switch (aArgs.type()) {
  case FTPChannelCreationArgs::TFTPChannelOpenArgs:
  {
    const FTPChannelOpenArgs& a = aArgs.get_FTPChannelOpenArgs();
    return DoAsyncOpen(a.uri(), a.startPos(), a.entityID(), a.uploadStream(),
                       a.requestingPrincipalInfo(), a.triggeringPrincipalInfo(),
                       a.securityFlags(), a.contentPolicyType(), a.innerWindowID());
  }
  case FTPChannelCreationArgs::TFTPChannelConnectArgs:
  {
    const FTPChannelConnectArgs& cArgs = aArgs.get_FTPChannelConnectArgs();
    return ConnectChannel(cArgs.channelId());
  }
  default:
    NS_NOTREACHED("unknown open type");
    return false;
  }
}

bool
FTPChannelParent::DoAsyncOpen(const URIParams& aURI,
                              const uint64_t& aStartPos,
                              const nsCString& aEntityID,
                              const OptionalInputStreamParams& aUploadStream,
                              const ipc::PrincipalInfo& aRequestingPrincipalInfo,
                              const ipc::PrincipalInfo& aTriggeringPrincipalInfo,
                              const uint32_t& aSecurityFlags,
                              const uint32_t& aContentPolicyType,
                              const uint32_t& aInnerWindowID)
{
  nsCOMPtr<nsIURI> uri = DeserializeURI(aURI);
  if (!uri)
      return false;

#ifdef DEBUG
  nsCString uriSpec;
  uri->GetSpec(uriSpec);
  LOG(("FTPChannelParent DoAsyncOpen [this=%p uri=%s]\n",
       this, uriSpec.get()));
#endif

  bool app_offline = false;
  uint32_t appId = GetAppId();
  if (appId != NECKO_UNKNOWN_APP_ID &&
      appId != NECKO_NO_APP_ID) {
    gIOService->IsAppOffline(appId, &app_offline);
    LOG(("FTP app id %u is offline %d\n", appId, app_offline));
  }

  if (app_offline)
    return SendFailedAsyncOpen(NS_ERROR_OFFLINE);

  nsresult rv;
  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  nsCOMPtr<nsIPrincipal> requestingPrincipal =
    mozilla::ipc::PrincipalInfoToPrincipal(aRequestingPrincipalInfo, &rv);
  if (NS_FAILED(rv)) {
    return SendFailedAsyncOpen(rv);
  }
  nsCOMPtr<nsIPrincipal> triggeringPrincipal =
    mozilla::ipc::PrincipalInfoToPrincipal(aTriggeringPrincipalInfo, &rv);
  if (NS_FAILED(rv)) {
    return SendFailedAsyncOpen(rv);
  }

  nsCOMPtr<nsILoadInfo> loadInfo =
    new mozilla::LoadInfo(requestingPrincipal, triggeringPrincipal,
                          aSecurityFlags, aContentPolicyType,
                          aInnerWindowID);

  nsCOMPtr<nsIChannel> chan;
  rv = NS_NewChannelInternal(getter_AddRefs(chan), uri, loadInfo,
                             nullptr, nullptr,
                             nsIRequest::LOAD_NORMAL, ios);

  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  mChannel = chan;

  
  
  nsFtpChannel* ftpChan = static_cast<nsFtpChannel*>(mChannel.get());

  if (mPBOverride != kPBOverride_Unset) {
    ftpChan->SetPrivate(mPBOverride == kPBOverride_Private ? true : false);
  }
  rv = ftpChan->SetNotificationCallbacks(this);
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  nsTArray<mozilla::ipc::FileDescriptor> fds;
  nsCOMPtr<nsIInputStream> upload = DeserializeInputStream(aUploadStream, fds);
  if (upload) {
    
    rv = ftpChan->SetUploadStream(upload, EmptyCString(), 0);
    if (NS_FAILED(rv))
      return SendFailedAsyncOpen(rv);
  }

  rv = ftpChan->ResumeAt(aStartPos, aEntityID);
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  rv = ftpChan->AsyncOpen(this, nullptr);
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);
  
  return true;
}

bool
FTPChannelParent::ConnectChannel(const uint32_t& channelId)
{
  nsresult rv;

  LOG(("Looking for a registered channel [this=%p, id=%d]", this, channelId));

  nsCOMPtr<nsIChannel> channel;
  rv = NS_LinkRedirectChannels(channelId, this, getter_AddRefs(channel));
  if (NS_SUCCEEDED(rv))
    mChannel = channel;

  LOG(("  found channel %p, rv=%08x", mChannel.get(), rv));

  return true;
}

bool
FTPChannelParent::RecvCancel(const nsresult& status)
{
  if (mChannel)
    mChannel->Cancel(status);

  return true;
}

bool
FTPChannelParent::RecvSuspend()
{
  if (mChannel)
    mChannel->Suspend();
  return true;
}

bool
FTPChannelParent::RecvResume()
{
  if (mChannel)
    mChannel->Resume();
  return true;
}

bool
FTPChannelParent::RecvDivertOnDataAvailable(const nsCString& data,
                                            const uint64_t& offset,
                                            const uint32_t& count)
{
  if (NS_WARN_IF(!mDivertingFromChild)) {
    MOZ_ASSERT(mDivertingFromChild,
               "Cannot RecvDivertOnDataAvailable if diverting is not set!");
    FailDiversion(NS_ERROR_UNEXPECTED);
    return false;
  }

  
  if (NS_FAILED(mStatus)) {
    return true;
  }

  nsCOMPtr<nsIInputStream> stringStream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stringStream), data.get(),
                                      count, NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv)) {
    if (mChannel) {
      mChannel->Cancel(rv);
    }
    mStatus = rv;
    return true;
  }

  rv = OnDataAvailable(mChannel, nullptr, stringStream, offset, count);

  stringStream->Close();
  if (NS_FAILED(rv)) {
    if (mChannel) {
      mChannel->Cancel(rv);
    }
    mStatus = rv;
  }
  return true;
}

bool
FTPChannelParent::RecvDivertOnStopRequest(const nsresult& statusCode)
{
  if (NS_WARN_IF(!mDivertingFromChild)) {
    MOZ_ASSERT(mDivertingFromChild,
               "Cannot RecvDivertOnStopRequest if diverting is not set!");
    FailDiversion(NS_ERROR_UNEXPECTED);
    return false;
  }

  
  nsresult status = NS_FAILED(mStatus) ? mStatus : statusCode;

  
  if (mChannel) {
    nsCOMPtr<nsIForcePendingChannel> forcePendingIChan = do_QueryInterface(mChannel);
    if (forcePendingIChan) {
      forcePendingIChan->ForcePending(false);
    }
  }

  OnStopRequest(mChannel, nullptr, status);
  return true;
}

bool
FTPChannelParent::RecvDivertComplete()
{
  if (NS_WARN_IF(!mDivertingFromChild)) {
    MOZ_ASSERT(mDivertingFromChild,
               "Cannot RecvDivertComplete if diverting is not set!");
    FailDiversion(NS_ERROR_UNEXPECTED);
    return false;
  }

  nsresult rv = ResumeForDiversion();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    FailDiversion(NS_ERROR_UNEXPECTED);
    return false;
  }

  return true;
}





NS_IMETHODIMP
FTPChannelParent::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  LOG(("FTPChannelParent::OnStartRequest [this=%p]\n", this));

  if (mDivertingFromChild) {
    MOZ_RELEASE_ASSERT(mDivertToListener,
                       "Cannot divert if listener is unset!");
    return mDivertToListener->OnStartRequest(aRequest, aContext);
  }

  nsCOMPtr<nsIChannel> chan = do_QueryInterface(aRequest);
  MOZ_ASSERT(chan);
  NS_ENSURE_TRUE(chan, NS_ERROR_UNEXPECTED);

  int64_t contentLength;
  chan->GetContentLength(&contentLength);
  nsCString contentType;
  chan->GetContentType(contentType);

  nsCString entityID;
  nsCOMPtr<nsIResumableChannel> resChan = do_QueryInterface(aRequest);
  MOZ_ASSERT(resChan); 
  if (resChan) {
    resChan->GetEntityID(entityID);
  }

  PRTime lastModified = 0;
  nsCOMPtr<nsIFTPChannel> ftpChan = do_QueryInterface(aRequest);
  if (ftpChan) {
    ftpChan->GetLastModifiedTime(&lastModified);
  }
  nsCOMPtr<nsIHttpChannelInternal> httpChan = do_QueryInterface(aRequest);
  if (httpChan) {
    httpChan->GetLastModifiedTime(&lastModified);
  }

  URIParams uriparam;
  nsCOMPtr<nsIURI> uri;
  chan->GetURI(getter_AddRefs(uri));
  SerializeURI(uri, uriparam);

  if (mIPCClosed || !SendOnStartRequest(mStatus, contentLength, contentType,
                                        lastModified, entityID, uriparam)) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

NS_IMETHODIMP
FTPChannelParent::OnStopRequest(nsIRequest* aRequest,
                                nsISupports* aContext,
                                nsresult aStatusCode)
{
  LOG(("FTPChannelParent::OnStopRequest: [this=%p status=%ul]\n",
       this, aStatusCode));

  if (mDivertingFromChild) {
    MOZ_RELEASE_ASSERT(mDivertToListener,
                       "Cannot divert if listener is unset!");
    return mDivertToListener->OnStopRequest(aRequest, aContext, aStatusCode);
  }

  if (mIPCClosed || !SendOnStopRequest(aStatusCode)) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}





NS_IMETHODIMP
FTPChannelParent::OnDataAvailable(nsIRequest* aRequest,
                                  nsISupports* aContext,
                                  nsIInputStream* aInputStream,
                                  uint64_t aOffset,
                                  uint32_t aCount)
{
  LOG(("FTPChannelParent::OnDataAvailable [this=%p]\n", this));

  if (mDivertingFromChild) {
    MOZ_RELEASE_ASSERT(mDivertToListener,
                       "Cannot divert if listener is unset!");
    return mDivertToListener->OnDataAvailable(aRequest, aContext, aInputStream,
                                              aOffset, aCount);
  }

  nsCString data;
  nsresult rv = NS_ReadInputStreamToString(aInputStream, data, aCount);
  if (NS_FAILED(rv))
    return rv;

  if (mIPCClosed || !SendOnDataAvailable(mStatus, data, aOffset, aCount))
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}





NS_IMETHODIMP
FTPChannelParent::SetParentListener(HttpChannelParentListener* aListener)
{
  
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelParent::NotifyTrackingProtectionDisabled()
{
  
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelParent::Delete()
{
  if (mIPCClosed || !SendDeleteSelf())
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}





NS_IMETHODIMP
FTPChannelParent::GetInterface(const nsIID& uuid, void** result)
{
  
  if (uuid.Equals(NS_GET_IID(nsILoadContext)) && mLoadContext) {
    NS_ADDREF(mLoadContext);
    *result = static_cast<nsILoadContext*>(mLoadContext);
    return NS_OK;
  }

  return QueryInterface(uuid, result);
}




nsresult
FTPChannelParent::SuspendForDiversion()
{
  MOZ_ASSERT(mChannel);
  if (NS_WARN_IF(mDivertingFromChild)) {
    MOZ_ASSERT(!mDivertingFromChild, "Already suspended for diversion!");
    return NS_ERROR_UNEXPECTED;
  }

  
  
  nsresult rv = mChannel->Suspend();
  MOZ_ASSERT(NS_SUCCEEDED(rv) || rv == NS_ERROR_NOT_AVAILABLE);
  mSuspendedForDiversion = NS_SUCCEEDED(rv);

  
  
  mDivertingFromChild = true;

  return NS_OK;
}


nsresult
FTPChannelParent::ResumeForDiversion()
{
  MOZ_ASSERT(mChannel);
  MOZ_ASSERT(mDivertToListener);
  if (NS_WARN_IF(!mDivertingFromChild)) {
    MOZ_ASSERT(mDivertingFromChild,
               "Cannot ResumeForDiversion if not diverting!");
    return NS_ERROR_UNEXPECTED;
  }

  if (mSuspendedForDiversion) {
    nsresult rv = mChannel->Resume();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      FailDiversion(NS_ERROR_UNEXPECTED, true);
      return rv;
    }
    mSuspendedForDiversion = false;
  }

  
  
  if (NS_WARN_IF(NS_FAILED(Delete()))) {
    FailDiversion(NS_ERROR_UNEXPECTED);
    return NS_ERROR_UNEXPECTED;   
  }
  return NS_OK;
}

void
FTPChannelParent::DivertTo(nsIStreamListener *aListener)
{
  MOZ_ASSERT(aListener);
  if (NS_WARN_IF(!mDivertingFromChild)) {
    MOZ_ASSERT(mDivertingFromChild,
               "Cannot DivertTo new listener if diverting is not set!");
    return;
  }

  if (NS_WARN_IF(mIPCClosed || !SendFlushedForDiversion())) {
    FailDiversion(NS_ERROR_UNEXPECTED);
    return;
  }

  mDivertToListener = aListener;

  
  
  NS_DispatchToCurrentThread(
    NS_NewRunnableMethod(this, &FTPChannelParent::StartDiversion));
  return;
}

void
FTPChannelParent::StartDiversion()
{
  if (NS_WARN_IF(!mDivertingFromChild)) {
    MOZ_ASSERT(mDivertingFromChild,
               "Cannot StartDiversion if diverting is not set!");
    return;
  }

  
  if (mChannel) {
    nsCOMPtr<nsIForcePendingChannel> forcePendingIChan = do_QueryInterface(mChannel);
    if (forcePendingIChan) {
      forcePendingIChan->ForcePending(true);
    }
  }

  
  nsresult rv = OnStartRequest(mChannel, nullptr);
  if (NS_FAILED(rv)) {
    if (mChannel) {
      mChannel->Cancel(rv);
    }
    mStatus = rv;
    return;
  }

  
  
  if (NS_WARN_IF(mIPCClosed || !SendDivertMessages())) {
    FailDiversion(NS_ERROR_UNEXPECTED);
    return;
  }
}

class FTPFailDiversionEvent : public nsRunnable
{
public:
  FTPFailDiversionEvent(FTPChannelParent *aChannelParent,
                        nsresult aErrorCode,
                        bool aSkipResume)
    : mChannelParent(aChannelParent)
    , mErrorCode(aErrorCode)
    , mSkipResume(aSkipResume)
  {
    MOZ_RELEASE_ASSERT(aChannelParent);
    MOZ_RELEASE_ASSERT(NS_FAILED(aErrorCode));
  }
  NS_IMETHOD Run()
  {
    mChannelParent->NotifyDiversionFailed(mErrorCode, mSkipResume);
    return NS_OK;
  }
private:
  nsRefPtr<FTPChannelParent> mChannelParent;
  nsresult mErrorCode;
  bool mSkipResume;
};

void
FTPChannelParent::FailDiversion(nsresult aErrorCode,
                                            bool aSkipResume)
{
  MOZ_RELEASE_ASSERT(NS_FAILED(aErrorCode));
  MOZ_RELEASE_ASSERT(mDivertingFromChild);
  MOZ_RELEASE_ASSERT(mDivertToListener);
  MOZ_RELEASE_ASSERT(mChannel);

  NS_DispatchToCurrentThread(
    new FTPFailDiversionEvent(this, aErrorCode, aSkipResume));
}

void
FTPChannelParent::NotifyDiversionFailed(nsresult aErrorCode,
                                        bool aSkipResume)
{
  MOZ_RELEASE_ASSERT(NS_FAILED(aErrorCode));
  MOZ_RELEASE_ASSERT(mDivertingFromChild);
  MOZ_RELEASE_ASSERT(mDivertToListener);
  MOZ_RELEASE_ASSERT(mChannel);

  mChannel->Cancel(aErrorCode);
  nsCOMPtr<nsIForcePendingChannel> forcePendingIChan = do_QueryInterface(mChannel);
  if (forcePendingIChan) {
    forcePendingIChan->ForcePending(false);
  }

  bool isPending = false;
  nsresult rv = mChannel->IsPending(&isPending);
  MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));

  
  if (mSuspendedForDiversion) {
    mChannel->Resume();
  }
  
  
  if (!mDivertedOnStartRequest) {
    nsCOMPtr<nsIForcePendingChannel> forcePendingIChan = do_QueryInterface(mChannel);
    if (forcePendingIChan) {
      forcePendingIChan->ForcePending(true);
    }
    mDivertToListener->OnStartRequest(mChannel, nullptr);

    if (forcePendingIChan) {
      forcePendingIChan->ForcePending(false);
    }
  }
  
  
  if (!isPending) {
    mDivertToListener->OnStopRequest(mChannel, nullptr, aErrorCode);
  }
  mDivertToListener = nullptr;
  mChannel = nullptr;

  if (!mIPCClosed) {
    unused << SendDeleteSelf();
  }
}

void
FTPChannelParent::OfflineDisconnect()
{
  if (mChannel) {
    mChannel->Cancel(NS_ERROR_OFFLINE);
  }
  mStatus = NS_ERROR_OFFLINE;
}

uint32_t
FTPChannelParent::GetAppId()
{
  uint32_t appId = NECKO_UNKNOWN_APP_ID;
  if (mLoadContext) {
    mLoadContext->GetAppId(&appId);
  }
  return appId;
}





NS_IMETHODIMP
FTPChannelParent::AsyncOnChannelRedirect(
                            nsIChannel *oldChannel,
                            nsIChannel *newChannel,
                            uint32_t redirectFlags,
                            nsIAsyncVerifyRedirectCallback* callback)
{
  nsCOMPtr<nsIFTPChannel> ftpChan = do_QueryInterface(newChannel);
  if (!ftpChan) {
    
    nsCOMPtr<nsIHttpChannel> httpChan = do_QueryInterface(newChannel);
    if (!httpChan)
      return NS_ERROR_UNEXPECTED; 
  }
  mChannel = newChannel;
  callback->OnRedirectVerifyCallback(NS_OK);
  return NS_OK; 
}


} 
} 

