




































#include "nsPopupWindowManager.h"

#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"





static const char kPopupDisablePref[] = "dom.disable_open_during_load";





nsPopupWindowManager::nsPopupWindowManager() :
  mPolicy(ALLOW_POPUP)
{
}

nsPopupWindowManager::~nsPopupWindowManager()
{
}

NS_IMPL_ISUPPORTS3(nsPopupWindowManager, 
                   nsIPopupWindowManager,
                   nsIObserver,
                   nsSupportsWeakReference)

nsresult
nsPopupWindowManager::Init()
{
  nsresult rv;
  mPermissionManager = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    PRBool permission;
    rv = prefBranch->GetBoolPref(kPopupDisablePref, &permission);
    if (NS_FAILED(rv)) {
      permission = PR_TRUE;
    }
    mPolicy = permission ? (PRUint32) DENY_POPUP : (PRUint32) ALLOW_POPUP;

    prefBranch->AddObserver(kPopupDisablePref, this, PR_TRUE);
  } 

  return NS_OK;
}





NS_IMETHODIMP
nsPopupWindowManager::TestPermission(nsIURI *aURI, PRUint32 *aPermission)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aPermission);

  nsresult rv;
  PRUint32 permit;

  *aPermission = mPolicy;

  if (mPermissionManager) {
    rv = mPermissionManager->TestPermission(aURI, "popup", &permit);

    if (NS_SUCCEEDED(rv)) {
      
      if (permit == nsIPermissionManager::ALLOW_ACTION) {
        *aPermission = ALLOW_POPUP;
      } else if (permit == nsIPermissionManager::DENY_ACTION) {
        *aPermission = DENY_POPUP;
      }
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsPopupWindowManager::Observe(nsISupports *aSubject, 
                              const char *aTopic,
                              const PRUnichar *aData)
{
  nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject);
  NS_ASSERTION(!nsCRT::strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic),
               "unexpected topic - we only deal with pref changes!");

  if (prefBranch) {
    
    PRBool permission = PR_TRUE;
    prefBranch->GetBoolPref(kPopupDisablePref, &permission);

    mPolicy = permission ? (PRUint32) DENY_POPUP : (PRUint32) ALLOW_POPUP;
  }

  return NS_OK;
}
