





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
  virtual ~DNSRequestParent();

  void DoAsyncResolve(const nsACString  &hostname, uint32_t flags);

protected:
  virtual void ActorDestroy(ActorDestroyReason why);
private:
  uint32_t mFlags;
  bool mIPCClosed;  
};

} 
} 
#endif 
