







































#ifndef mozilla_net_HttpChannelChild_h
#define mozilla_net_HttpChannelChild_h

#include "mozilla/net/PHttpChannelChild.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace net {


class HttpChannelChild :
  public PHttpChannelChild
{
public:
  HttpChannelChild();
  virtual ~HttpChannelChild();

protected:
};


} 
} 

#endif
