



#include "nsContentBlocker.h"
#include "nsIContent.h"
#include "nsIURI.h"
#include "nsIServiceManager.h"
#include "nsIDocShellTreeItem.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIDocShell.h"
#include "nsString.h"
#include "nsContentPolicyUtils.h"
#include "nsIObjectLoadingContent.h"



#define BEHAVIOR_ACCEPT nsIPermissionManager::ALLOW_ACTION
#define BEHAVIOR_REJECT nsIPermissionManager::DENY_ACTION
#define BEHAVIOR_NOFOREIGN 3


static const char *kTypeString[] = {"other",
                                    "script",
                                    "image",
                                    "stylesheet",
                                    "object",
                                    "document",
                                    "subdocument",
                                    "refresh",
                                    "xbl",
                                    "ping",
                                    "xmlhttprequest",
                                    "objectsubrequest",
                                    "dtd",
                                    "font",
                                    "media",
                                    "websocket"};

#define NUMBER_OF_TYPES NS_ARRAY_LENGTH(kTypeString)
uint8_t nsContentBlocker::mBehaviorPref[NUMBER_OF_TYPES];

NS_IMPL_ISUPPORTS3(nsContentBlocker, 
                   nsIContentPolicy,
                   nsIObserver,
                   nsSupportsWeakReference)

nsContentBlocker::nsContentBlocker()
{
  memset(mBehaviorPref, BEHAVIOR_ACCEPT, NUMBER_OF_TYPES);
}

