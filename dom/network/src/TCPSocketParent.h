



#include "mozilla/net/PTCPSocketParent.h"
#include "nsITCPSocketParent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsIDOMTCPSocket.h"

struct JSContext;
class JSObject;

namespace mozilla {
namespace dom {

class PBrowserParent;

class TCPSocketParent : public mozilla::net::PTCPSocketParent
                      , public nsITCPSocketParent
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS(TCPSocketParent)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITCPSOCKETPARENT

  TCPSocketParent() : mIntermediaryObj(nullptr), mIPCOpen(true) {}

  bool Init(const nsString& aHost, const uint16_t& aPort,
            const bool& useSSL, const nsString& aBinaryType);

  virtual bool RecvSuspend() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;
  virtual bool RecvClose() MOZ_OVERRIDE;
  virtual bool RecvData(const SendableData& aData) MOZ_OVERRIDE;
  virtual bool RecvRequestDelete() MOZ_OVERRIDE;

private:
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  nsCOMPtr<nsITCPSocketIntermediary> mIntermediary;
  nsCOMPtr<nsIDOMTCPSocket> mSocket;
  JSObject* mIntermediaryObj;
  bool mIPCOpen;
};

} 
} 
