





































#include "mozilla/net/CookieServiceChild.h"
#include "mozilla/net/NeckoChild.h"
#include "nsIURI.h"

namespace mozilla {
namespace net {

static CookieServiceChild *gCookieService;

CookieServiceChild*
CookieServiceChild::GetSingleton()
{
  if (!gCookieService)
    gCookieService = new CookieServiceChild();

  NS_ADDREF(gCookieService);
  return gCookieService;
}

NS_IMPL_ISUPPORTS1(CookieServiceChild, nsICookieService)

CookieServiceChild::CookieServiceChild()
{
  NS_ASSERTION(IsNeckoChild(), "not a child process");

  
  NS_ADDREF_THIS();

  
  NeckoChild::InitNeckoChild();
  gNeckoChild->SendPCookieServiceConstructor(this);

  mPermissionService = do_GetService(NS_COOKIEPERMISSION_CONTRACTID);
  if (!mPermissionService)
    NS_WARNING("couldn't get nsICookiePermission in child");
}

CookieServiceChild::~CookieServiceChild()
{
  gCookieService = nsnull;
}

void
CookieServiceChild::SerializeURIs(nsIURI *aHostURI,
                                  nsIChannel *aChannel,
                                  nsCString &aHostSpec,
                                  nsCString &aHostCharset,
                                  nsCString &aOriginatingSpec,
                                  nsCString &aOriginatingCharset)
{
  
  
  
  aHostURI->GetSpec(aHostSpec);
  aHostURI->GetOriginCharset(aHostCharset);

  
  if (!mPermissionService) {
    NS_WARNING("nsICookiePermission unavailable! Cookie may be rejected");
    return;
  }

  nsCOMPtr<nsIURI> originatingURI;
  mPermissionService->GetOriginatingURI(aChannel,
                                        getter_AddRefs(originatingURI));
  if (originatingURI) {
    originatingURI->GetSpec(aOriginatingSpec);
    originatingURI->GetOriginCharset(aOriginatingSpec);
  }
}

nsresult
CookieServiceChild::GetCookieStringInternal(nsIURI *aHostURI,
                                            nsIChannel *aChannel,
                                            char **aCookieString,
                                            bool aFromHttp)
{
  NS_ENSURE_ARG(aHostURI);
  NS_ENSURE_ARG_POINTER(aCookieString);

  *aCookieString = NULL;

  nsCAutoString hostSpec, hostCharset, originatingSpec, originatingCharset;
  SerializeURIs(aHostURI, aChannel, hostSpec, hostCharset,
                originatingSpec, originatingCharset);

  
  nsCAutoString result;
  SendGetCookieString(hostSpec, hostCharset,
                      originatingSpec, originatingCharset,
                      aFromHttp, &result);
  if (!result.IsEmpty())
    *aCookieString = ToNewCString(result);

  return NS_OK;
}

nsresult
CookieServiceChild::SetCookieStringInternal(nsIURI *aHostURI,
                                            nsIChannel *aChannel,
                                            const char *aCookieString,
                                            const char *aServerTime,
                                            bool aFromHttp)
{
  NS_ENSURE_ARG(aHostURI);
  NS_ENSURE_ARG_POINTER(aCookieString);

  nsCAutoString hostSpec, hostCharset, originatingSpec, originatingCharset;
  SerializeURIs(aHostURI, aChannel, hostSpec, hostCharset,
                originatingSpec, originatingCharset);

  nsDependentCString cookieString(aCookieString);
  nsDependentCString serverTime;
  if (aServerTime)
    serverTime.Rebind(aServerTime);

  
  SendSetCookieString(hostSpec, hostCharset,
                      originatingSpec, originatingCharset,
                      cookieString, serverTime, aFromHttp);
  return NS_OK;
}

NS_IMETHODIMP
CookieServiceChild::GetCookieString(nsIURI *aHostURI,
                                    nsIChannel *aChannel,
                                    char **aCookieString)
{
  return GetCookieStringInternal(aHostURI, aChannel, aCookieString, false);
}

NS_IMETHODIMP
CookieServiceChild::GetCookieStringFromHttp(nsIURI *aHostURI,
                                            nsIURI *aFirstURI,
                                            nsIChannel *aChannel,
                                            char **aCookieString)
{
  return GetCookieStringInternal(aHostURI, aChannel, aCookieString, true);
}

NS_IMETHODIMP
CookieServiceChild::SetCookieString(nsIURI *aHostURI,
                                    nsIPrompt *aPrompt,
                                    const char *aCookieString,
                                    nsIChannel *aChannel)
{
  return SetCookieStringInternal(aHostURI, aChannel, aCookieString,
                                 nsnull, false);
}

NS_IMETHODIMP
CookieServiceChild::SetCookieStringFromHttp(nsIURI     *aHostURI,
                                            nsIURI     *aFirstURI,
                                            nsIPrompt  *aPrompt,
                                            const char *aCookieString,
                                            const char *aServerTime,
                                            nsIChannel *aChannel) 
{
  return SetCookieStringInternal(aHostURI, aChannel, aCookieString,
                                 aServerTime, true);
}

}
}

