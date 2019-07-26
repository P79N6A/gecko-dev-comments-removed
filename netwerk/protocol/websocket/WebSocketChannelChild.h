





#ifndef mozilla_net_WebSocketChannelChild_h
#define mozilla_net_WebSocketChannelChild_h

#include "mozilla/net/PWebSocketChild.h"
#include "mozilla/net/BaseWebSocketChannel.h"
#include "nsString.h"

namespace mozilla {
namespace net {

class ChannelEvent;
class ChannelEventQueue;

class WebSocketChannelChild : public BaseWebSocketChannel,
                              public PWebSocketChild
{
 public:
  WebSocketChannelChild(bool aSecure);
  ~WebSocketChannelChild();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITHREADRETARGETABLEREQUEST

  
  
  NS_IMETHOD AsyncOpen(nsIURI *aURI, const nsACString &aOrigin,
                       nsIWebSocketListener *aListener, nsISupports *aContext);
  NS_IMETHOD Close(uint16_t code, const nsACString & reason);
  NS_IMETHOD SendMsg(const nsACString &aMsg);
  NS_IMETHOD SendBinaryMsg(const nsACString &aMsg);
  NS_IMETHOD SendBinaryStream(nsIInputStream *aStream, uint32_t aLength);
  nsresult SendBinaryStream(OptionalInputStreamParams *aStream, uint32_t aLength);
  NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo);

  void AddIPDLReference();
  void ReleaseIPDLReference();

 private:
  bool RecvOnStart(const nsCString& aProtocol, const nsCString& aExtensions) MOZ_OVERRIDE;
  bool RecvOnStop(const nsresult& aStatusCode) MOZ_OVERRIDE;
  bool RecvOnMessageAvailable(const nsCString& aMsg) MOZ_OVERRIDE;
  bool RecvOnBinaryMessageAvailable(const nsCString& aMsg) MOZ_OVERRIDE;
  bool RecvOnAcknowledge(const uint32_t& aSize) MOZ_OVERRIDE;
  bool RecvOnServerClose(const uint16_t& aCode, const nsCString &aReason) MOZ_OVERRIDE;

  void OnStart(const nsCString& aProtocol, const nsCString& aExtensions);
  void OnStop(const nsresult& aStatusCode);
  void OnMessageAvailable(const nsCString& aMsg);
  void OnBinaryMessageAvailable(const nsCString& aMsg);
  void OnAcknowledge(const uint32_t& aSize);
  void OnServerClose(const uint16_t& aCode, const nsCString& aReason);
  void AsyncOpenFailed();  

  void DispatchToTargetThread(ChannelEvent *aChannelEvent);

  nsRefPtr<ChannelEventQueue> mEventQ;
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
