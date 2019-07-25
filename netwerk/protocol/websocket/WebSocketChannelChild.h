





#ifndef mozilla_net_WebSocketChannelChild_h
#define mozilla_net_WebSocketChannelChild_h

#include "mozilla/net/PWebSocketChild.h"
#include "mozilla/net/ChannelEventQueue.h"
#include "mozilla/net/BaseWebSocketChannel.h"
#include "nsCOMPtr.h"
#include "nsString.h"

namespace mozilla {
namespace net {

class WebSocketChannelChild : public BaseWebSocketChannel,
                              public PWebSocketChild
{
 public:
  WebSocketChannelChild(bool aSecure);
  ~WebSocketChannelChild();

  NS_DECL_ISUPPORTS

  
  
  NS_IMETHOD AsyncOpen(nsIURI *aURI, const nsACString &aOrigin,
                       nsIWebSocketListener *aListener, nsISupports *aContext);
  NS_IMETHOD Close(uint16_t code, const nsACString & reason);
  NS_IMETHOD SendMsg(const nsACString &aMsg);
  NS_IMETHOD SendBinaryMsg(const nsACString &aMsg);
  NS_IMETHOD SendBinaryStream(nsIInputStream *aStream, uint32_t aLength);
  NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo);

  void AddIPDLReference();
  void ReleaseIPDLReference();

 private:
  bool RecvOnStart(const nsCString& aProtocol, const nsCString& aExtensions);
  bool RecvOnStop(const nsresult& aStatusCode);
  bool RecvOnMessageAvailable(const nsCString& aMsg);
  bool RecvOnBinaryMessageAvailable(const nsCString& aMsg);
  bool RecvOnAcknowledge(const uint32_t& aSize);
  bool RecvOnServerClose(const uint16_t& aCode, const nsCString &aReason);
  bool RecvAsyncOpenFailed();

  void OnStart(const nsCString& aProtocol, const nsCString& aExtensions);
  void OnStop(const nsresult& aStatusCode);
  void OnMessageAvailable(const nsCString& aMsg);
  void OnBinaryMessageAvailable(const nsCString& aMsg);
  void OnAcknowledge(const uint32_t& aSize);
  void OnServerClose(const uint16_t& aCode, const nsCString& aReason);
  void AsyncOpenFailed();  

  ChannelEventQueue mEventQ;
  bool mIPCOpen;

  friend class StartEvent;
  friend class StopEvent;
  friend class MessageEvent;
  friend class AcknowledgeEvent;
  friend class ServerCloseEvent;
  friend class AsyncOpenFailedEvent;
};

} 
} 

#endif 
