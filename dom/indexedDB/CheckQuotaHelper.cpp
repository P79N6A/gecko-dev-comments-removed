






































#include "CheckQuotaHelper.h"

#include "nsIDOMWindow.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"
#include "nsXULAppAPI.h"

#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"

#include "IDBFactory.h"

#define PERMISSION_INDEXEDDB_UNLIMITED "indexedDB-unlimited"

#define TOPIC_QUOTA_PROMPT "indexedDB-quota-prompt"
#define TOPIC_QUOTA_RESPONSE "indexedDB-quota-response"
#define TOPIC_QUOTA_CANCEL "indexedDB-quota-cancel"

USING_INDEXEDDB_NAMESPACE
using namespace mozilla::services;
using mozilla::MutexAutoLock;

namespace {

inline
PRUint32
GetQuotaPermissions(const nsACString& aASCIIOrigin,
                    nsIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(sop, nsIPermissionManager::DENY_ACTION);

  if (nsContentUtils::IsSystemPrincipal(sop->GetPrincipal())) {
    return nsIPermissionManager::ALLOW_ACTION;
  }

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aASCIIOrigin);
  NS_ENSURE_SUCCESS(rv, nsIPermissionManager::DENY_ACTION);

  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permissionManager, nsIPermissionManager::DENY_ACTION);

  PRUint32 permission;
  rv = permissionManager->TestPermission(uri, PERMISSION_INDEXEDDB_UNLIMITED,
                                         &permission);
  NS_ENSURE_SUCCESS(rv, nsIPermissionManager::DENY_ACTION);

  return permission;
}

} 

CheckQuotaHelper::CheckQuotaHelper(IDBDatabase* aDatabase,
                                   mozilla::Mutex& aMutex)
: mWindow(aDatabase->Owner()),
  mWindowSerial(mWindow->GetSerial()),
  mOrigin(aDatabase->Origin()),
  mMutex(aMutex),
  mCondVar(mMutex, "CheckQuotaHelper::mCondVar"),
  mPromptResult(0),
  mWaiting(true),
  mHasPrompted(false)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  mMutex.AssertCurrentThreadOwns();
}

bool
CheckQuotaHelper::PromptAndReturnQuotaIsDisabled()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  mMutex.AssertCurrentThreadOwns();

  while (mWaiting) {
    mCondVar.Wait();
  }

  NS_ASSERTION(!mWindow, "This should always be null here!");

  NS_ASSERTION(mPromptResult == nsIPermissionManager::ALLOW_ACTION ||
               mPromptResult == nsIPermissionManager::DENY_ACTION ||
               mPromptResult == nsIPermissionManager::UNKNOWN_ACTION,
               "Unknown permission!");

  return mPromptResult == nsIPermissionManager::ALLOW_ACTION;
}

void
CheckQuotaHelper::Cancel()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mMutex.AssertCurrentThreadOwns();

  if (mWaiting && !mHasPrompted) {
    MutexAutoUnlock unlock(mMutex);

    
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    NS_WARN_IF_FALSE(obs, "Failed to get observer service!");
    if (obs && NS_FAILED(obs->NotifyObservers(static_cast<nsIRunnable*>(this),
                                              TOPIC_QUOTA_CANCEL, nsnull))) {
      NS_WARNING("Failed to notify observers!");
    }

    
    
    if (!mHasPrompted) {
      nsAutoString response;
      response.AppendInt(nsIPermissionManager::UNKNOWN_ACTION);

      if (NS_SUCCEEDED(Observe(nsnull, TOPIC_QUOTA_RESPONSE, response.get()))) {
        NS_ASSERTION(mHasPrompted, "Should have set this in Observe!");
      }
      else {
        NS_WARNING("Failed to notify!");
      }
    }
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS3(CheckQuotaHelper, nsIRunnable,
                                                nsIInterfaceRequestor,
                                                nsIObserver)

NS_IMETHODIMP
CheckQuotaHelper::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mHasPrompted) {
    mPromptResult = GetQuotaPermissions(mOrigin, mWindow);
  }

  nsresult rv;
  if (mHasPrompted) {
    
    
    
    
    if (mPromptResult != nsIPermissionManager::UNKNOWN_ACTION &&
        XRE_GetProcessType() == GeckoProcessType_Default) {
      nsCOMPtr<nsIURI> uri;
      rv = NS_NewURI(getter_AddRefs(uri), mOrigin);
      NS_ENSURE_SUCCESS(rv, rv);
  
      nsCOMPtr<nsIPermissionManager> permissionManager =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
      NS_ENSURE_STATE(permissionManager);
  
      rv = permissionManager->Add(uri, PERMISSION_INDEXEDDB_UNLIMITED,
                                  mPromptResult,
                                  nsIPermissionManager::EXPIRE_NEVER, 0);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (mPromptResult == nsIPermissionManager::UNKNOWN_ACTION) {
    PRUint32 quota = IDBFactory::GetIndexedDBQuota();
    NS_ASSERTION(quota, "Shouldn't get here if quota is disabled!");

    nsString quotaString;
    quotaString.AppendInt(quota);

    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    NS_ENSURE_STATE(obs);

    
    
    rv = obs->AddObserver(this, DOM_WINDOW_DESTROYED_TOPIC, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obs->NotifyObservers(static_cast<nsIRunnable*>(this),
                              TOPIC_QUOTA_PROMPT, quotaString.get());
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  MutexAutoLock lock(mMutex);

  NS_ASSERTION(mWaiting, "Huh?!");

    
  mWindow = nsnull;

  mWaiting = false;
  mCondVar.NotifyAll();

  return NS_OK;
}

NS_IMETHODIMP
CheckQuotaHelper::GetInterface(const nsIID& aIID,
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
CheckQuotaHelper::Observe(nsISupports* aSubject,
                          const char* aTopic,
                          const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsresult rv;

  if (!strcmp(aTopic, TOPIC_QUOTA_RESPONSE)) {
    if (!mHasPrompted) {
      mHasPrompted = true;

      mPromptResult = nsDependentString(aData).ToInteger(&rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_DispatchToCurrentThread(this);
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsCOMPtr<nsIObserverService> obs = GetObserverService();
      NS_ENSURE_STATE(obs);

      rv = obs->RemoveObserver(this, DOM_WINDOW_DESTROYED_TOPIC);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }

  if (!strcmp(aTopic, DOM_WINDOW_DESTROYED_TOPIC)) {
    NS_ASSERTION(!mHasPrompted, "Should have removed observer before now!");
    NS_ASSERTION(mWindow, "This should never be null!");

    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aSubject));
    NS_ENSURE_STATE(window);

    if (mWindow->GetSerial() == window->GetSerial()) {
      
      mHasPrompted = true;
      mPromptResult = nsIPermissionManager::UNKNOWN_ACTION;

      rv = NS_DispatchToCurrentThread(this);
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsCOMPtr<nsIObserverService> obs = GetObserverService();
      NS_ENSURE_STATE(obs);

      rv = obs->RemoveObserver(this, DOM_WINDOW_DESTROYED_TOPIC);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }

  NS_NOTREACHED("Unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}
