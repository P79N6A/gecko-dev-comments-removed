







































#ifndef mozilla_net_HttpChannelParent_h
#define mozilla_net_HttpChannelParent_h

#include "mozilla/net/PHttpChannelParent.h"

namespace mozilla {
namespace net {


class HttpChannelParent :
  public PHttpChannelParent
{
public:
  HttpChannelParent();
  virtual ~HttpChannelParent();

protected:
  virtual bool RecvasyncOpen(const nsCString& uri);
};

} 
} 

#endif 
