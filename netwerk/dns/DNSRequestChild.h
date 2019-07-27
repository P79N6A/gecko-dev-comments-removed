





#ifndef mozilla_net_DNSRequestChild_h
#define mozilla_net_DNSRequestChild_h

#include "mozilla/net/PDNSRequestChild.h"
#include "nsICancelable.h"
#include "nsIDNSRecord.h"
#include "nsIDNSListener.h"
#include "nsIEventTarget.h"

namespace mozilla {
namespace net {

class DNSRequestChild final
  : public PDNSRequestChild
  , public nsICancelable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICANCELABLE

  DNSRequestChild(const nsCString& aHost, const uint32_t& aFlags,
                  const nsCString& aNetworkInterface,
                  nsIDNSListener *aListener, nsIEventTarget *target);

  void AddIPDLReference() {
    AddRef();
  }
  void ReleaseIPDLReference();

  
  void StartRequest();
  void CallOnLookupComplete();

protected:
  friend class CancelDNSRequestEvent;
  friend class ChildDNSService;
  virtual ~DNSRequestChild() {}

  virtual bool RecvLookupCompleted(const DNSRequestResponse& reply) override;
  virtual void ActorDestroy(ActorDestroyReason why) override;

  nsCOMPtr<nsIDNSListener>  mListener;
  nsCOMPtr<nsIEventTarget>  mTarget;
  nsCOMPtr<nsIDNSRecord>    mResultRecord;
  nsresult                  mResultStatus;
  nsCString                 mHost;
  uint16_t                  mFlags;
  nsCString                 mNetworkInterface;
  bool                      mIPCOpen;
};

} 
} 

#endif 
