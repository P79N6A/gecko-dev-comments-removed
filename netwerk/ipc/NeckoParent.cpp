







































#include "mozilla/net/NeckoParent.h"
#include "mozilla/net/HttpChannelParent.h"

namespace mozilla {
namespace net {


NeckoParent::NeckoParent()
{
}

NeckoParent::~NeckoParent()
{
}

PHttpChannelParent* 
NeckoParent::AllocPHttpChannel()
{
  return new HttpChannelParent();
}

bool 
NeckoParent::DeallocPHttpChannel(PHttpChannelParent* channel)
{
  delete channel;
  return true;
}


}} 

