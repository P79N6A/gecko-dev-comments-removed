






































#ifndef mozilla_net_WebSocketChannelParent_h
#define mozilla_net_WebSocketChannelParent_h

#include "mozilla/net/PWebSocketParent.h"
#include "mozilla/net/nsWebSocketHandler.h"
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

  WebSocketChannelParent(nsIAuthPromptProvider* aAuthProvider);

 private:
  bool RecvAsyncOpen(const IPC::URI& aURI,
                     const nsCString& aOrigin,
                     const nsCString& aProtocol,
                     const bool& aSecure);
  bool RecvClose();
  bool RecvSendMsg(const nsCString& aMsg);
  bool RecvSendBinaryMsg(const nsCString& aMsg);
  bool RecvDeleteSelf();
  bool CancelEarly();

  void ActorDestroy(ActorDestroyReason why);

  nsCOMPtr<nsIAuthPromptProvider> mAuthProvider;
  nsCOMPtr<nsIWebSocketProtocol> mChannel;
  bool mIPCOpen;
};

} 
} 

#endif 
