





#ifndef _channeldiverterparent_h_
#define _channeldiverterparent_h_

#include "mozilla/net/PChannelDiverterParent.h"

class nsIStreamListener;

namespace mozilla {
namespace net {

class ChannelDiverterArgs;
class ADivertableParentChannel;

class ChannelDiverterParent :
  public PChannelDiverterParent
{
public:
  ChannelDiverterParent();
  virtual ~ChannelDiverterParent();

  bool Init(const ChannelDiverterArgs& aChannel);

  void DivertTo(nsIStreamListener* newListener);
private:
  nsRefPtr<ADivertableParentChannel> mDivertableChannelParent;
};

} 
} 

#endif 
