






#include "mozilla/net/FTPChannelParent.h"
#include "nsFTPChannel.h"
#include "nsNetUtil.h"
#include "nsFtpProtocolHandler.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "SerializedLoadContext.h"

using namespace mozilla::ipc;

#undef LOG
#define LOG(args) PR_LOG(gFTPLog, PR_LOG_DEBUG, args)

namespace mozilla {
namespace net {

FTPChannelParent::FTPChannelParent(nsILoadContext* aLoadContext, PBOverrideStatus aOverrideStatus)
  : mIPCClosed(false)
  , mLoadContext(aLoadContext)
  , mPBOverride(aOverrideStatus)
{
  nsIProtocolHandler* handler;
  CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "ftp", &handler);
  NS_ASSERTION(handler, "no ftp handler");
}

FTPChannelParent::~FTPChannelParent()
{
  gFtpHandler->Release();
}

void
FTPChannelParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  mIPCClosed = true;
}





NS_IMPL_ISUPPORTS4(FTPChannelParent,
                   nsIStreamListener,
                   nsIParentChannel,
                   nsIInterfaceRequestor,
                   nsIRequestObserver)









bool
FTPChannelParent::Init(const FTPChannelCreationArgs& aArgs)
{
  switch (aArgs.type()) {
  case FTPChannelCreationArgs::TFTPChannelOpenArgs:
  {
    const FTPChannelOpenArgs& a = aArgs.get_FTPChannelOpenArgs();
    return DoAsyncOpen(a.uri(), a.startPos(), a.entityID(), a.uploadStream());
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
                              const OptionalInputStreamParams& aUploadStream)
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

  nsresult rv;
  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  nsCOMPtr<nsIChannel> chan;
  rv = NS_NewChannel(getter_AddRefs(chan), uri, ios);
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  mChannel = static_cast<nsFtpChannel*>(chan.get());

  if (mPBOverride != kPBOverride_Unset) {
    mChannel->SetPrivate(mPBOverride == kPBOverride_Private ? true : false);
  }

  rv = mChannel->SetNotificationCallbacks(this);
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  nsCOMPtr<nsIInputStream> upload = DeserializeInputStream(aUploadStream);
  if (upload) {
    
    rv = mChannel->SetUploadStream(upload, EmptyCString(), 0);
    if (NS_FAILED(rv))
      return SendFailedAsyncOpen(rv);
  }

  rv = mChannel->ResumeAt(aStartPos, aEntityID);
  if (NS_FAILED(rv))
    return SendFailedAsyncOpen(rv);

  rv = mChannel->AsyncOpen(this, nullptr);
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
    mChannel = static_cast<nsFtpChannel*>(channel.get());

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





NS_IMETHODIMP
FTPChannelParent::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  LOG(("FTPChannelParent::OnStartRequest [this=%p]\n", this));

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

  nsCOMPtr<nsIFTPChannel> ftpChan = do_QueryInterface(aRequest);
  PRTime lastModified = 0;
  if (ftpChan) {
    ftpChan->GetLastModifiedTime(&lastModified);
  } else {
    
    
    aRequest->Cancel(NS_ERROR_NOT_IMPLEMENTED);
  }

  URIParams uriparam;
  nsCOMPtr<nsIURI> uri;
  chan->GetURI(getter_AddRefs(uri));
  SerializeURI(uri, uriparam);

  if (mIPCClosed || !SendOnStartRequest(contentLength, contentType,
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
  
  nsCString data;
  nsresult rv = NS_ReadInputStreamToString(aInputStream, data, aCount);
  if (NS_FAILED(rv))
    return rv;

  if (mIPCClosed || !SendOnDataAvailable(data, aOffset, aCount))
    return NS_ERROR_UNEXPECTED;

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


} 
} 

