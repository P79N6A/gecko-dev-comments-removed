





#ifndef mozilla_net_WebSocketChannelParent_h
#define mozilla_net_WebSocketChannelParent_h

#include "mozilla/net/PWebSocketParent.h"
#include "nsIWebSocketListener.h"
#include "nsIWebSocketChannel.h"
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
  bool RecvClose(const PRUint16 & code, const nsCString & reason);
  bool RecvSendMsg(const nsCString& aMsg);
  bool RecvSendBinaryMsg(const nsCString& aMsg);
  bool RecvSendBinaryStream(const InputStream& aStream,
                            const PRUint32& aLength);
  bool RecvDeleteSelf();

  void ActorDestroy(ActorDestroyReason why);

  nsCOMPtr<nsIAuthPromptProvider> mAuthProvider;
  nsCOMPtr<nsIWebSocketChannel> mChannel;
  bool mIPCOpen;
};

} 
} 

#endif 
