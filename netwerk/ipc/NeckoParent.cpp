







































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

nsresult 
NeckoParent::PHttpChannelDestructor(PHttpChannelParent* channel)
{
  delete channel;
  return NS_OK;
}


}} 

