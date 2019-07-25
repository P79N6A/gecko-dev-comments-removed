







































#include "nsHttp.h"
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
  NS_ABORT_IF_FALSE(IsNeckoChild(), "InitNeckoChild called by non-child!");

  if (!gNeckoChild) {
    mozilla::dom::ContentProcessChild * cpc = 
      mozilla::dom::ContentProcessChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    gNeckoChild = cpc->SendPNeckoConstructor(); 
    NS_ASSERTION(gNeckoChild, "PNecko Protocol init failed!");
  }
}



void NeckoChild::DestroyNeckoChild()
{
  NS_ABORT_IF_FALSE(IsNeckoChild(), "DestroyNeckoChild called by non-child!");
  static bool alreadyDestroyed = false;
  NS_ABORT_IF_FALSE(!alreadyDestroyed, "DestroyNeckoChild already called!");

  if (!alreadyDestroyed) {
    Send__delete__(gNeckoChild); 
    gNeckoChild = nsnull;
    alreadyDestroyed = true;
  }
}

PHttpChannelChild* 
NeckoChild::AllocPHttpChannel()
{
  
  NS_RUNTIMEABORT("AllocPHttpChannel should not be called");
  return nsnull;
}

bool 
NeckoChild::DeallocPHttpChannel(PHttpChannelChild* channel)
{
  NS_ABORT_IF_FALSE(IsNeckoChild(), "DeallocPHttpChannel called by non-child!");

  HttpChannelChild *p = static_cast<HttpChannelChild*>(channel);
  p->Release();
  return true;
}

}} 

