




#include "nsPopupWindowManager.h"

#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"





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

  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    bool permission;
    rv = prefBranch->GetBoolPref(kPopupDisablePref, &permission);
    if (NS_FAILED(rv)) {
      permission = true;
    }
    mPolicy = permission ? (PRUint32) DENY_POPUP : (PRUint32) ALLOW_POPUP;

    prefBranch->AddObserver(kPopupDisablePref, this, true);
  } 

  return NS_OK;
}





NS_IMETHODIMP
nsPopupWindowManager::TestPermission(nsIPrincipal* aPrincipal,
                                     PRUint32 *aPermission)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  NS_ENSURE_ARG_POINTER(aPermission);

  PRUint32 permit;
  *aPermission = mPolicy;

  if (mPermissionManager) {
    if (NS_SUCCEEDED(mPermissionManager->TestPermissionFromPrincipal(aPrincipal, "popup", &permit))) {
      
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
    
    bool permission = true;
    prefBranch->GetBoolPref(kPopupDisablePref, &permission);

    mPolicy = permission ? (PRUint32) DENY_POPUP : (PRUint32) ALLOW_POPUP;
  }

  return NS_OK;
}
