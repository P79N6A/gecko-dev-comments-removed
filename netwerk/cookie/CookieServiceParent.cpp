




#include "mozilla/net/CookieServiceParent.h"
#include "mozilla/dom/PContentParent.h"
#include "mozilla/net/NeckoParent.h"

#include "mozilla/ipc/URIUtils.h"
#include "nsCookieService.h"
#include "nsNetCID.h"
#include "nsPrintfCString.h"
#include "SerializedLoadContext.h"

using namespace mozilla::ipc;
using mozilla::dom::PContentParent;
using mozilla::net::NeckoParent;

namespace mozilla {
namespace net {

MOZ_WARN_UNUSED_RESULT
bool
CookieServiceParent::GetAppInfoFromParams(const IPC::SerializedLoadContext &aLoadContext,
                                          uint32_t& aAppId,
                                          bool& aIsInBrowserElement,
                                          bool& aIsPrivate)
{
  aAppId = NECKO_NO_APP_ID;
  aIsInBrowserElement = false;
  aIsPrivate = false;

  const char* error = NeckoParent::GetValidatedAppInfo(aLoadContext,
                                                       Manager()->Manager(),
                                                       &aAppId,
                                                       &aIsInBrowserElement);
  if (error) {
    NS_WARNING(nsPrintfCString("CookieServiceParent: GetAppInfoFromParams: "
                               "FATAL error: %s: KILLING CHILD PROCESS\n",
                               error).get());
    return false;
  }

  if (aLoadContext.IsPrivateBitValid()) {
    aIsPrivate = aLoadContext.mUsePrivateBrowsing;
  }
  return true;
}

CookieServiceParent::CookieServiceParent()
{
  
  
  nsCOMPtr<nsICookieService> cs = do_GetService(NS_COOKIESERVICE_CONTRACTID);

  
  mCookieService =
    already_AddRefed<nsCookieService>(nsCookieService::GetSingleton());
  NS_ASSERTION(mCookieService, "couldn't get nsICookieService");
}

CookieServiceParent::~CookieServiceParent()
{
}

void
CookieServiceParent::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

bool
CookieServiceParent::RecvGetCookieString(const URIParams& aHost,
                                         const bool& aIsForeign,
                                         const bool& aFromHttp,
                                         const IPC::SerializedLoadContext&
                                               aLoadContext,
                                         nsCString* aResult)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI = DeserializeURI(aHost);
  if (!hostURI)
    return false;

  uint32_t appId;
  bool isInBrowserElement, isPrivate;
  bool valid = GetAppInfoFromParams(aLoadContext, appId,
                                    isInBrowserElement, isPrivate);
  if (!valid) {
    return false;
  }

  mCookieService->GetCookieStringInternal(hostURI, aIsForeign, aFromHttp, appId,
                                          isInBrowserElement, isPrivate, *aResult);
  return true;
}

bool
CookieServiceParent::RecvSetCookieString(const URIParams& aHost,
                                         const bool& aIsForeign,
                                         const nsCString& aCookieString,
                                         const nsCString& aServerTime,
                                         const bool& aFromHttp,
                                         const IPC::SerializedLoadContext&
                                               aLoadContext)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI = DeserializeURI(aHost);
  if (!hostURI)
    return false;

  uint32_t appId;
  bool isInBrowserElement, isPrivate;
  bool valid = GetAppInfoFromParams(aLoadContext, appId,
                                    isInBrowserElement, isPrivate);
  if (!valid) {
    return false;
  }

  nsDependentCString cookieString(aCookieString, 0);
  
  mCookieService->SetCookieStringInternal(hostURI, aIsForeign, cookieString,
                                          aServerTime, aFromHttp, appId,
                                          isInBrowserElement, isPrivate, nullptr);
  return true;
}

mozilla::ipc::IProtocol*
CookieServiceParent::CloneProtocol(Channel* aChannel,
                                   mozilla::ipc::ProtocolCloneContext* aCtx)
{
  NeckoParent* manager = aCtx->GetNeckoParent();
  nsAutoPtr<PCookieServiceParent> actor(manager->AllocPCookieServiceParent());
  if (!actor || !manager->RecvPCookieServiceConstructor(actor)) {
    return nullptr;
  }
  return actor.forget();
}

}
}

