



#include "mozilla/net/PTCPSocketParent.h"
#include "nsITCPSocketParent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsIDOMTCPSocket.h"
#include "js/TypeDecls.h"

#define TCPSOCKETPARENT_CID \
  { 0x4e7246c6, 0xa8b3, 0x426d, { 0x9c, 0x17, 0x76, 0xda, 0xb1, 0xe1, 0xe1, 0x4a } }

namespace mozilla {
namespace dom {

class PBrowserParent;

class TCPSocketParentBase : public nsITCPSocketParent
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS(TCPSocketParentBase)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  void AddIPDLReference();
  void ReleaseIPDLReference();

protected:
  TCPSocketParentBase();
  virtual ~TCPSocketParentBase();

  nsCOMPtr<nsITCPSocketIntermediary> mIntermediary;
  nsCOMPtr<nsIDOMTCPSocket> mSocket;
  bool mIPCOpen;
};

class TCPSocketParent : public mozilla::net::PTCPSocketParent
                      , public TCPSocketParentBase
{
public:
  NS_DECL_NSITCPSOCKETPARENT
  NS_IMETHOD_(nsrefcnt) Release() MOZ_OVERRIDE;

  TCPSocketParent() : mIntermediaryObj(nullptr) {}

  virtual bool RecvOpen(const nsString& aHost, const uint16_t& aPort,
                        const bool& useSSL, const nsString& aBinaryType);

  virtual bool RecvStartTLS() MOZ_OVERRIDE;
  virtual bool RecvSuspend() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;
  virtual bool RecvClose() MOZ_OVERRIDE;
  virtual bool RecvData(const SendableData& aData,
                        const uint32_t& aTrackingNumber) MOZ_OVERRIDE;
  virtual bool RecvRequestDelete() MOZ_OVERRIDE;

private:
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  JSObject* mIntermediaryObj;
};

} 
} 
