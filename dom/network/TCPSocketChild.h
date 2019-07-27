





#ifndef mozilla_dom_TCPSocketChild_h
#define mozilla_dom_TCPSocketChild_h

#include "mozilla/net/PTCPSocketChild.h"
#include "nsITCPSocketChild.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "js/TypeDecls.h"

#define TCPSOCKETCHILD_CID \
  { 0xa589d96f, 0x7e09, 0x4edf, { 0xa0, 0x1a, 0xeb, 0x49, 0x51, 0xf4, 0x2f, 0x37 } }

class nsITCPSocketInternal;

namespace mozilla {
namespace dom {

class TCPSocketChildBase : public nsITCPSocketChild {
public:
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TCPSocketChildBase)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  void AddIPDLReference();
  void ReleaseIPDLReference();

protected:
  TCPSocketChildBase();
  virtual ~TCPSocketChildBase();

  nsCOMPtr<nsITCPSocketInternal> mSocket;
  JS::Heap<JSObject*> mWindowObj;
  bool mIPCOpen;
};

class TCPSocketChild : public mozilla::net::PTCPSocketChild
                     , public TCPSocketChildBase
{
public:
  NS_DECL_NSITCPSOCKETCHILD
  NS_IMETHOD_(MozExternalRefCountType) Release() override;

  TCPSocketChild();
  ~TCPSocketChild();

  void Init(const nsString& aHost, const uint16_t& aPort);

  virtual bool RecvCallback(const nsString& aType,
                            const CallbackData& aData,
                            const nsString& aReadyState) override;
  virtual bool RecvRequestDelete() override;
  virtual bool RecvUpdateBufferedAmount(const uint32_t& aBufferred,
                                        const uint32_t& aTrackingNumber) override;
private:
  nsString mHost;
  uint16_t mPort;
};

} 
} 

#endif
