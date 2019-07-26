




#include "mozilla/net/CookieServiceParent.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/net/NeckoParent.h"

#include "mozilla/ipc/URIUtils.h"
#include "nsCookieService.h"
#include "nsNetUtil.h"
#include "nsPrintfCString.h"

using namespace mozilla::ipc;
using mozilla::dom::PBrowserParent;
using mozilla::net::NeckoParent;

namespace mozilla {
namespace net {

MOZ_WARN_UNUSED_RESULT
static bool
GetAppInfoFromParams(const IPC::SerializedLoadContext &aLoadContext,
                     PBrowserParent* aBrowser,
                     uint32_t& aAppId,
                     bool& aIsInBrowserElement,
                     bool& aIsPrivate)
{
  aAppId = NECKO_NO_APP_ID;
  aIsInBrowserElement = false;
  aIsPrivate = false;

  const char* error = NeckoParent::GetValidatedAppInfo(aLoadContext, aBrowser,
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

bool
CookieServiceParent::RecvGetCookieString(const URIParams& aHost,
                                         const bool& aIsForeign,
                                         const bool& aFromHttp,
                                         const IPC::SerializedLoadContext&
                                               aLoadContext,
                                         PBrowserParent* aBrowser,
                                         nsCString* aResult)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI = DeserializeURI(aHost);
  if (!hostURI)
    return false;

  uint32_t appId;
  bool isInBrowserElement, isPrivate;
  bool valid = GetAppInfoFromParams(aLoadContext, aBrowser, appId,
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
                                               aLoadContext,
                                         PBrowserParent* aBrowser)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI = DeserializeURI(aHost);
  if (!hostURI)
    return false;

  uint32_t appId;
  bool isInBrowserElement, isPrivate;
  bool valid = GetAppInfoFromParams(aLoadContext, aBrowser, appId,
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

}
}

