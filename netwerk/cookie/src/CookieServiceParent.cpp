





































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

  mIOService = do_GetService(NS_IOSERVICE_CONTRACTID);
  NS_ASSERTION(mIOService, "couldn't get nsIIOService");
}

CookieServiceParent::~CookieServiceParent()
{
}

bool
CookieServiceParent::RecvGetCookieString(const nsCString& aHostSpec,
                                         const nsCString& aHostCharset,
                                         const nsCString& aOriginatingSpec,
                                         const nsCString& aOriginatingCharset,
                                         const bool& aFromHttp,
                                         nsCString* aResult)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI, originatingURI;
  NS_NewURI(getter_AddRefs(hostURI),
            aHostSpec, aHostCharset.get(),
            nsnull, mIOService);
  NS_NewURI(getter_AddRefs(originatingURI),
            aOriginatingSpec, aOriginatingCharset.get(),
            nsnull, mIOService);
  if (!hostURI)
    return false;

  mCookieService->GetCookieInternal(hostURI, originatingURI,
                                    aFromHttp, *aResult);
  return true;
}

bool
CookieServiceParent::RecvSetCookieString(const nsCString& aHostSpec,
                                         const nsCString& aHostCharset,
                                         const nsCString& aOriginatingSpec,
                                         const nsCString& aOriginatingCharset,
                                         const nsCString& aCookieString,
                                         const nsCString& aServerTime,
                                         const bool& aFromHttp)
{
  if (!mCookieService)
    return true;

  
  
  nsCOMPtr<nsIURI> hostURI, originatingURI;
  NS_NewURI(getter_AddRefs(hostURI),
            aHostSpec, aHostCharset.get(),
            nsnull, mIOService);
  NS_NewURI(getter_AddRefs(originatingURI),
            aOriginatingSpec, aOriginatingCharset.get(),
            nsnull, mIOService);
  if (!hostURI)
    return false;

  mCookieService->SetCookieStringInternal(hostURI, originatingURI,
                                          aCookieString, aServerTime,
                                          aFromHttp);
  return true;
}

}
}

