






#include "nsHttp.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/net/CookieServiceParent.h"
#include "mozilla/net/WyciwygChannelParent.h"
#include "mozilla/net/FTPChannelParent.h"
#include "mozilla/net/WebSocketChannelParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/network/TCPSocketParent.h"
#include "mozilla/Preferences.h"
#include "mozilla/LoadContext.h"
#include "nsPrintfCString.h"
#include "nsHTMLDNSPrefetch.h"

using mozilla::dom::TabParent;
using mozilla::net::PTCPSocketParent;
using mozilla::dom::TCPSocketParent;
using IPC::SerializedLoadContext;

namespace mozilla {
namespace net {

static bool gDisableIPCSecurity = false;
static const char kPrefDisableIPCSecurity[] = "network.disable.ipc.security";


NeckoParent::NeckoParent()
{
  Preferences::AddBoolVarCache(&gDisableIPCSecurity, kPrefDisableIPCSecurity);
}

NeckoParent::~NeckoParent()
{
}

static PBOverrideStatus
PBOverrideStatusFromLoadContext(const SerializedLoadContext& aSerialized)
{
  if (!aSerialized.IsNotNull() && aSerialized.IsPrivateBitValid()) {
    return aSerialized.mUsePrivateBrowsing ?
      kPBOverride_Private :
      kPBOverride_NotPrivate;
  }
  return kPBOverride_Unset;
}

const char*
NeckoParent::GetValidatedAppInfo(const SerializedLoadContext& aSerialized,
                                 PBrowserParent* aBrowser,
                                 uint32_t* aAppId,
                                 bool* aInBrowserElement)
{
  if (!gDisableIPCSecurity) {
    if (!aBrowser) {
      return "missing required PBrowser argument";
    }
    if (!aSerialized.IsNotNull()) {
      return "SerializedLoadContext from child is null";
    }
  }

  *aAppId = NECKO_UNKNOWN_APP_ID;
  *aInBrowserElement = false;

  if (aBrowser) {
    nsRefPtr<TabParent> tabParent = static_cast<TabParent*>(aBrowser);

    *aAppId = tabParent->OwnOrContainingAppId();
    *aInBrowserElement = tabParent->IsBrowserElement();

    if (*aAppId == NECKO_UNKNOWN_APP_ID) {
      return "TabParent reports appId=NECKO_UNKNOWN_APP_ID!";
    }
    
    if (*aAppId == NECKO_NO_APP_ID) {
      if (tabParent->HasOwnApp()) {
        return "TabParent reports NECKO_NO_APP_ID but also is an app";
      }
      if (tabParent->IsBrowserElement()) {
        
        
        
        
        return "TabParent reports appId=NECKO_NO_APP_ID but is a mozbrowser";
      }
    }
  } else {
    
    
    MOZ_ASSERT(gDisableIPCSecurity);
    if (!gDisableIPCSecurity) {
      return "internal error";
    }
    if (aSerialized.IsNotNull()) {
      *aAppId = aSerialized.mAppId;
      *aInBrowserElement = aSerialized.mIsInBrowserElement;
    } else {
      *aAppId = NECKO_NO_APP_ID;
    }
  }
  return nullptr;
}

const char *
NeckoParent::CreateChannelLoadContext(PBrowserParent* aBrowser,
                                      const SerializedLoadContext& aSerialized,
                                      nsCOMPtr<nsILoadContext> &aResult)
{
  uint32_t appId = NECKO_UNKNOWN_APP_ID;
  bool inBrowser = false;
  nsIDOMElement* topFrameElement = nullptr;
  const char* error = GetValidatedAppInfo(aSerialized, aBrowser, &appId, &inBrowser);
  if (error) {
    return error;
  }

  if (aBrowser) {
    nsRefPtr<TabParent> tabParent = static_cast<TabParent*>(aBrowser);
    topFrameElement = tabParent->GetOwnerElement();
  }

  
  
  if (aSerialized.IsNotNull()) {
    aResult = new LoadContext(aSerialized, topFrameElement, appId, inBrowser);
  }

  return nullptr;
}

PHttpChannelParent*
NeckoParent::AllocPHttpChannel(PBrowserParent* aBrowser,
                               const SerializedLoadContext& aSerialized)
{
  nsCOMPtr<nsILoadContext> loadContext;
  const char *error = CreateChannelLoadContext(aBrowser, aSerialized,
                                               loadContext);
  if (error) {
    NS_WARNING(nsPrintfCString("NeckoParent::AllocPHttpChannel: "
                               "FATAL error: %s: KILLING CHILD PROCESS\n",
                               error).get());
    return nullptr;
  }
  PBOverrideStatus overrideStatus = PBOverrideStatusFromLoadContext(aSerialized);
  HttpChannelParent *p = new HttpChannelParent(aBrowser, loadContext, overrideStatus);
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

PFTPChannelParent*
NeckoParent::AllocPFTPChannel(PBrowserParent* aBrowser,
                              const SerializedLoadContext& aSerialized)
{
  nsCOMPtr<nsILoadContext> loadContext;
  const char *error = CreateChannelLoadContext(aBrowser, aSerialized,
                                               loadContext);
  if (error) {
    NS_WARNING(nsPrintfCString("NeckoParent::AllocPFTPChannel: "
                               "FATAL error: %s: KILLING CHILD PROCESS\n",
                               error).get());
    return nullptr;
  }
  PBOverrideStatus overrideStatus = PBOverrideStatusFromLoadContext(aSerialized);
  FTPChannelParent *p = new FTPChannelParent(loadContext, overrideStatus);
  p->AddRef();
  return p;
}

bool
NeckoParent::DeallocPFTPChannel(PFTPChannelParent* channel)
{
  FTPChannelParent *p = static_cast<FTPChannelParent *>(channel);
  p->Release();
  return true;
}

PCookieServiceParent* 
NeckoParent::AllocPCookieService()
{
  return new CookieServiceParent();
}

bool 
NeckoParent::DeallocPCookieService(PCookieServiceParent* cs)
{
  delete cs;
  return true;
}

PWyciwygChannelParent*
NeckoParent::AllocPWyciwygChannel()
{
  WyciwygChannelParent *p = new WyciwygChannelParent();
  p->AddRef();
  return p;
}

bool
NeckoParent::DeallocPWyciwygChannel(PWyciwygChannelParent* channel)
{
  WyciwygChannelParent *p = static_cast<WyciwygChannelParent *>(channel);
  p->Release();
  return true;
}

PWebSocketParent*
NeckoParent::AllocPWebSocket(PBrowserParent* browser,
                             const SerializedLoadContext& serialized)
{
  nsCOMPtr<nsILoadContext> loadContext;
  const char *error = CreateChannelLoadContext(browser, serialized,
                                               loadContext);
  if (error) {
    NS_WARNING(nsPrintfCString("NeckoParent::AllocPWebSocket: "
                               "FATAL error: %s: KILLING CHILD PROCESS\n",
                               error).get());
    return nullptr;
  }

  TabParent* tabParent = static_cast<TabParent*>(browser);
  PBOverrideStatus overrideStatus = PBOverrideStatusFromLoadContext(serialized);
  WebSocketChannelParent* p = new WebSocketChannelParent(tabParent, loadContext,
                                                         overrideStatus);
  p->AddRef();
  return p;
}

bool
NeckoParent::DeallocPWebSocket(PWebSocketParent* actor)
{
  WebSocketChannelParent* p = static_cast<WebSocketChannelParent*>(actor);
  p->Release();
  return true;
}

PTCPSocketParent*
NeckoParent::AllocPTCPSocket(const nsString& aHost,
                             const uint16_t& aPort,
                             const bool& useSSL,
                             const nsString& aBinaryType,
                             PBrowserParent* aBrowser)
{
  TCPSocketParent* p = new TCPSocketParent();
  p->AddRef();
  return p;
}

bool
NeckoParent::RecvPTCPSocketConstructor(PTCPSocketParent* aActor,
                                       const nsString& aHost,
                                       const uint16_t& aPort,
                                       const bool& useSSL,
                                       const nsString& aBinaryType,
                                       PBrowserParent* aBrowser)
{
  return static_cast<TCPSocketParent*>(aActor)->
      Init(aHost, aPort, useSSL, aBinaryType, aBrowser);
}

bool
NeckoParent::DeallocPTCPSocket(PTCPSocketParent* actor)
{
  TCPSocketParent* p = static_cast<TCPSocketParent*>(actor);
  p->Release();
  return true;
}

bool
NeckoParent::RecvHTMLDNSPrefetch(const nsString& hostname,
                                 const uint16_t& flags)
{
  nsHTMLDNSPrefetch::Prefetch(hostname, flags);
  return true;
}

bool
NeckoParent::RecvCancelHTMLDNSPrefetch(const nsString& hostname,
                                 const uint16_t& flags,
                                 const nsresult& reason)
{
  nsHTMLDNSPrefetch::CancelPrefetch(hostname, flags, reason);
  return true;
}

}} 

