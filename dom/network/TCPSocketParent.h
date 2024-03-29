





#ifndef mozilla_dom_TCPSocketParent_h
#define mozilla_dom_TCPSocketParent_h

#include "mozilla/net/PTCPSocketParent.h"
#include "nsITCPSocketParent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsIDOMTCPSocket.h"
#include "js/TypeDecls.h"
#include "mozilla/net/OfflineObserver.h"

#define TCPSOCKETPARENT_CID \
  { 0x4e7246c6, 0xa8b3, 0x426d, { 0x9c, 0x17, 0x76, 0xda, 0xb1, 0xe1, 0xe1, 0x4a } }

namespace mozilla {
namespace dom {

class TCPSocketParentBase : public nsITCPSocketParent
                          , public mozilla::net::DisconnectableParent
{
public:
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TCPSocketParentBase)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  void AddIPDLReference();
  void ReleaseIPDLReference();

protected:
  TCPSocketParentBase();
  virtual ~TCPSocketParentBase();

  JS::Heap<JSObject*> mIntermediaryObj;
  nsCOMPtr<nsITCPSocketIntermediary> mIntermediary;
  nsCOMPtr<nsIDOMTCPSocket> mSocket;
  nsRefPtr<mozilla::net::OfflineObserver> mObserver;
  bool mIPCOpen;
};

class TCPSocketParent : public mozilla::net::PTCPSocketParent
                      , public TCPSocketParentBase
{
public:
  NS_DECL_NSITCPSOCKETPARENT
  NS_IMETHOD_(MozExternalRefCountType) Release() override;

  TCPSocketParent() {}

  virtual bool RecvOpen(const nsString& aHost, const uint16_t& aPort,
                        const bool& useSSL, const nsString& aBinaryType) override;

  virtual bool RecvStartTLS() override;
  virtual bool RecvSuspend() override;
  virtual bool RecvResume() override;
  virtual bool RecvClose() override;
  virtual bool RecvData(const SendableData& aData,
                        const uint32_t& aTrackingNumber) override;
  virtual bool RecvRequestDelete() override;
  virtual nsresult OfflineNotification(nsISupports *) override;
  virtual uint32_t GetAppId() override;
  bool GetInBrowser();

private:
  virtual void ActorDestroy(ActorDestroyReason why) override;
};

} 
} 

#endif
