




#include "mozilla/net/CookieServiceChild.h"
#include "mozilla/ipc/URIUtils.h"
#include "mozilla/net/NeckoChild.h"
#include "nsIURI.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsServiceManagerUtils.h"
#include "SerializedLoadContext.h"

using namespace mozilla::ipc;

namespace mozilla {
namespace net {


static const char kPrefCookieBehavior[] = "network.cookie.cookieBehavior";
static const char kPrefThirdPartySession[] =
  "network.cookie.thirdparty.sessionOnly";

static CookieServiceChild *gCookieService;

CookieServiceChild*
CookieServiceChild::GetSingleton()
{
  if (!gCookieService)
    gCookieService = new CookieServiceChild();

  NS_ADDREF(gCookieService);
  return gCookieService;
}

NS_IMPL_ISUPPORTS(CookieServiceChild,
                  nsICookieService,
                  nsIObserver,
                  nsISupportsWeakReference)

CookieServiceChild::CookieServiceChild()
  : mCookieBehavior(nsICookieService::BEHAVIOR_ACCEPT)
  , mThirdPartySession(false)
{
  NS_ASSERTION(IsNeckoChild(), "not a child process");

  
  NS_ADDREF_THIS();

  
  NeckoChild::InitNeckoChild();
  gNeckoChild->SendPCookieServiceConstructor(this);

  
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_WARN_IF_FALSE(prefBranch, "no prefservice");
  if (prefBranch) {
    prefBranch->AddObserver(kPrefCookieBehavior, this, true);
    prefBranch->AddObserver(kPrefThirdPartySession, this, true);
    PrefChanged(prefBranch);
  }
}

CookieServiceChild::~CookieServiceChild()
{
  gCookieService = nullptr;
}

void
CookieServiceChild::PrefChanged(nsIPrefBranch *aPrefBranch)
{
  int32_t val;
  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookieBehavior, &val)))
    mCookieBehavior =
      val >= nsICookieService::BEHAVIOR_ACCEPT &&
      val <= nsICookieService::BEHAVIOR_LIMIT_FOREIGN
        ? val : nsICookieService::BEHAVIOR_ACCEPT;

  bool boolval;
  if (NS_SUCCEEDED(aPrefBranch->GetBoolPref(kPrefThirdPartySession, &boolval)))
    mThirdPartySession = !!boolval;

  if (!mThirdPartyUtil && RequireThirdPartyCheck()) {
    mThirdPartyUtil = do_GetService(THIRDPARTYUTIL_CONTRACTID);
    NS_ASSERTION(mThirdPartyUtil, "require ThirdPartyUtil service");
  }
}

bool
CookieServiceChild::RequireThirdPartyCheck()
{
  return mCookieBehavior == nsICookieService::BEHAVIOR_REJECT_FOREIGN ||
    mCookieBehavior == nsICookieService::BEHAVIOR_LIMIT_FOREIGN ||
    mThirdPartySession;
}

nsresult
CookieServiceChild::GetCookieStringInternal(nsIURI *aHostURI,
                                            nsIChannel *aChannel,
                                            char **aCookieString,
                                            bool aFromHttp)
{
  NS_ENSURE_ARG(aHostURI);
  NS_ENSURE_ARG_POINTER(aCookieString);

  *aCookieString = nullptr;

  
  
  nsAutoCString scheme;
  aHostURI->GetScheme(scheme);
  if (scheme.EqualsLiteral("moz-nullprincipal"))
    return NS_OK;

  
  bool isForeign = true;
  if (RequireThirdPartyCheck())
    mThirdPartyUtil->IsThirdPartyChannel(aChannel, aHostURI, &isForeign);

  URIParams uriParams;
  SerializeURI(aHostURI, uriParams);

  
  nsAutoCString result;
  SendGetCookieString(uriParams, !!isForeign, aFromHttp,
                      IPC::SerializedLoadContext(aChannel), &result);
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

  
  
  nsAutoCString scheme;
  aHostURI->GetScheme(scheme);
  if (scheme.EqualsLiteral("moz-nullprincipal"))
    return NS_OK;

  
  bool isForeign = true;
  if (RequireThirdPartyCheck())
    mThirdPartyUtil->IsThirdPartyChannel(aChannel, aHostURI, &isForeign);

  nsDependentCString cookieString(aCookieString);
  nsDependentCString serverTime;
  if (aServerTime)
    serverTime.Rebind(aServerTime);

  URIParams uriParams;
  SerializeURI(aHostURI, uriParams);

  
  SendSetCookieString(uriParams, !!isForeign, cookieString, serverTime,
                      aFromHttp, IPC::SerializedLoadContext(aChannel));
  return NS_OK;
}

NS_IMETHODIMP
CookieServiceChild::Observe(nsISupports     *aSubject,
                            const char      *aTopic,
                            const char16_t *aData)
{
  NS_ASSERTION(strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0,
               "not a pref change topic!");

  nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject);
  if (prefBranch)
    PrefChanged(prefBranch);
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
                                 nullptr, false);
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

