





#ifndef _channeldiverterchild_h_
#define _channeldiverterchild_h_

#include "mozilla/net/PChannelDiverterChild.h"

namespace mozilla {
namespace net {

class ChannelDiverterChild :
  public PChannelDiverterChild
{
public:
  ChannelDiverterChild();
  virtual ~ChannelDiverterChild();
};

} 
} 

#endif 
