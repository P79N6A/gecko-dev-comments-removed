





#ifndef mozilla_dom_UDPSocketParent_h__
#define mozilla_dom_UDPSocketParent_h__

#include "mozilla/net/PUDPSocketParent.h"
#include "nsCOMPtr.h"
#include "nsIUDPSocket.h"
#include "nsIUDPSocketFilter.h"
#include "mozilla/net/OfflineObserver.h"

namespace mozilla {
namespace dom {

class UDPSocketParent : public mozilla::net::PUDPSocketParent
                      , public nsIUDPSocketListener
                      , public mozilla::net::DisconnectableParent
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPSOCKETLISTENER

  explicit UDPSocketParent();

  bool Init(const nsACString& aFilter);

  virtual bool RecvBind(const UDPAddressInfo& aAddressInfo,
                        const bool& aAddressReuse, const bool& aLoopback) MOZ_OVERRIDE;

  virtual bool RecvOutgoingData(const UDPData& aData, const UDPSocketAddr& aAddr) MOZ_OVERRIDE;

  virtual bool RecvClose() MOZ_OVERRIDE;

  virtual bool RecvRequestDelete() MOZ_OVERRIDE;
  virtual bool RecvJoinMulticast(const nsCString& aMulticastAddress,
                                 const nsCString& aInterface) MOZ_OVERRIDE;
  virtual bool RecvLeaveMulticast(const nsCString& aMulticastAddress,
                                  const nsCString& aInterface) MOZ_OVERRIDE;

  virtual nsresult OfflineNotification(nsISupports *) MOZ_OVERRIDE;
  virtual uint32_t GetAppId() MOZ_OVERRIDE;
private:
  virtual ~UDPSocketParent();

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;
  void Send(const InfallibleTArray<uint8_t>& aData, const UDPSocketAddr& aAddr);
  void Send(const InputStreamParams& aStream, const UDPSocketAddr& aAddr);
  nsresult BindInternal(const nsCString& aHost, const uint16_t& aPort,
                        const bool& aAddressReuse, const bool& aLoopback);

  void FireInternalError(uint32_t aLineNo);

  bool mIPCOpen;
  nsCOMPtr<nsIUDPSocket> mSocket;
  nsCOMPtr<nsIUDPSocketFilter> mFilter;
  nsRefPtr<mozilla::net::OfflineObserver> mObserver;
};

} 
} 

#endif 
