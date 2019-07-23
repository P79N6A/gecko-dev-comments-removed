











































#include "nsCookiePermission.h"
#include "nsICookie2.h"
#include "nsIServiceManager.h"
#include "nsICookiePromptService.h"
#include "nsICookieManager2.h"
#include "nsNetUtil.h"
#include "nsIURI.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIPrincipal.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsILoadContext.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsNetCID.h"










static const PRUint32 ACCEPT_NORMALLY = 0;
static const PRUint32 ASK_BEFORE_ACCEPT = 1;
static const PRUint32 ACCEPT_SESSION = 2;
static const PRUint32 ACCEPT_FOR_N_DAYS = 3;

static const PRBool kDefaultPolicy = PR_TRUE;
static const char kCookiesLifetimePolicy[] = "network.cookie.lifetimePolicy";
static const char kCookiesLifetimeDays[] = "network.cookie.lifetime.days";
static const char kCookiesAlwaysAcceptSession[] = "network.cookie.alwaysAcceptSessionCookies";

static const char kCookiesPrefsMigrated[] = "network.cookie.prefsMigrated";

static const char kCookiesLifetimeEnabled[] = "network.cookie.lifetime.enabled";
static const char kCookiesLifetimeBehavior[] = "network.cookie.lifetime.behavior";
static const char kCookiesAskPermission[] = "network.cookie.warnAboutCookies";

static const char kPermissionType[] = "cookie";

#ifdef MOZ_MAIL_NEWS


static PRBool
IsFromMailNews(nsIURI *aURI)
{
  static const char *kMailNewsProtocols[] =
      { "imap", "news", "snews", "mailbox", nsnull };
  PRBool result;
  for (const char **p = kMailNewsProtocols; *p; ++p) {
    if (NS_SUCCEEDED(aURI->SchemeIs(*p, &result)) && result)
      return PR_TRUE;
  }
  return PR_FALSE;
}
#endif

NS_IMPL_ISUPPORTS2(nsCookiePermission,
                   nsICookiePermission,
                   nsIObserver)

nsresult
nsCookiePermission::Init()
{
  nsresult rv;
  mPermMgr = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIPrefBranch2> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefBranch) {
    prefBranch->AddObserver(kCookiesLifetimePolicy, this, PR_FALSE);
    prefBranch->AddObserver(kCookiesLifetimeDays, this, PR_FALSE);
    prefBranch->AddObserver(kCookiesAlwaysAcceptSession, this, PR_FALSE);
    PrefChanged(prefBranch, nsnull);

    
    PRBool migrated;
    rv = prefBranch->GetBoolPref(kCookiesPrefsMigrated, &migrated);
    if (NS_FAILED(rv) || !migrated) {
      PRBool warnAboutCookies = PR_FALSE;
      prefBranch->GetBoolPref(kCookiesAskPermission, &warnAboutCookies);

      
      if (warnAboutCookies)
        prefBranch->SetIntPref(kCookiesLifetimePolicy, ASK_BEFORE_ACCEPT);
        
      PRBool lifetimeEnabled = PR_FALSE;
      prefBranch->GetBoolPref(kCookiesLifetimeEnabled, &lifetimeEnabled);
      
      
      
      if (lifetimeEnabled && !warnAboutCookies) {
        PRInt32 lifetimeBehavior;
        prefBranch->GetIntPref(kCookiesLifetimeBehavior, &lifetimeBehavior);
        if (lifetimeBehavior)
          prefBranch->SetIntPref(kCookiesLifetimePolicy, ACCEPT_FOR_N_DAYS);
        else
          prefBranch->SetIntPref(kCookiesLifetimePolicy, ACCEPT_SESSION);
      }
      prefBranch->SetBoolPref(kCookiesPrefsMigrated, PR_TRUE);
    }
  }

  return NS_OK;
}

void
nsCookiePermission::PrefChanged(nsIPrefBranch *aPrefBranch,
                                const char    *aPref)
{
  PRInt32 val;

#define PREF_CHANGED(_P) (!aPref || !strcmp(aPref, _P))

  if (PREF_CHANGED(kCookiesLifetimePolicy) &&
      NS_SUCCEEDED(aPrefBranch->GetIntPref(kCookiesLifetimePolicy, &val)))
    mCookiesLifetimePolicy = val;

  if (PREF_CHANGED(kCookiesLifetimeDays) &&
      NS_SUCCEEDED(aPrefBranch->GetIntPref(kCookiesLifetimeDays, &val)))
    
    mCookiesLifetimeSec = val * 24 * 60 * 60;

  if (PREF_CHANGED(kCookiesAlwaysAcceptSession) &&
      NS_SUCCEEDED(aPrefBranch->GetBoolPref(kCookiesAlwaysAcceptSession, &val)))
    mCookiesAlwaysAcceptSession = val;
}

