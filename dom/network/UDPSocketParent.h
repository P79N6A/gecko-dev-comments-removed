





#ifndef mozilla_dom_UDPSocketParent_h__
#define mozilla_dom_UDPSocketParent_h__

#include "mozilla/net/PUDPSocketParent.h"
#include "nsCOMPtr.h"
#include "nsIUDPSocket.h"
#include "nsIUDPSocketFilter.h"
#include "mozilla/net/OfflineObserver.h"
#include "mozilla/dom/PermissionMessageUtils.h"

namespace mozilla {
namespace dom {

class UDPSocketParent : public mozilla::net::PUDPSocketParent
                      , public nsIUDPSocketListener
                      , public mozilla::net::DisconnectableParent
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPSOCKETLISTENER

  UDPSocketParent();

  bool Init(const IPC::Principal& aPrincipal, const nsACString& aFilter);

  virtual bool RecvBind(const UDPAddressInfo& aAddressInfo,
                        const bool& aAddressReuse, const bool& aLoopback) override;

  virtual bool RecvOutgoingData(const UDPData& aData, const UDPSocketAddr& aAddr) override;

  virtual bool RecvClose() override;
  virtual bool RecvRequestDelete() override;
  virtual bool RecvJoinMulticast(const nsCString& aMulticastAddress,
                                 const nsCString& aInterface) override;
  virtual bool RecvLeaveMulticast(const nsCString& aMulticastAddress,
                                  const nsCString& aInterface) override;
  virtual nsresult OfflineNotification(nsISupports *) override;
  virtual uint32_t GetAppId() override;

private:
  virtual ~UDPSocketParent();

  virtual void ActorDestroy(ActorDestroyReason why) override;
  void Send(const InfallibleTArray<uint8_t>& aData, const UDPSocketAddr& aAddr);
  void Send(const InputStreamParams& aStream, const UDPSocketAddr& aAddr);
  nsresult BindInternal(const nsCString& aHost, const uint16_t& aPort,
                        const bool& aAddressReuse, const bool& aLoopback);

  void FireInternalError(uint32_t aLineNo);

  bool mIPCOpen;
  nsCOMPtr<nsIUDPSocket> mSocket;
  nsCOMPtr<nsIUDPSocketFilter> mFilter;
  nsRefPtr<mozilla::net::OfflineObserver> mObserver;
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

} 
} 

#endif 
