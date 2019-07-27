





#include "mozilla/net/ChannelDiverterParent.h"
#include "mozilla/net/NeckoChannelParams.h"
#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/net/FTPChannelParent.h"
#include "mozilla/net/PHttpChannelParent.h"
#include "mozilla/net/PFTPChannelParent.h"
#include "ADivertableParentChannel.h"

namespace mozilla {
namespace net {

ChannelDiverterParent::ChannelDiverterParent()
{
}

ChannelDiverterParent::~ChannelDiverterParent()
{
}

bool
ChannelDiverterParent::Init(const ChannelDiverterArgs& aChannel)
{
  switch (aChannel.type()) {
  case ChannelDiverterArgs::TPHttpChannelParent:
  {
    mDivertableChannelParent = static_cast<ADivertableParentChannel*>(
      static_cast<HttpChannelParent*>(aChannel.get_PHttpChannelParent()));
    break;
  }
  case ChannelDiverterArgs::TPFTPChannelParent:
  {
    mDivertableChannelParent = static_cast<ADivertableParentChannel*>(
      static_cast<FTPChannelParent*>(aChannel.get_PFTPChannelParent()));
    break;
  }
  default:
    NS_NOTREACHED("unknown ChannelDiverterArgs type");
    return false;
  }
  MOZ_ASSERT(mDivertableChannelParent);

  nsresult rv = mDivertableChannelParent->SuspendForDiversion();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }
  return true;
}

void
ChannelDiverterParent::DivertTo(nsIStreamListener* newListener)
{
  MOZ_ASSERT(newListener);
  MOZ_ASSERT(mDivertableChannelParent);

  mDivertableChannelParent->DivertTo(newListener);
}

void
ChannelDiverterParent::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

} 
} 
