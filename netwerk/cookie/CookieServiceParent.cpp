




#include "mozilla/net/CookieServiceParent.h"

#include "mozilla/ipc/URIUtils.h"
#include "nsCookieService.h"
#include "nsNetUtil.h"

using namespace mozilla::ipc;

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
                                         nsCString* aResult)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI = DeserializeURI(aHost);
  if (!hostURI)
    return false;

  mCookieService->GetCookieStringInternal(hostURI, aIsForeign,
                                          aFromHttp, *aResult);
  return true;
}

bool
CookieServiceParent::RecvSetCookieString(const URIParams& aHost,
                                         const bool& aIsForeign,
                                         const nsCString& aCookieString,
                                         const nsCString& aServerTime,
                                         const bool& aFromHttp)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI = DeserializeURI(aHost);
  if (!hostURI)
    return false;

  nsDependentCString cookieString(aCookieString, 0);
  mCookieService->SetCookieStringInternal(hostURI, aIsForeign,
                                          cookieString, aServerTime,
                                          aFromHttp);
  return true;
}

}
}

