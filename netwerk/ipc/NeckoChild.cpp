






#include "nsHttp.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/net/HttpChannelChild.h"
#include "mozilla/net/CookieServiceChild.h"
#include "mozilla/net/WyciwygChannelChild.h"
#include "mozilla/net/FTPChannelChild.h"
#include "mozilla/net/WebSocketChannelChild.h"
#include "mozilla/net/RemoteOpenFileChild.h"
#include "mozilla/dom/network/TCPSocketChild.h"

using mozilla::dom::TCPSocketChild;

namespace mozilla {
namespace net {

PNeckoChild *gNeckoChild = nullptr;


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
    gNeckoChild = nullptr;
    alreadyDestroyed = true;
  }
}

PHttpChannelChild*
NeckoChild::AllocPHttpChannel(PBrowserChild* browser,
                              const SerializedLoadContext& loadContext)
{
  
  
  NS_NOTREACHED("AllocPHttpChannel should not be called on child");
  return nullptr;
}

bool 
NeckoChild::DeallocPHttpChannel(PHttpChannelChild* channel)
{
  NS_ABORT_IF_FALSE(IsNeckoChild(), "DeallocPHttpChannel called by non-child!");

  HttpChannelChild* child = static_cast<HttpChannelChild*>(channel);
  child->ReleaseIPDLReference();
  return true;
}

PFTPChannelChild*
NeckoChild::AllocPFTPChannel(PBrowserChild* aBrowser,
                             const SerializedLoadContext& aSerialized)
{
  
  NS_RUNTIMEABORT("AllocPFTPChannel should not be called");
  return nullptr;
}

bool
NeckoChild::DeallocPFTPChannel(PFTPChannelChild* channel)
{
  NS_ABORT_IF_FALSE(IsNeckoChild(), "DeallocPFTPChannel called by non-child!");

  FTPChannelChild* child = static_cast<FTPChannelChild*>(channel);
  child->ReleaseIPDLReference();
  return true;
}

PCookieServiceChild* 
NeckoChild::AllocPCookieService()
{
  
  NS_NOTREACHED("AllocPCookieService should not be called");
  return nullptr;
}

bool 
NeckoChild::DeallocPCookieService(PCookieServiceChild* cs)
{
  NS_ASSERTION(IsNeckoChild(), "DeallocPCookieService called by non-child!");

  CookieServiceChild *p = static_cast<CookieServiceChild*>(cs);
  p->Release();
  return true;
}

PWyciwygChannelChild*
NeckoChild::AllocPWyciwygChannel()
{
  WyciwygChannelChild *p = new WyciwygChannelChild();
  p->AddIPDLReference();
  return p;
}

bool
NeckoChild::DeallocPWyciwygChannel(PWyciwygChannelChild* channel)
{
  NS_ABORT_IF_FALSE(IsNeckoChild(), "DeallocPWyciwygChannel called by non-child!");

  WyciwygChannelChild *p = static_cast<WyciwygChannelChild*>(channel);
  p->ReleaseIPDLReference();
  return true;
}

PWebSocketChild*
NeckoChild::AllocPWebSocket(PBrowserChild* browser,
                            const SerializedLoadContext& aSerialized)
{
  NS_NOTREACHED("AllocPWebSocket should not be called");
  return nullptr;
}

bool
NeckoChild::DeallocPWebSocket(PWebSocketChild* child)
{
  WebSocketChannelChild* p = static_cast<WebSocketChannelChild*>(child);
  p->ReleaseIPDLReference();
  return true;
}

PTCPSocketChild*
NeckoChild::AllocPTCPSocket(const nsString& aHost,
                            const uint16_t& aPort,
                            const bool& useSSL,
                            const nsString& aBinaryType,
                            PBrowserChild* aBrowser)
{
  NS_NOTREACHED("AllocPTCPSocket should not be called");
  return nullptr;
}

bool
NeckoChild::DeallocPTCPSocket(PTCPSocketChild* child)
{
  TCPSocketChild* p = static_cast<TCPSocketChild*>(child);
  p->ReleaseIPDLReference();
  return true;
}

PRemoteOpenFileChild*
NeckoChild::AllocPRemoteOpenFile(const URIParams&, PBrowserChild*)
{
  
  
  NS_NOTREACHED("AllocPRemoteOpenFile should not be called on child");
  return nullptr;
}

bool
NeckoChild::DeallocPRemoteOpenFile(PRemoteOpenFileChild* aChild)
{
  RemoteOpenFileChild *p = static_cast<RemoteOpenFileChild*>(aChild);
  p->ReleaseIPDLReference();
  return true;
}

}} 

