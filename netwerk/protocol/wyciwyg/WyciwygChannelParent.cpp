



#include "nsWyciwyg.h"

#include "mozilla/net/WyciwygChannelParent.h"
#include "nsWyciwygChannel.h"
#include "nsNetUtil.h"
#include "nsISupportsPriority.h"
#include "nsCharsetSource.h"
#include "nsISerializable.h"
#include "nsSerializationHelper.h"
#include "mozilla/ipc/URIUtils.h"
#include "mozilla/net/NeckoParent.h"

using namespace mozilla::ipc;

namespace mozilla {
namespace net {

WyciwygChannelParent::WyciwygChannelParent()
 : mIPCClosed(false)
{
#if defined(PR_LOGGING)
  if (!gWyciwygLog)
    gWyciwygLog = PR_NewLogModule("nsWyciwygChannel");
#endif
}

WyciwygChannelParent::~WyciwygChannelParent()
{
}

void
WyciwygChannelParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  mIPCClosed = true;
}





NS_IMPL_ISUPPORTS3(WyciwygChannelParent,
                   nsIStreamListener,
                   nsIInterfaceRequestor,
                   nsIRequestObserver)





bool
WyciwygChannelParent::RecvInit(const URIParams& aURI)
{
  nsresult rv;

  nsCOMPtr<nsIURI> uri = DeserializeURI(aURI);
  if (!uri)
    return false;

  nsCString uriSpec;
  uri->GetSpec(uriSpec);
  LOG(("WyciwygChannelParent RecvInit [this=%x uri=%s]\n",
       this, uriSpec.get()));

  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  nsCOMPtr<nsIChannel> chan;
  rv = NS_NewChannel(getter_AddRefs(chan), uri, ios);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  mChannel = do_QueryInterface(chan, &rv);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  return true;
}

bool
WyciwygChannelParent::RecvAsyncOpen(const URIParams& aOriginal,
                                    const uint32_t& aLoadFlags,
                                    const IPC::SerializedLoadContext& loadContext,
                                    PBrowserParent* aParent)
{
  nsCOMPtr<nsIURI> original = DeserializeURI(aOriginal);
  if (!original)
    return false;

  LOG(("WyciwygChannelParent RecvAsyncOpen [this=%x]\n", this));

  if (!mChannel)
    return true;

  nsresult rv;

  rv = mChannel->SetOriginalURI(original);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  rv = mChannel->SetLoadFlags(aLoadFlags);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  const char* error = NeckoParent::CreateChannelLoadContext(aParent, loadContext,
                                                            mLoadContext);
  if (error) {
    NS_WARNING(nsPrintfCString("WyciwygChannelParent::RecvAsyncOpen: error: %s\n",
                               error).get());
    return false;
  }

  if (!mLoadContext && loadContext.IsPrivateBitValid()) {
    nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel = do_QueryInterface(mChannel);
    if (pbChannel)
      pbChannel->SetPrivate(loadContext.mUsePrivateBrowsing);
  }

  rv = mChannel->SetNotificationCallbacks(this);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  rv = mChannel->AsyncOpen(this, nullptr);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  return true;
}

bool
WyciwygChannelParent::RecvWriteToCacheEntry(const nsString& data)
{
  if (mChannel)
    mChannel->WriteToCacheEntry(data);

  return true;
}

bool
WyciwygChannelParent::RecvCloseCacheEntry(const nsresult& reason)
{
  if (mChannel)
    mChannel->CloseCacheEntry(reason);

  return true;
}

bool
WyciwygChannelParent::RecvSetCharsetAndSource(const int32_t& aCharsetSource,
                                              const nsCString& aCharset)
{
  if (mChannel)
    mChannel->SetCharsetAndSource(aCharsetSource, aCharset);

  return true;
}

bool
WyciwygChannelParent::RecvSetSecurityInfo(const nsCString& aSecurityInfo)
{
  if (mChannel) {
    nsCOMPtr<nsISupports> securityInfo;
    NS_DeserializeObject(aSecurityInfo, getter_AddRefs(securityInfo));
    mChannel->SetSecurityInfo(securityInfo);
  }

  return true;
}

bool
WyciwygChannelParent::RecvCancel(const nsresult& aStatusCode)
{
  if (mChannel)
    mChannel->Cancel(aStatusCode);
  return true;
}





NS_IMETHODIMP
WyciwygChannelParent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  LOG(("WyciwygChannelParent::OnStartRequest [this=%x]\n", this));

  nsresult rv;

  nsCOMPtr<nsIWyciwygChannel> chan = do_QueryInterface(aRequest, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsresult status;
  chan->GetStatus(&status);

  int64_t contentLength = -1;
  chan->GetContentLength(&contentLength);

  int32_t charsetSource = kCharsetUninitialized;
  nsAutoCString charset;
  chan->GetCharsetAndSource(&charsetSource, charset);

  nsCOMPtr<nsISupports> securityInfo;
  chan->GetSecurityInfo(getter_AddRefs(securityInfo));
  nsCString secInfoStr;
  if (securityInfo) {
    nsCOMPtr<nsISerializable> serializable = do_QueryInterface(securityInfo);
    if (serializable)
      NS_SerializeToString(serializable, secInfoStr);
    else {
      NS_ERROR("Can't serialize security info");
      return NS_ERROR_UNEXPECTED;
    }
  }

  if (mIPCClosed ||
      !SendOnStartRequest(status, contentLength, charsetSource, charset, secInfoStr)) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

NS_IMETHODIMP
WyciwygChannelParent::OnStopRequest(nsIRequest *aRequest,
                                    nsISupports *aContext,
                                    nsresult aStatusCode)
{
  LOG(("WyciwygChannelParent::OnStopRequest: [this=%x status=%ul]\n",
       this, aStatusCode));

  if (mIPCClosed || !SendOnStopRequest(aStatusCode)) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}





NS_IMETHODIMP
WyciwygChannelParent::OnDataAvailable(nsIRequest *aRequest,
                                      nsISupports *aContext,
                                      nsIInputStream *aInputStream,
                                      uint64_t aOffset,
                                      uint32_t aCount)
{
  LOG(("WyciwygChannelParent::OnDataAvailable [this=%x]\n", this));

  nsCString data;
  nsresult rv = NS_ReadInputStreamToString(aInputStream, data, aCount);
  if (NS_FAILED(rv))
    return rv;

  if (mIPCClosed || !SendOnDataAvailable(data, aOffset)) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}





NS_IMETHODIMP
WyciwygChannelParent::GetInterface(const nsIID& uuid, void** result)
{
  
  if (uuid.Equals(NS_GET_IID(nsILoadContext)) && mLoadContext) {
    NS_ADDREF(mLoadContext);
    *result = static_cast<nsILoadContext*>(mLoadContext);
    return NS_OK;
  }

  return QueryInterface(uuid, result);
}


}} 
