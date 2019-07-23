







































#include "mozilla/net/PNeckoParent.h"
#include "mozilla/net/HttpChannelChild.h"

#ifndef mozilla_net_NeckoParent_h
#define mozilla_net_NeckoParent_h

namespace mozilla {
namespace net {


class NeckoParent :
  public PNeckoParent
{
public:
  NeckoParent();
  virtual ~NeckoParent();

protected:
  virtual PHttpChannelParent* PHttpChannelConstructor();
  virtual nsresult PHttpChannelDestructor(PHttpChannelParent*);
};

} 
} 

#endif 
