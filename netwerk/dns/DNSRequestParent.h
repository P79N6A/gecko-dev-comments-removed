





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

  void DoAsyncResolve(const nsACString  &hostname, uint32_t flags,
                      const nsACString  &networkInterface);

  
  
  bool RecvCancelDNSRequest(const nsCString& hostName,
                            const uint32_t& flags,
                            const nsCString& networkInterface,
                            const nsresult& reason) override;
  bool Recv__delete__() override;

protected:
  virtual void ActorDestroy(ActorDestroyReason why) override;
private:
  virtual ~DNSRequestParent();

  uint32_t mFlags;
  bool mIPCClosed;  
};

} 
} 

#endif 
