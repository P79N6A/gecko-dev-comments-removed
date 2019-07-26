



#include "mozilla/net/PNeckoParent.h"
#include "mozilla/net/PTCPServerSocketParent.h"
#include "nsITCPSocketParent.h"
#include "nsITCPServerSocketParent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsIDOMTCPSocket.h"

namespace mozilla {
namespace dom {

class PBrowserParent;

class TCPServerSocketParent : public mozilla::net::PTCPServerSocketParent
                            , public nsITCPServerSocketParent
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS(TCPServerSocketParent)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITCPSERVERSOCKETPARENT

  TCPServerSocketParent() : mNeckoParent(nullptr), mIPCOpen(false) {}

  bool Init(PNeckoParent* neckoParent, const uint16_t& aLocalPort, const uint16_t& aBacklog,
            const nsString& aBinaryType);

  virtual bool RecvClose() MOZ_OVERRIDE;
  virtual bool RecvRequestDelete() MOZ_OVERRIDE;

  void AddIPDLReference();
  void ReleaseIPDLReference();

private:
  ~TCPServerSocketParent() {}

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  PNeckoParent* mNeckoParent;
  nsCOMPtr<nsITCPSocketIntermediary> mIntermediary;
  nsCOMPtr<nsIDOMTCPServerSocket> mServerSocket;
  bool mIPCOpen;
};

} 
} 
