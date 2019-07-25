





































#include "mozilla/net/CookieServiceParent.h"
#include "nsCookieService.h"
#include "nsNetUtil.h"

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
CookieServiceParent::RecvGetCookieString(const IPC::URI& aHost,
                                         const IPC::URI& aOriginating,
                                         const bool& aFromHttp,
                                         nsCString* aResult)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI(aHost);
  nsCOMPtr<nsIURI> originatingURI(aOriginating);
  if (!hostURI)
    return false;

  mCookieService->GetCookieInternal(hostURI, originatingURI,
                                    aFromHttp, *aResult);
  return true;
}

bool
CookieServiceParent::RecvSetCookieString(const IPC::URI& aHost,
                                         const IPC::URI& aOriginating,
                                         const nsCString& aCookieString,
                                         const nsCString& aServerTime,
                                         const bool& aFromHttp)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI(aHost);
  nsCOMPtr<nsIURI> originatingURI(aOriginating);
  if (!hostURI)
    return false;

  mCookieService->SetCookieStringInternal(hostURI, originatingURI,
                                          aCookieString, aServerTime,
                                          aFromHttp);
  return true;
}

}
}

