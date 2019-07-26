





#ifndef _channeldiverterchild_h_
#define _channeldiverterchild_h_

#include "mozilla/net/PChannelDiverterChild.h"

class nsIDivertableChannel;

namespace mozilla {
namespace net {

class ChannelDiverterArgs;

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
