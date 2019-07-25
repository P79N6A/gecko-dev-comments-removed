






































#include "CheckPermissionsHelper.h"

#include "nsIDOMWindow.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"

#include "nsContentUtils.h"
#include "nsDOMStorage.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"

#include "IndexedDatabaseManager.h"

#define PERMISSION_INDEXEDDB "indexedDB"
#define PREF_INDEXEDDB_ENABLED "dom.indexedDB.enabled"
#define TOPIC_PERMISSIONS_PROMPT "indexedDB-permissions-prompt"
#define TOPIC_PERMISSIONS_RESPONSE "indexedDB-permissions-response"

using namespace mozilla;
USING_INDEXEDDB_NAMESPACE
using namespace mozilla::services;

namespace {

inline
PRUint32
GetIndexedDBPermissions(const nsACString& aASCIIOrigin,
                        nsIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!Preferences::GetBool(PREF_INDEXEDDB_ENABLED)) {
    return nsIPermissionManager::DENY_ACTION;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(sop, nsIPermissionManager::DENY_ACTION);

  if (nsContentUtils::IsSystemPrincipal(sop->GetPrincipal())) {
    return nsIPermissionManager::ALLOW_ACTION;
  }

  if (nsDOMStorageManager::gStorageManager->InPrivateBrowsingMode()) {
    
    return nsIPermissionManager::DENY_ACTION;
  }

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aASCIIOrigin);
  NS_ENSURE_SUCCESS(rv, nsIPermissionManager::DENY_ACTION);

  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permissionManager, nsIPermissionManager::DENY_ACTION);

  PRUint32 permission;
  rv = permissionManager->TestPermission(uri, PERMISSION_INDEXEDDB,
                                         &permission);
  NS_ENSURE_SUCCESS(rv, nsIPermissionManager::DENY_ACTION);

  return permission;
}

} 

NS_IMPL_THREADSAFE_ISUPPORTS3(CheckPermissionsHelper, nsIRunnable,
                                                      nsIInterfaceRequestor,
                                                      nsIObserver)

NS_IMETHODIMP
CheckPermissionsHelper::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  PRUint32 permission = mHasPrompted ?
                        mPromptResult :
                        GetIndexedDBPermissions(mASCIIOrigin, mWindow);

  nsresult rv;
  if (mHasPrompted) {
    
    
    
    
    if (permission != nsIPermissionManager::UNKNOWN_ACTION &&
        XRE_GetProcessType() == GeckoProcessType_Default) {
      nsCOMPtr<nsIURI> uri;
      rv = NS_NewURI(getter_AddRefs(uri), mASCIIOrigin);
      NS_ENSURE_SUCCESS(rv, rv);
  
      nsCOMPtr<nsIPermissionManager> permissionManager =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
      NS_ENSURE_STATE(permissionManager);
  
      rv = permissionManager->Add(uri, PERMISSION_INDEXEDDB, permission,
                                  nsIPermissionManager::EXPIRE_NEVER, 0);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (permission == nsIPermissionManager::UNKNOWN_ACTION &&
           mPromptAllowed) {
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    rv = obs->NotifyObservers(static_cast<nsIRunnable*>(this),
                              TOPIC_PERMISSIONS_PROMPT, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  nsRefPtr<OpenDatabaseHelper> helper;
  helper.swap(mHelper);

  nsCOMPtr<nsIDOMWindow> window;
  window.swap(mWindow);

  if (permission == nsIPermissionManager::ALLOW_ACTION) {
    IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
    NS_ASSERTION(mgr, "This should never be null!");

    return helper->Dispatch(mgr->IOThread());
  }

  NS_ASSERTION(permission == nsIPermissionManager::UNKNOWN_ACTION ||
               permission == nsIPermissionManager::DENY_ACTION,
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

  *aResult = nsnull;
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
CheckPermissionsHelper::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!strcmp(aTopic, TOPIC_PERMISSIONS_RESPONSE), "Bad topic!");
  NS_ASSERTION(mPromptAllowed, "How did we get here?");

  mHasPrompted = true;

  nsresult rv;
  mPromptResult = nsDependentString(aData).ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_DispatchToCurrentThread(this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
