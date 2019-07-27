





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

  bool Init(const ChannelDiverterArgs& aArgs);

  void DivertTo(nsIStreamListener* newListener);

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
  nsRefPtr<ADivertableParentChannel> mDivertableChannelParent;
};

} 
} 

#endif 