nsresult
nsContentBlocker::Init()
{
  nsresult rv;
  mPermissionManager = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch("permissions.default.", getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrefBranch> oldPrefBranch;
  oldPrefBranch = do_QueryInterface(prefService);
  int32_t oldPref;
  rv = oldPrefBranch->GetIntPref("network.image.imageBehavior", &oldPref);
  if (NS_SUCCEEDED(rv) && oldPref) {
    int32_t newPref;
    switch (oldPref) {
      default:
        newPref = BEHAVIOR_ACCEPT;
        break;
      case 1:
        newPref = BEHAVIOR_NOFOREIGN;
        break;
      case 2:
        newPref = BEHAVIOR_REJECT;
        break;
    }
    prefBranch->SetIntPref("image", newPref);
    oldPrefBranch->ClearUserPref("network.image.imageBehavior");
  }


  
  
  
  mPrefBranchInternal = do_QueryInterface(prefBranch, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrefBranchInternal->AddObserver("", this, true);
  PrefChanged(prefBranch, nullptr);

  return rv;
}

#undef  LIMIT
#define LIMIT(x, low, high, default) ((x) >= (low) && (x) <= (high) ? (x) : (default))

void
nsContentBlocker::PrefChanged(nsIPrefBranch *aPrefBranch,
                              const char    *aPref)
{
  int32_t val;

#define PREF_CHANGED(_P) (!aPref || !strcmp(aPref, _P))

  for(uint32_t i = 0; i < NUMBER_OF_TYPES; ++i) {
    if (PREF_CHANGED(kTypeString[i]) &&
        NS_SUCCEEDED(aPrefBranch->GetIntPref(kTypeString[i], &val)))
      mBehaviorPref[i] = LIMIT(val, 1, 3, 1);
  }

}


NS_IMETHODIMP 
nsContentBlocker::ShouldLoad(uint32_t          aContentType,
                             nsIURI           *aContentLocation,
                             nsIURI           *aRequestingLocation,
                             nsISupports      *aRequestingContext,
                             const nsACString &aMimeGuess,
                             nsISupports      *aExtra,
                             nsIPrincipal     *aRequestPrincipal,
                             int16_t          *aDecision)
{
  *aDecision = nsIContentPolicy::ACCEPT;
  nsresult rv;

  
  
  if (aContentType > NUMBER_OF_TYPES)
    return NS_OK;
  
  
  if (!aContentLocation)
    return NS_OK;

  
  
  if (aContentType == nsIContentPolicy::TYPE_OBJECT)
    return NS_OK;
  
  
  
  nsCAutoString scheme;
  aContentLocation->GetScheme(scheme);
  if (!scheme.LowerCaseEqualsLiteral("ftp") &&
      !scheme.LowerCaseEqualsLiteral("http") &&
      !scheme.LowerCaseEqualsLiteral("https"))
    return NS_OK;

  bool shouldLoad, fromPrefs;
  rv = TestPermission(aContentLocation, aRequestingLocation, aContentType,
                      &shouldLoad, &fromPrefs);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!shouldLoad) {
    if (fromPrefs) {
      *aDecision = nsIContentPolicy::REJECT_TYPE;
    } else {
      *aDecision = nsIContentPolicy::REJECT_SERVER;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsContentBlocker::ShouldProcess(uint32_t          aContentType,
                                nsIURI           *aContentLocation,
                                nsIURI           *aRequestingLocation,
                                nsISupports      *aRequestingContext,
                                const nsACString &aMimeGuess,
                                nsISupports      *aExtra,
                                nsIPrincipal     *aRequestPrincipal,
                                int16_t          *aDecision)
{
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> item =
    do_QueryInterface(NS_CP_GetDocShellFromContext(aRequestingContext));

  if (item) {
    int32_t type;
    item->GetItemType(&type);
    if (type == nsIDocShellTreeItem::typeChrome) {
      *aDecision = nsIContentPolicy::ACCEPT;
      return NS_OK;
    }
  }

  
  
  
  
  
  
  if (aContentType == nsIContentPolicy::TYPE_OBJECT) {
    *aDecision = nsIContentPolicy::ACCEPT;

    bool shouldLoad, fromPrefs;
    nsresult rv = TestPermission(aContentLocation, aRequestingLocation,
				 aContentType, &shouldLoad, &fromPrefs);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!shouldLoad) {
      if (fromPrefs) {
	*aDecision = nsIContentPolicy::REJECT_TYPE;
      } else {
	*aDecision = nsIContentPolicy::REJECT_SERVER;
      }
    }
    return NS_OK;
  }
  
  
  
  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aRequestPrincipal,
                    aDecision);
}

nsresult
nsContentBlocker::TestPermission(nsIURI *aCurrentURI,
                                 nsIURI *aFirstURI,
                                 int32_t aContentType,
                                 bool *aPermission,
                                 bool *aFromPrefs)
{
  *aFromPrefs = false;
  
  
  *aPermission = true;

  
  
  
  
  uint32_t permission;
  nsresult rv = mPermissionManager->TestPermission(aCurrentURI, 
                                                   kTypeString[aContentType - 1],
                                                   &permission);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!permission) {
    permission = mBehaviorPref[aContentType - 1];
    *aFromPrefs = true;
  }

  
  
  switch (permission) {
  case BEHAVIOR_ACCEPT:
    *aPermission = true;
    break;
  case BEHAVIOR_REJECT:
    *aPermission = false;
    break;

  case BEHAVIOR_NOFOREIGN:
    

    
    if (!aFirstURI)
      return NS_OK;

    bool trustedSource = false;
    rv = aFirstURI->SchemeIs("chrome", &trustedSource);
    NS_ENSURE_SUCCESS(rv,rv);
    if (!trustedSource) {
      rv = aFirstURI->SchemeIs("resource", &trustedSource);
      NS_ENSURE_SUCCESS(rv,rv);
    }
    if (trustedSource)
      return NS_OK;

    
    
    
    
    

    nsCAutoString currentHost;
    rv = aCurrentURI->GetAsciiHost(currentHost);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    int32_t dot = currentHost.RFindChar('.');
    dot = currentHost.RFindChar('.', dot-1);
    ++dot;

    
    
    const nsCSubstring &tail =
      Substring(currentHost, dot, currentHost.Length() - dot);

    nsCAutoString firstHost;
    rv = aFirstURI->GetAsciiHost(firstHost);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (firstHost.Length() < tail.Length()) {
      *aPermission = false;
      return NS_OK;
    }
    
    
    const nsCSubstring &firstTail = 
      Substring(firstHost, firstHost.Length() - tail.Length(), tail.Length());

    
    
    if ((firstHost.Length() > tail.Length() && 
         firstHost.CharAt(firstHost.Length() - tail.Length() - 1) != '.') || 
        !tail.Equals(firstTail)) {
      *aPermission = false;
    }
    break;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsContentBlocker::Observe(nsISupports     *aSubject,
                          const char      *aTopic,
                          const PRUnichar *aData)
{
  NS_ASSERTION(!strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic),
               "unexpected topic - we only deal with pref changes!");

  if (mPrefBranchInternal)
    PrefChanged(mPrefBranchInternal, NS_LossyConvertUTF16toASCII(aData).get());
  return NS_OK;
}