NS_IMETHODIMP
nsCookiePermission::SetAccess(nsIURI         *aURI,
                              nsCookieAccess  aAccess)
{
  
  
  
  
  
  return mPermMgr->Add(aURI, kPermissionType, aAccess,
                       nsIPermissionManager::EXPIRE_NEVER, 0);
}

NS_IMETHODIMP
nsCookiePermission::CanAccess(nsIURI         *aURI,
                              nsIChannel     *aChannel,
                              nsCookieAccess *aResult)
{
#ifdef MOZ_MAIL_NEWS
  
  
  if (IsFromMailNews(aURI)) {
    *aResult = ACCESS_DENY;
    return NS_OK;
  }
#endif 
  
  
  nsresult rv = mPermMgr->TestPermission(aURI, kPermissionType, (PRUint32 *) aResult);
  if (NS_SUCCEEDED(rv)) {
    switch (*aResult) {
    
    case nsIPermissionManager::UNKNOWN_ACTION: 
    case nsIPermissionManager::ALLOW_ACTION:   
    case nsIPermissionManager::DENY_ACTION:    
      break;

    
    
    case nsICookiePermission::ACCESS_SESSION:
      *aResult = ACCESS_ALLOW;
      break;

    
    default:
      *aResult = ACCESS_DEFAULT;
    }
  }

  return rv;
}

