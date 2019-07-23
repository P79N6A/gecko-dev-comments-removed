







































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
NeckoParent::PHttpChannelConstructor()
{
  return new HttpChannelParent();
}

bool 
NeckoParent::PHttpChannelDestructor(PHttpChannelParent* channel)
{
  delete channel;
  return true;
}


}} 

