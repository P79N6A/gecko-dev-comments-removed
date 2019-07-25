






































#include "WebSocketLog.h"
#include "WebSocketChannelParent.h"
#include "nsIAuthPromptProvider.h"

namespace mozilla {
namespace net {

NS_IMPL_THREADSAFE_ISUPPORTS2(WebSocketChannelParent,
                              nsIWebSocketListener,
                              nsIInterfaceRequestor)

WebSocketChannelParent::WebSocketChannelParent(nsIAuthPromptProvider* aAuthProvider)
  : mAuthProvider(aAuthProvider)
  , mIPCOpen(true)
{
#if defined(PR_LOGGING)
  if (!webSocketLog)
    webSocketLog = PR_NewLogModule("nsWebSocket");
#endif
}

bool
WebSocketChannelParent::RecvDeleteSelf()
{
  LOG(("WebSocketChannelParent::RecvDeleteSelf() %p\n", this));
  mChannel = nsnull;
  mAuthProvider = nsnull;
  return mIPCOpen ? Send__delete__(this) : true;
}

bool
WebSocketChannelParent::RecvAsyncOpen(const IPC::URI& aURI,
                                      const nsCString& aOrigin,
                                      const nsCString& aProtocol,
                                      const bool& aSecure)
{
  LOG(("WebSocketChannelParent::RecvAsyncOpen() %p\n", this));
  nsresult rv;
  if (aSecure) {
    mChannel =
      do_CreateInstance("@mozilla.org/network/protocol;1?name=wss", &rv);
  } else {
    mChannel =
      do_CreateInstance("@mozilla.org/network/protocol;1?name=ws", &rv);
  }
  if (NS_FAILED(rv))
    goto fail;

  rv = mChannel->SetNotificationCallbacks(this);
  if (NS_FAILED(rv))
    goto fail;

  rv = mChannel->SetProtocol(aProtocol);
  if (NS_FAILED(rv))
    goto fail;

  rv = mChannel->AsyncOpen(aURI, aOrigin, this, nsnull);
  if (NS_FAILED(rv))
    goto fail;

  return true;

fail:
  mChannel = nsnull;
  return SendOnStop(rv);
}

bool
WebSocketChannelParent::RecvClose(const PRUint16& code, const nsCString& reason)
{
  LOG(("WebSocketChannelParent::RecvClose() %p\n", this));
  if (mChannel) {
    nsresult rv = mChannel->Close(code, reason);
    NS_ENSURE_SUCCESS(rv, true);
  }
  return true;
}

bool
WebSocketChannelParent::RecvSendMsg(const nsCString& aMsg)
{
  LOG(("WebSocketChannelParent::RecvSendMsg() %p\n", this));
  if (mChannel) {
    nsresult rv = mChannel->SendMsg(aMsg);
    NS_ENSURE_SUCCESS(rv, true);
  }
  return true;
}

bool
WebSocketChannelParent::RecvSendBinaryMsg(const nsCString& aMsg)
{
  LOG(("WebSocketChannelParent::RecvSendBinaryMsg() %p\n", this));
  if (mChannel) {
    nsresult rv = mChannel->SendBinaryMsg(aMsg);
    NS_ENSURE_SUCCESS(rv, true);
  }
  return true;
}

NS_IMETHODIMP
WebSocketChannelParent::GetInterface(const nsIID & iid, void **result NS_OUTPARAM)
{
  LOG(("WebSocketChannelParent::GetInterface() %p\n", this));
  if (mAuthProvider && iid.Equals(NS_GET_IID(nsIAuthPromptProvider)))
    return mAuthProvider->GetAuthPrompt(nsIAuthPromptProvider::PROMPT_NORMAL,
                                        iid, result);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
WebSocketChannelParent::OnStart(nsISupports *aContext)
{
  LOG(("WebSocketChannelParent::OnStart() %p\n", this));
  nsCAutoString protocol, extensions;
  if (mChannel) {
    mChannel->GetProtocol(protocol);
    mChannel->GetExtensions(extensions);
  }
  if (!mIPCOpen || !SendOnStart(protocol, extensions)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelParent::OnStop(nsISupports *aContext, nsresult aStatusCode)
{
  LOG(("WebSocketChannelParent::OnStop() %p\n", this));
  if (!mIPCOpen || !SendOnStop(aStatusCode)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelParent::OnMessageAvailable(nsISupports *aContext, const nsACString& aMsg)
{
  LOG(("WebSocketChannelParent::OnMessageAvailable() %p\n", this));
  if (!mIPCOpen || !SendOnMessageAvailable(nsCString(aMsg))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelParent::OnBinaryMessageAvailable(nsISupports *aContext, const nsACString& aMsg)
{
  LOG(("WebSocketChannelParent::OnBinaryMessageAvailable() %p\n", this));
  if (!mIPCOpen || !SendOnBinaryMessageAvailable(nsCString(aMsg))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelParent::OnAcknowledge(nsISupports *aContext, PRUint32 aSize)
{
  LOG(("WebSocketChannelParent::OnAcknowledge() %p\n", this));
  if (!mIPCOpen || !SendOnAcknowledge(aSize)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelParent::OnServerClose(nsISupports *aContext,
                                      PRUint16 code, const nsACString & reason)
{
  LOG(("WebSocketChannelParent::OnServerClose() %p\n", this));
  if (!mIPCOpen || !SendOnServerClose(code, nsCString(reason))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

void
WebSocketChannelParent::ActorDestroy(ActorDestroyReason why)
{
  LOG(("WebSocketChannelParent::ActorDestroy() %p\n", this));
  mIPCOpen = false;
}

} 
} 
