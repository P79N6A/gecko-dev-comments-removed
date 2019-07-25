







































#include "nsHttp.h"
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
  HttpChannelParent *p = new HttpChannelParent();
  p->AddRef();
  return p;
}

bool 
NeckoParent::DeallocPHttpChannel(PHttpChannelParent* channel)
{
  HttpChannelParent *p = static_cast<HttpChannelParent *>(channel);
  p->Release();
  return true;
}


}} 

