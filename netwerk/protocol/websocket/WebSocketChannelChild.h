





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
  explicit WebSocketChannelChild(bool aSecure);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITHREADRETARGETABLEREQUEST

  
  
  NS_IMETHOD AsyncOpen(nsIURI *aURI, const nsACString &aOrigin,
                       nsIWebSocketListener *aListener, nsISupports *aContext) MOZ_OVERRIDE;
  NS_IMETHOD Close(uint16_t code, const nsACString & reason) MOZ_OVERRIDE;
  NS_IMETHOD SendMsg(const nsACString &aMsg) MOZ_OVERRIDE;
  NS_IMETHOD SendBinaryMsg(const nsACString &aMsg) MOZ_OVERRIDE;
  NS_IMETHOD SendBinaryStream(nsIInputStream *aStream, uint32_t aLength) MOZ_OVERRIDE;
  nsresult SendBinaryStream(OptionalInputStreamParams *aStream, uint32_t aLength);
  NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo) MOZ_OVERRIDE;

  void AddIPDLReference();
  void ReleaseIPDLReference();

  
  void GetEffectiveURL(nsAString& aEffectiveURL) const MOZ_OVERRIDE;
  bool IsEncrypted() const MOZ_OVERRIDE;

 private:
  ~WebSocketChannelChild();

  bool RecvOnStart(const nsCString& aProtocol, const nsCString& aExtensions,
                   const nsString& aEffectiveURL, const bool& aSecure) MOZ_OVERRIDE;
  bool RecvOnStop(const nsresult& aStatusCode) MOZ_OVERRIDE;
  bool RecvOnMessageAvailable(const nsCString& aMsg) MOZ_OVERRIDE;
  bool RecvOnBinaryMessageAvailable(const nsCString& aMsg) MOZ_OVERRIDE;
  bool RecvOnAcknowledge(const uint32_t& aSize) MOZ_OVERRIDE;
  bool RecvOnServerClose(const uint16_t& aCode, const nsCString &aReason) MOZ_OVERRIDE;

  void OnStart(const nsCString& aProtocol, const nsCString& aExtensions,
               const nsString& aEffectiveURL, const bool& aSecure);
  void OnStop(const nsresult& aStatusCode);
  void OnMessageAvailable(const nsCString& aMsg);
  void OnBinaryMessageAvailable(const nsCString& aMsg);
  void OnAcknowledge(const uint32_t& aSize);
  void OnServerClose(const uint16_t& aCode, const nsCString& aReason);
  void AsyncOpenFailed();  

  void DispatchToTargetThread(ChannelEvent *aChannelEvent);
  bool IsOnTargetThread();

  nsRefPtr<ChannelEventQueue> mEventQ;
  nsString mEffectiveURL;
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
