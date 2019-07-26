




#include "mozilla/net/CookieServiceParent.h"

#include "mozilla/ipc/URIUtils.h"
#include "nsCookieService.h"
#include "nsNetUtil.h"

using namespace mozilla::ipc;

static void
GetAppInfoFromLoadContext(const IPC::SerializedLoadContext &aLoadContext,
                          uint32_t& aAppId,
                          bool& aIsInBrowserElement,
                          bool& aIsPrivate)
{
  
  
  aAppId = NECKO_NO_APP_ID;
  aIsInBrowserElement = false;
  aIsPrivate = false;

  if (aLoadContext.IsNotNull()) {
    aAppId = aLoadContext.mAppId;
    aIsInBrowserElement = aLoadContext.mIsInBrowserElement;
  }

  if (aLoadContext.IsPrivateBitValid())
    aIsPrivate = aLoadContext.mUsePrivateBrowsing;
}

namespace mozilla {
namespace net {

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
                                         nsCString* aResult)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI = DeserializeURI(aHost);
  if (!hostURI)
    return false;

  uint32_t appId;
  bool isInBrowserElement, isPrivate;
  GetAppInfoFromLoadContext(aLoadContext, appId, isInBrowserElement, isPrivate);

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
  GetAppInfoFromLoadContext(aLoadContext, appId, isInBrowserElement, isPrivate);

  nsDependentCString cookieString(aCookieString, 0);
  
  mCookieService->SetCookieStringInternal(hostURI, aIsForeign, cookieString,
                                          aServerTime, aFromHttp, appId,
                                          isInBrowserElement, isPrivate, nullptr);
  return true;
}

}
}

