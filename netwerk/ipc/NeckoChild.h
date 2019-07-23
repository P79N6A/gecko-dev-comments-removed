







































#ifndef mozilla_net_NeckoChild_h
#define mozilla_net_NeckoChild_h

#include "mozilla/net/PNeckoChild.h"
#include "mozilla/net/HttpChannelChild.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace net {


class NeckoChild :
  public PNeckoChild
{
public:
  NeckoChild();
  virtual ~NeckoChild();

  static void InitNeckoChild();

  virtual PHttpChannelChild* AllocPHttpChannel();
  virtual bool DeallocPHttpChannel(PHttpChannelChild*);

protected:
};





extern PNeckoChild *gNeckoChild;

static inline PRBool 
IsNeckoChild() 
{
  return XRE_GetProcessType() == GeckoProcessType_Content;        
}

} 
} 

#endif
