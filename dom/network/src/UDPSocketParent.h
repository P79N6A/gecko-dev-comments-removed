





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

  explicit UDPSocketParent(nsIUDPSocketFilter* filter);

  bool Init(const nsCString& aHost, const uint16_t aPort);

  virtual bool RecvClose() MOZ_OVERRIDE;
  virtual bool RecvData(const InfallibleTArray<uint8_t>& aData,
                        const nsCString& aRemoteAddress,
                        const uint16_t& aPort) MOZ_OVERRIDE;
  virtual bool RecvDataWithAddress( const InfallibleTArray<uint8_t>& data,
                                    const mozilla::net::NetAddr& addr);
  virtual bool RecvRequestDelete() MOZ_OVERRIDE;

  virtual nsresult OfflineNotification(nsISupports *) MOZ_OVERRIDE;
  virtual uint32_t GetAppId() MOZ_OVERRIDE;
private:
  virtual ~UDPSocketParent();

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  bool mIPCOpen;
  nsCOMPtr<nsIUDPSocket> mSocket;
  nsCOMPtr<nsIUDPSocketFilter> mFilter;
  nsRefPtr<mozilla::net::OfflineObserver> mObserver;
};

} 
} 

#endif 
