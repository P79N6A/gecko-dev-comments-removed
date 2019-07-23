


































#include "nsContentBlocker.h"
#include "nsIDocument.h"
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


static const char *kTypeString[NUMBER_OF_TYPES] = {"other", 
                                                   "script",
                                                   "image",
                                                   "stylesheet",
                                                   "object",
                                                   "document",
                                                   "subdocument",
                                                   "refresh"};


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
  PRInt32 oldPref;
  rv = oldPrefBranch->GetIntPref("network.image.imageBehavior", &oldPref);
  if (NS_SUCCEEDED(rv) && oldPref) {
    PRInt32 newPref;
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

  rv = mPrefBranchInternal->AddObserver("", this, PR_TRUE);
  PrefChanged(prefBranch, nsnull);

  return rv;
}

#undef  LIMIT
#define LIMIT(x, low, high, default) ((x) >= (low) && (x) <= (high) ? (x) : (default))

void
nsContentBlocker::PrefChanged(nsIPrefBranch *aPrefBranch,
                              const char    *aPref)
{
  PRInt32 val;

#define PREF_CHANGED(_P) (!aPref || !strcmp(aPref, _P))

  for(PRUint32 i = 0; i < NUMBER_OF_TYPES; ++i) {
    if (PREF_CHANGED(kTypeString[i]) &&
        NS_SUCCEEDED(aPrefBranch->GetIntPref(kTypeString[i], &val)))
      mBehaviorPref[i] = LIMIT(val, 1, 3, 1);
  }

}


NS_IMETHODIMP 
nsContentBlocker::ShouldLoad(PRUint32          aContentType,
                             nsIURI           *aContentLocation,
                             nsIURI           *aRequestingLocation,
                             nsISupports      *aRequestingContext,
                             const nsACString &aMimeGuess,
                             nsISupports      *aExtra,
                             PRInt16          *aDecision)
{
  *aDecision = nsIContentPolicy::ACCEPT;
  nsresult rv;

  
  
  if (aContentType > NUMBER_OF_TYPES)
    return NS_OK;
  
  
  if (!aContentLocation)
    return NS_OK;

  
  
  nsCAutoString scheme;
  aContentLocation->GetScheme(scheme);
  if (!scheme.LowerCaseEqualsLiteral("ftp") &&
      !scheme.LowerCaseEqualsLiteral("http") &&
      !scheme.LowerCaseEqualsLiteral("https"))
    return NS_OK;

  PRBool shouldLoad, fromPrefs;
  rv = TestPermission(aContentLocation, aRequestingLocation, aContentType,
                      &shouldLoad, &fromPrefs);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!shouldLoad)
    if (fromPrefs)
      *aDecision = nsIContentPolicy::REJECT_TYPE;
    else
      *aDecision = nsIContentPolicy::REJECT_SERVER;

  if (aContentType != nsIContentPolicy::TYPE_OBJECT || aMimeGuess.IsEmpty())
    return NS_OK;

  
  
  nsCOMPtr<nsIObjectLoadingContent> objectLoader =
    do_QueryInterface(aRequestingContext);
  if (!objectLoader)
    return NS_OK;

  PRUint32 contentType;
  rv = objectLoader->GetContentTypeForMIMEType(aMimeGuess, &contentType);
  if (NS_FAILED(rv))
    return rv;
    
  switch (contentType) {
  case nsIObjectLoadingContent::TYPE_IMAGE:
    aContentType = nsIContentPolicy::TYPE_IMAGE;
    break;
  case nsIObjectLoadingContent::TYPE_DOCUMENT:
    aContentType = nsIContentPolicy::TYPE_SUBDOCUMENT;
    break;
  default:
    return NS_OK;
  }

  NS_ASSERTION(aContentType != nsIContentPolicy::TYPE_OBJECT,
	       "Shouldn't happen.  Infinite loops are bad!");

  
  
  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
		    aRequestingContext, aMimeGuess, aExtra, aDecision);
}

NS_IMETHODIMP
nsContentBlocker::ShouldProcess(PRUint32          aContentType,
                                nsIURI           *aContentLocation,
                                nsIURI           *aRequestingLocation,
                                nsISupports      *aRequestingContext,
                                const nsACString &aMimeGuess,
                                nsISupports      *aExtra,
                                PRInt16          *aDecision)
{
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> item =
    do_QueryInterface(NS_CP_GetDocShellFromContext(aRequestingContext));

  if (item) {
    PRInt32 type;
    item->GetItemType(&type);
    if (type == nsIDocShellTreeItem::typeChrome) {
      *aDecision = nsIContentPolicy::ACCEPT;
      return NS_OK;
    }
  }

  
  
  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aDecision);
}

nsresult
nsContentBlocker::TestPermission(nsIURI *aCurrentURI,
                                 nsIURI *aFirstURI,
                                 PRInt32 aContentType,
                                 PRBool *aPermission,
                                 PRBool *aFromPrefs)
{
  *aFromPrefs = PR_FALSE;
  
  
  *aPermission = PR_TRUE;

  
  
  
  
  PRUint32 permission;
  nsresult rv = mPermissionManager->TestPermission(aCurrentURI, 
                                                   kTypeString[aContentType - 1],
                                                   &permission);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!permission) {
    permission = mBehaviorPref[aContentType - 1];
    *aFromPrefs = PR_TRUE;
  }

  
  
  switch (permission) {
  case BEHAVIOR_ACCEPT:
    *aPermission = PR_TRUE;
    break;
  case BEHAVIOR_REJECT:
    *aPermission = PR_FALSE;
    break;

  case BEHAVIOR_NOFOREIGN:
    

    
    if (!aFirstURI)
      return NS_OK;

    PRBool trustedSource = PR_FALSE;
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

    
    
    
    PRInt32 dot = currentHost.RFindChar('.');
    dot = currentHost.RFindChar('.', dot-1);
    ++dot;

    
    
    const nsCSubstring &tail =
      Substring(currentHost, dot, currentHost.Length() - dot);

    nsCAutoString firstHost;
    rv = aFirstURI->GetAsciiHost(firstHost);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (firstHost.Length() < tail.Length()) {
      *aPermission = PR_FALSE;
      return NS_OK;
    }
    
    
    const nsCSubstring &firstTail = 
      Substring(firstHost, firstHost.Length() - tail.Length(), tail.Length());

    
    
    if ((firstHost.Length() > tail.Length() && 
         firstHost.CharAt(firstHost.Length() - tail.Length() - 1) != '.') || 
        !tail.Equals(firstTail)) {
      *aPermission = PR_FALSE;
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
