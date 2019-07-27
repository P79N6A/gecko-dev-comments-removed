





#ifndef mozilla_net_DNSRequestParent_h
#define mozilla_net_DNSRequestParent_h

#include "mozilla/net/PDNSRequestParent.h"
#include "nsIDNSService.h"
#include "nsIDNSListener.h"

namespace mozilla {
namespace net {

class DNSRequestParent
  : public PDNSRequestParent
  , public nsIDNSListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDNSLISTENER

  DNSRequestParent();

  void DoAsyncResolve(const nsACString  &hostname, uint32_t flags);

  
  
  bool RecvCancelDNSRequest(const nsCString& hostName,
                            const uint32_t& flags,
                            const nsresult& reason);
  bool Recv__delete__();

protected:
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;
private:
  virtual ~DNSRequestParent();

  uint32_t mFlags;
  bool mIPCClosed;  
};

} 
} 
#endif 
