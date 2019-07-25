







































#include "nsHttp.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/net/HttpChannelChild.h"
#include "mozilla/net/CookieServiceChild.h"

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
    mozilla::dom::ContentChild * cpc = 
      mozilla::dom::ContentChild::GetSingleton();
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
NeckoChild::AllocPHttpChannel(PBrowserChild* iframeEmbedding)
{
  
  NS_RUNTIMEABORT("AllocPHttpChannel should not be called");
  return nsnull;
}

bool 
NeckoChild::DeallocPHttpChannel(PHttpChannelChild* channel)
{
  NS_ABORT_IF_FALSE(IsNeckoChild(), "DeallocPHttpChannel called by non-child!");

  
  delete channel;
  return true;
}

PCookieServiceChild* 
NeckoChild::AllocPCookieService()
{
  
  NS_NOTREACHED("AllocPCookieService should not be called");
  return nsnull;
}

bool 
NeckoChild::DeallocPCookieService(PCookieServiceChild* cs)
{
  NS_ASSERTION(IsNeckoChild(), "DeallocPCookieService called by non-child!");

  CookieServiceChild *p = static_cast<CookieServiceChild*>(cs);
  p->Release();
  return true;
}

}} 

