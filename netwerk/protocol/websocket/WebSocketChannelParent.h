





#ifndef mozilla_net_WebSocketChannelParent_h
#define mozilla_net_WebSocketChannelParent_h

#include "mozilla/net/PWebSocketParent.h"
#include "mozilla/net/NeckoParent.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWebSocketListener.h"
#include "nsIWebSocketChannel.h"
#include "nsILoadContext.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsIAuthPromptProvider;

namespace mozilla {
namespace net {

class WebSocketChannelParent : public PWebSocketParent,
                               public nsIWebSocketListener,
                               public nsIInterfaceRequestor
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBSOCKETLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

  WebSocketChannelParent(nsIAuthPromptProvider* aAuthProvider,
                         nsILoadContext* aLoadContext,
                         PBOverrideStatus aOverrideStatus);

 private:
  bool RecvAsyncOpen(const URIParams& aURI,
                     const nsCString& aOrigin,
                     const nsCString& aProtocol,
                     const bool& aSecure,
                     const uint32_t& aPingInterval,
                     const bool& aClientSetPingInterval,
                     const uint32_t& aPingTimeout,
                     const bool& aClientSetPingTimeout);
  bool RecvClose(const uint16_t & code, const nsCString & reason);
  bool RecvSendMsg(const nsCString& aMsg);
  bool RecvSendBinaryMsg(const nsCString& aMsg);
  bool RecvSendBinaryStream(const InputStreamParams& aStream,
                            const uint32_t& aLength);
  bool RecvDeleteSelf();

  void ActorDestroy(ActorDestroyReason why);

  nsCOMPtr<nsIAuthPromptProvider> mAuthProvider;
  nsCOMPtr<nsIWebSocketChannel> mChannel;
  nsCOMPtr<nsILoadContext> mLoadContext;
  bool mIPCOpen;

};

} 
} 

#endif 
