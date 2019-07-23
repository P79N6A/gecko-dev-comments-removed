







































#include "mozilla/net/NeckoChild.h"
#include "mozilla/dom/ContentProcessChild.h"
#include "mozilla/net/HttpChannelChild.h"

namespace mozilla {
namespace net {

PNeckoChild *gNeckoChild = nsnull;


NeckoChild::NeckoChild()
{
}

NeckoChild::~NeckoChild()
{
}

void NeckoChild::InitNeckoChild()
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentProcessChild * cpc = 
      mozilla::dom::ContentProcessChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    gNeckoChild = cpc->SendPNeckoConstructor(); 
    NS_ASSERTION(gNeckoChild, "PNecko Protocol init failed!");
  }
}

PHttpChannelChild* 
NeckoChild::PHttpChannelConstructor()
{
  return new HttpChannelChild();
}

nsresult 
NeckoChild::PHttpChannelDestructor(PHttpChannelChild* channel)
{
  delete channel;
  return NS_OK;
}

}} 

