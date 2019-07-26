






#include "nsHttp.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/net/CookieServiceParent.h"
#include "mozilla/net/WyciwygChannelParent.h"
#include "mozilla/net/FTPChannelParent.h"
#include "mozilla/net/WebSocketChannelParent.h"
#include "mozilla/net/RemoteOpenFileParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/network/TCPSocketParent.h"
#include "mozilla/ipc/URIUtils.h"
#include "mozilla/Preferences.h"

#include "nsHTMLDNSPrefetch.h"
#include "nsIAppsService.h"
#include "nsEscape.h"

using mozilla::dom::TabParent;
using mozilla::net::PTCPSocketParent;
using mozilla::dom::TCPSocketParent;

namespace mozilla {
namespace net {

static bool gDisableIPCSecurity = false;
static const char kPrefDisableIPCSecurity[] = "network.disable.ipc.security";


NeckoParent::NeckoParent()
{
  Preferences::AddBoolVarCache(&gDisableIPCSecurity, kPrefDisableIPCSecurity);

  if (!gDisableIPCSecurity) {
    
    nsAutoString corePath, webPath;
    nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
    if (appsService) {
      appsService->GetCoreAppsBasePath(corePath);
      appsService->GetWebAppsBasePath(webPath);
    }
    
    MOZ_ASSERT(!webPath.IsEmpty());

    LossyCopyUTF16toASCII(corePath, mCoreAppsBasePath);
    LossyCopyUTF16toASCII(webPath, mWebAppsBasePath);
  }
}

NeckoParent::~NeckoParent()
{
}

PHttpChannelParent*
NeckoParent::AllocPHttpChannel(PBrowserParent* browser,
                               const SerializedLoadContext& loadContext)
{
  HttpChannelParent *p = new HttpChannelParent(browser, loadContext);
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
NeckoParent::AllocPFTPChannel()
{
  FTPChannelParent *p = new FTPChannelParent();
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
NeckoParent::AllocPWebSocket(PBrowserParent* browser)
{
  TabParent* tabParent = static_cast<TabParent*>(browser);
  WebSocketChannelParent* p = new WebSocketChannelParent(tabParent);
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

PRemoteOpenFileParent*
NeckoParent::AllocPRemoteOpenFile(const URIParams& aURI,
                                  PBrowserParent* aBrowser)
{
  nsCOMPtr<nsIURI> uri = DeserializeURI(aURI);
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(uri);
  if (!fileURL) {
    return nullptr;
  }

  
  if (!gDisableIPCSecurity) {
    if (!aBrowser) {
      NS_WARNING("NeckoParent::AllocPRemoteOpenFile: "
                 "FATAL error: missing TabParent: KILLING CHILD PROCESS\n");
      return nullptr;
    }
    nsRefPtr<TabParent> tabParent = static_cast<TabParent*>(aBrowser);
    uint32_t appId = tabParent->OwnOrContainingAppId();
    nsCOMPtr<nsIAppsService> appsService =
      do_GetService(APPS_SERVICE_CONTRACTID);
    if (!appsService) {
      return nullptr;
    }
    nsCOMPtr<mozIDOMApplication> domApp;
    nsresult rv = appsService->GetAppByLocalId(appId, getter_AddRefs(domApp));
    if (!domApp) {
      return nullptr;
    }
    nsCOMPtr<mozIApplication> mozApp = do_QueryInterface(domApp);
    if (!mozApp) {
      return nullptr;
    }
    bool hasManage = false;
    rv = mozApp->HasPermission("webapps-manage", &hasManage);
    if (NS_FAILED(rv)) {
      return nullptr;
    }

    nsAutoCString requestedPath;
    fileURL->GetPath(requestedPath);
    NS_UnescapeURL(requestedPath);

    if (hasManage) {
      
      
      
      NS_NAMED_LITERAL_CSTRING(appzip, "/application.zip");
      nsAutoCString pathEnd;
      requestedPath.Right(pathEnd, appzip.Length());
      if (!pathEnd.Equals(appzip)) {
        return nullptr;
      }
      nsAutoCString pathStart;
      requestedPath.Left(pathStart, mWebAppsBasePath.Length());
      if (!pathStart.Equals(mWebAppsBasePath)) {
        if (mCoreAppsBasePath.IsEmpty()) {
          return nullptr;
        }
        requestedPath.Left(pathStart, mCoreAppsBasePath.Length());
        if (!pathStart.Equals(mCoreAppsBasePath)) {
          return nullptr;
        }
      }
      
      
      
      
      if (PL_strnstr(requestedPath.BeginReading(), "/../",
                     requestedPath.Length())) {
        NS_WARNING("NeckoParent::AllocPRemoteOpenFile: "
                   "FATAL error: requested file URI contains '/../' "
                   "KILLING CHILD PROCESS\n");
        return nullptr;
      }
    } else {
      
      nsAutoString basePath;
      rv = mozApp->GetBasePath(basePath);
      if (NS_FAILED(rv)) {
        return nullptr;
      }
      nsAutoString uuid;
      rv = mozApp->GetId(uuid);
      if (NS_FAILED(rv)) {
        return nullptr;
      }
      nsPrintfCString mustMatch("%s/%s/application.zip",
                                NS_LossyConvertUTF16toASCII(basePath).get(),
                                NS_LossyConvertUTF16toASCII(uuid).get());
      if (!requestedPath.Equals(mustMatch)) {
        NS_WARNING("NeckoParent::AllocPRemoteOpenFile: "
                   "FATAL error: requesting file other than application.zip: "
                   "KILLING CHILD PROCESS\n");
        return nullptr;
      }
    }
  }

  RemoteOpenFileParent* parent = new RemoteOpenFileParent(fileURL);
  return parent;
}

bool
NeckoParent::DeallocPRemoteOpenFile(PRemoteOpenFileParent* actor)
{
  delete actor;
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

