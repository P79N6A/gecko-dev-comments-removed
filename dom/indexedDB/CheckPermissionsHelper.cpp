





#include "CheckPermissionsHelper.h"

#include "nsIDOMWindow.h"
#include "nsILoadContext.h"
#include "nsIWebNavigation.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"

#include "CheckQuotaHelper.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"

#include "IndexedDatabaseManager.h"

#define PERMISSION_INDEXEDDB "indexedDB"
#define TOPIC_PERMISSIONS_PROMPT "indexedDB-permissions-prompt"
#define TOPIC_PERMISSIONS_RESPONSE "indexedDB-permissions-response"





#define PERMISSION_ALLOWED nsIPermissionManager::UNKNOWN_ACTION
#define PERMISSION_DENIED nsIPermissionManager::DENY_ACTION
#define PERMISSION_PROMPT nsIPermissionManager::ALLOW_ACTION

USING_INDEXEDDB_NAMESPACE
using namespace mozilla::services;
using mozilla::dom::quota::CheckQuotaHelper;

namespace {

inline
uint32_t
GetIndexedDBPermissions(nsIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ASSERTION(aWindow, "Chrome shouldn't check the permission!");

  nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(sop, nsIPermissionManager::DENY_ACTION);

  NS_ASSERTION(!nsContentUtils::IsSystemPrincipal(sop->GetPrincipal()),
               "Chrome windows shouldn't check the permission!");

  nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(aWindow);
  nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(webNav);
  if (loadContext && loadContext->UsePrivateBrowsing()) {
    
    NS_WARNING("IndexedDB may not be used while in private browsing mode!");
    return PERMISSION_DENIED;
  }

  nsCOMPtr<nsIPermissionManager> permissionManager = GetPermissionManager();
  NS_ENSURE_TRUE(permissionManager, PERMISSION_DENIED);

  uint32_t permission;
  nsresult rv =
    permissionManager->TestPermissionFromPrincipal(sop->GetPrincipal(),
                                                   PERMISSION_INDEXEDDB,
                                                   &permission);
  NS_ENSURE_SUCCESS(rv, PERMISSION_DENIED);

  return permission;
}

} 

NS_IMPL_ISUPPORTS(CheckPermissionsHelper, nsIRunnable,
                  nsIInterfaceRequestor,
                  nsIObserver)

NS_IMETHODIMP
CheckPermissionsHelper::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  uint32_t permission = mHasPrompted ?
                        mPromptResult :
                        GetIndexedDBPermissions(mWindow);

  nsresult rv;
  if (mHasPrompted) {
    
    
    
    
    if (permission != PERMISSION_PROMPT &&
        IndexedDatabaseManager::IsMainProcess()) {
      NS_ASSERTION(mWindow, "Null window!");

      nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(mWindow);
      NS_ASSERTION(sop, "Window didn't QI to nsIScriptObjectPrincipal!");

      nsIPrincipal* windowPrincipal = sop->GetPrincipal();
      NS_ASSERTION(windowPrincipal, "Null principal!");

      nsCOMPtr<nsIPermissionManager> permissionManager = GetPermissionManager();
      NS_ENSURE_STATE(permissionManager);

      rv = permissionManager->AddFromPrincipal(windowPrincipal,
                                               PERMISSION_INDEXEDDB, permission,
                                               nsIPermissionManager::EXPIRE_NEVER,
                                               0);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (permission == PERMISSION_PROMPT && mPromptAllowed) {
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    rv = obs->NotifyObservers(static_cast<nsIRunnable*>(this),
                              TOPIC_PERMISSIONS_PROMPT, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  nsRefPtr<OpenDatabaseHelper> helper;
  helper.swap(mHelper);

  nsCOMPtr<nsIDOMWindow> window;
  window.swap(mWindow);

  if (permission == PERMISSION_ALLOWED) {
    
    
    
    if (window) {
      nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(window);
      NS_ASSERTION(sop, "Window didn't QI to nsIScriptObjectPrincipal!");

      nsIPrincipal* windowPrincipal = sop->GetPrincipal();
      NS_ASSERTION(windowPrincipal, "Null principal!");

      uint32_t quotaPermission =
        CheckQuotaHelper::GetQuotaPermission(windowPrincipal);

      if (quotaPermission == nsIPermissionManager::ALLOW_ACTION) {
        helper->SetUnlimitedQuotaAllowed();
      }
    }

    return helper->DispatchToIOThread();
  }

  NS_ASSERTION(permission == PERMISSION_PROMPT ||
               permission == PERMISSION_DENIED,
               "Unknown permission!");

  helper->SetError(NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR);

  return helper->RunImmediately();
}

NS_IMETHODIMP
CheckPermissionsHelper::GetInterface(const nsIID& aIID,
                                     void** aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  if (aIID.Equals(NS_GET_IID(nsIObserver))) {
    return QueryInterface(aIID, aResult);
  }

  if (aIID.Equals(NS_GET_IID(nsIDOMWindow))) {
    return mWindow->QueryInterface(aIID, aResult);
  }

  *aResult = nullptr;
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
CheckPermissionsHelper::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const char16_t* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!strcmp(aTopic, TOPIC_PERMISSIONS_RESPONSE), "Bad topic!");
  NS_ASSERTION(mPromptAllowed, "How did we get here?");

  mHasPrompted = true;

  nsresult rv;
  uint32_t promptResult = nsDependentString(aData).ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  switch (promptResult) {
    case nsIPermissionManager::ALLOW_ACTION:
      mPromptResult = PERMISSION_ALLOWED;
      break;
    case nsIPermissionManager::DENY_ACTION:
      mPromptResult = PERMISSION_DENIED;
      break;
    case nsIPermissionManager::UNKNOWN_ACTION:
      mPromptResult = PERMISSION_PROMPT;
      break;

    default:
      NS_NOTREACHED("Unknown permission type!");
      mPromptResult = PERMISSION_DENIED;
  }

  rv = NS_DispatchToCurrentThread(this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