NS_IMETHODIMP 
nsCookiePermission::CanSetCookie(nsIURI     *aURI,
                                 nsIChannel *aChannel,
                                 nsICookie2 *aCookie,
                                 PRBool     *aIsSession,
                                 PRInt64    *aExpiry,
                                 PRBool     *aResult)
{
  NS_ASSERTION(aURI, "null uri");

  *aResult = kDefaultPolicy;

  PRUint32 perm;
  mPermMgr->TestPermission(aURI, kPermissionType, &perm);
  switch (perm) {
  case nsICookiePermission::ACCESS_SESSION:
    *aIsSession = PR_TRUE;

  case nsIPermissionManager::ALLOW_ACTION: 
    *aResult = PR_TRUE;
    break;

  case nsIPermissionManager::DENY_ACTION:  
    *aResult = PR_FALSE;
    break;

  default:
    
    
    NS_ASSERTION(perm == nsIPermissionManager::UNKNOWN_ACTION, "unknown permission");
    
    
    
    if (mCookiesLifetimePolicy == ACCEPT_NORMALLY) {
      *aResult = PR_TRUE;
      return NS_OK;
    }
    
    
    PRInt64 currentTime = PR_Now() / PR_USEC_PER_SEC;
    PRInt64 delta = *aExpiry - currentTime;
    
    
    if (mCookiesLifetimePolicy == ASK_BEFORE_ACCEPT) {
      
      
      
      if ((*aIsSession && mCookiesAlwaysAcceptSession) ||
          InPrivateBrowsing()) {
        *aResult = PR_TRUE;
        return NS_OK;
      }
      
      
      *aResult = PR_FALSE;

      nsCAutoString hostPort;
      aURI->GetHostPort(hostPort);

      if (!aCookie) {
         return NS_ERROR_UNEXPECTED;
      }
      
      
      
      
      if (hostPort.IsEmpty()) {
        aURI->GetScheme(hostPort);
        if (hostPort.IsEmpty()) {
          
          return NS_OK;
        }
        hostPort = hostPort + NS_LITERAL_CSTRING("://");
      }

      
      
      nsresult rv;
      nsCOMPtr<nsICookiePromptService> cookiePromptService =
          do_GetService(NS_COOKIEPROMPTSERVICE_CONTRACTID, &rv);
      if (NS_FAILED(rv)) return rv;

      
      nsCOMPtr<nsIDOMWindow> parent;
      if (aChannel) {
        nsCOMPtr<nsILoadContext> ctx;
        NS_QueryNotificationCallbacks(aChannel, ctx);
        if (ctx) {
          ctx->GetAssociatedWindow(getter_AddRefs(parent));
        }
      }

      
      
      
      PRBool foundCookie = PR_FALSE;
      PRUint32 countFromHost;
      nsCOMPtr<nsICookieManager2> cookieManager = do_GetService(NS_COOKIEMANAGER_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv)) {
        nsCAutoString rawHost;
        aCookie->GetRawHost(rawHost);
        rv = cookieManager->CountCookiesFromHost(rawHost, &countFromHost);

        if (NS_SUCCEEDED(rv) && countFromHost > 0)
          rv = cookieManager->CookieExists(aCookie, &foundCookie);
      }
      if (NS_FAILED(rv)) return rv;

      
      
      
      if (!foundCookie && !*aIsSession && delta <= 0) {
        
        
        *aResult = PR_TRUE;
        return rv;
      }

      PRBool rememberDecision = PR_FALSE;
      rv = cookiePromptService->CookieDialog(parent, aCookie, hostPort, 
                                             countFromHost, foundCookie,
                                             &rememberDecision, aResult);
      if (NS_FAILED(rv)) return rv;
      
      if (*aResult == nsICookiePromptService::ACCEPT_SESSION_COOKIE)
        *aIsSession = PR_TRUE;

      if (rememberDecision) {
        switch (*aResult) {
          case nsICookiePromptService::DENY_COOKIE:
            mPermMgr->Add(aURI, kPermissionType, (PRUint32) nsIPermissionManager::DENY_ACTION,
                          nsIPermissionManager::EXPIRE_NEVER, 0);
            break;
          case nsICookiePromptService::ACCEPT_COOKIE:
            mPermMgr->Add(aURI, kPermissionType, (PRUint32) nsIPermissionManager::ALLOW_ACTION,
                          nsIPermissionManager::EXPIRE_NEVER, 0);
            break;
          case nsICookiePromptService::ACCEPT_SESSION_COOKIE:
            mPermMgr->Add(aURI, kPermissionType, nsICookiePermission::ACCESS_SESSION,
                          nsIPermissionManager::EXPIRE_NEVER, 0);
            break;
          default:
            break;
        }
      }
    } else {
      
      
      if (!*aIsSession && delta > 0) {
        if (mCookiesLifetimePolicy == ACCEPT_SESSION) {
          
          *aIsSession = PR_TRUE;
        } else if (delta > mCookiesLifetimeSec) {
          
          *aExpiry = currentTime + mCookiesLifetimeSec;
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsCookiePermission::GetOriginatingURI(nsIChannel  *aChannel,
                                      nsIURI     **aURI)
{
  
























  *aURI = nsnull;

  
  if (!aChannel)
    return NS_ERROR_NULL_POINTER;

  
  nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal = do_QueryInterface(aChannel);
  if (httpChannelInternal)
  {
    PRBool doForce = PR_FALSE;
    if (NS_SUCCEEDED(httpChannelInternal->GetForceAllowThirdPartyCookie(&doForce)) && doForce)
    {
      
      aChannel->GetURI(aURI);
      if (!*aURI)
        return NS_ERROR_NULL_POINTER;

      return NS_OK;
    }
  }

  
  nsCOMPtr<nsILoadContext> ctx;
  NS_QueryNotificationCallbacks(aChannel, ctx);
  nsCOMPtr<nsIDOMWindow> topWin, ourWin;
  if (ctx) {
    ctx->GetTopWindow(getter_AddRefs(topWin));
    ctx->GetAssociatedWindow(getter_AddRefs(ourWin));
  }

  
  if (!topWin)
    return NS_ERROR_INVALID_ARG;

  
  if (ourWin == topWin) {
    
    
    
    nsLoadFlags flags;
    aChannel->GetLoadFlags(&flags);

    if (flags & nsIChannel::LOAD_DOCUMENT_URI) {
      
      aChannel->GetURI(aURI);
      if (!*aURI)
        return NS_ERROR_NULL_POINTER;

      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIScriptObjectPrincipal> scriptObjPrin = do_QueryInterface(topWin);
  NS_ENSURE_TRUE(scriptObjPrin, NS_ERROR_UNEXPECTED);

  nsIPrincipal* prin = scriptObjPrin->GetPrincipal();
  NS_ENSURE_TRUE(prin, NS_ERROR_UNEXPECTED);
  
  prin->GetURI(aURI);

  if (!*aURI)
    return NS_ERROR_NULL_POINTER;

  
  return NS_OK;
}

NS_IMETHODIMP
nsCookiePermission::Observe(nsISupports     *aSubject,
                            const char      *aTopic,
                            const PRUnichar *aData)
{
  nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject);
  NS_ASSERTION(!nsCRT::strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic),
               "unexpected topic - we only deal with pref changes!");

  if (prefBranch)
    PrefChanged(prefBranch, NS_LossyConvertUTF16toASCII(aData).get());
  return NS_OK;
}

PRBool
nsCookiePermission::InPrivateBrowsing()
{
  PRBool inPrivateBrowsingMode = PR_FALSE;
  if (!mPBService)
    mPBService = do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (mPBService)
    mPBService->GetPrivateBrowsingEnabled(&inPrivateBrowsingMode);
  return inPrivateBrowsingMode;
}
