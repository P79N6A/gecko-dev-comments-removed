





#include "DataStoreService.h"

#include "DataStoreCallbacks.h"
#include "DataStoreDB.h"
#include "DataStoreRevision.h"
#include "mozilla/dom/DataStore.h"
#include "mozilla/dom/DataStoreBinding.h"
#include "mozilla/dom/DataStoreImplBinding.h"
#include "nsIDataStore.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/indexedDB/IDBCursor.h"
#include "mozilla/dom/indexedDB/IDBObjectStore.h"
#include "mozilla/dom/PermissionMessageUtils.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/unused.h"

#include "mozIApplication.h"
#include "mozIApplicationClearPrivateDataParams.h"
#include "nsIAppsService.h"
#include "nsIDOMEvent.h"
#include "nsIDocument.h"
#include "nsIDOMGlobalPropertyInitializer.h"
#include "nsIIOService.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIUUIDGenerator.h"
#include "nsPIDOMWindow.h"
#include "nsIURI.h"

#include "nsContentUtils.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#define ASSERT_PARENT_PROCESS()                                             \
  AssertIsInMainProcess();                                                  \
  if (NS_WARN_IF(!IsMainProcess())) {                                       \
    return NS_ERROR_FAILURE;                                                \
  }

namespace mozilla {
namespace dom {

using namespace indexedDB;


class DataStoreInfo
{
public:
  DataStoreInfo()
    : mReadOnly(true)
    , mEnabled(false)
  {}

  DataStoreInfo(const nsAString& aName,
                const nsAString& aOriginURL,
                const nsAString& aManifestURL,
                bool aReadOnly,
                bool aEnabled)
  {
    Init(aName, aOriginURL, aManifestURL, aReadOnly, aEnabled);
  }

  void Init(const nsAString& aName,
            const nsAString& aOriginURL,
            const nsAString& aManifestURL,
            bool aReadOnly,
            bool aEnabled)
  {
    mName = aName;
    mOriginURL = aOriginURL;
    mManifestURL = aManifestURL;
    mReadOnly = aReadOnly;
    mEnabled = aEnabled;
  }

  void Update(const nsAString& aName,
              const nsAString& aOriginURL,
              const nsAString& aManifestURL,
              bool aReadOnly)
  {
    mName = aName;
    mOriginURL = aOriginURL;
    mManifestURL = aManifestURL;
    mReadOnly = aReadOnly;
  }

  void Enable()
  {
    mEnabled = true;
  }

  nsString mName;
  nsString mOriginURL;
  nsString mManifestURL;
  bool mReadOnly;

  
  bool mEnabled;
};

namespace {


StaticRefPtr<DataStoreService> gDataStoreService;
static uint64_t gCounterID = 0;

typedef nsClassHashtable<nsUint32HashKey, DataStoreInfo> HashApp;

bool
IsMainProcess()
{
  static const bool isMainProcess =
    XRE_GetProcessType() == GeckoProcessType_Default;
  return isMainProcess;
}

void
AssertIsInMainProcess()
{
  MOZ_ASSERT(IsMainProcess());
}

void
RejectPromise(nsPIDOMWindow* aWindow, Promise* aPromise, nsresult aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(NS_FAILED(aRv));

  nsRefPtr<DOMError> error;
  if (aRv == NS_ERROR_DOM_SECURITY_ERR) {
    error = new DOMError(aWindow, NS_LITERAL_STRING("SecurityError"),
                         NS_LITERAL_STRING("Access denied"));
  } else {
    error = new DOMError(aWindow, NS_LITERAL_STRING("InternalError"),
                         NS_LITERAL_STRING("An error occurred"));
  }

  aPromise->MaybeRejectBrokenly(error);
}

void
DeleteDatabase(const nsAString& aName,
               const nsAString& aManifestURL)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<DataStoreDB> db = new DataStoreDB(aManifestURL, aName);
  db->Delete();
}

PLDHashOperator
DeleteDataStoresAppEnumerator(
                             const uint32_t& aAppId,
                             nsAutoPtr<DataStoreInfo>& aInfo,
                             void* aUserData)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  auto* appId = static_cast<uint32_t*>(aUserData);
  if (*appId != aAppId) {
    return PL_DHASH_NEXT;
  }

  DeleteDatabase(aInfo->mName, aInfo->mManifestURL);
  return PL_DHASH_REMOVE;
}

PLDHashOperator
DeleteDataStoresEnumerator(const nsAString& aName,
                           nsAutoPtr<HashApp>& aApps,
                           void* aUserData)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  aApps->Enumerate(DeleteDataStoresAppEnumerator, aUserData);
  return aApps->Count() ? PL_DHASH_NEXT : PL_DHASH_REMOVE;
}

void
GeneratePermissionName(nsAString& aPermission,
                       const nsAString& aName,
                       const nsAString& aManifestURL)
{
  aPermission.AssignLiteral("indexedDB-chrome-");
  aPermission.Append(aName);
  aPermission.Append('|');
  aPermission.Append(aManifestURL);
}

nsresult
ResetPermission(uint32_t aAppId, const nsAString& aOriginURL,
                const nsAString& aManifestURL,
                const nsAString& aPermission,
                bool aReadOnly)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;
  nsCOMPtr<nsIIOService> ioService(do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIURI> uri;
  rv = ioService->NewURI(NS_ConvertUTF16toUTF8(aOriginURL), nullptr, nullptr,
                         getter_AddRefs(uri));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
  if (!ssm) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPrincipal> principal;
  rv = ssm->GetAppCodebasePrincipal(uri, aAppId, false,
                                    getter_AddRefs(principal));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIPermissionManager> pm =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (!pm) {
    return NS_ERROR_FAILURE;
  }

  nsCString basePermission;
  basePermission.Append(NS_ConvertUTF16toUTF8(aPermission));

  
  {
    nsCString permission;
    permission.Append(basePermission);
    permission.AppendLiteral("-write");

    uint32_t perm = nsIPermissionManager::UNKNOWN_ACTION;
    rv = pm->TestExactPermissionFromPrincipal(principal, permission.get(),
                                              &perm);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (aReadOnly && perm == nsIPermissionManager::ALLOW_ACTION) {
      rv = pm->RemoveFromPrincipal(principal, permission.get());
    }
    else if (!aReadOnly && perm != nsIPermissionManager::ALLOW_ACTION) {
      rv = pm->AddFromPrincipal(principal, permission.get(),
                                nsIPermissionManager::ALLOW_ACTION,
                                nsIPermissionManager::EXPIRE_NEVER, 0);
    }

    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  {
    nsCString permission;
    permission.Append(basePermission);
    permission.AppendLiteral("-read");

    uint32_t perm = nsIPermissionManager::UNKNOWN_ACTION;
    rv = pm->TestExactPermissionFromPrincipal(principal, permission.get(),
                                              &perm);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (perm != nsIPermissionManager::ALLOW_ACTION) {
      rv = pm->AddFromPrincipal(principal, permission.get(),
                                nsIPermissionManager::ALLOW_ACTION,
                                nsIPermissionManager::EXPIRE_NEVER, 0);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  
  uint32_t perm = nsIPermissionManager::UNKNOWN_ACTION;
  rv = pm->TestExactPermissionFromPrincipal(principal, basePermission.get(),
                                            &perm);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (perm != nsIPermissionManager::ALLOW_ACTION) {
    rv = pm->AddFromPrincipal(principal, basePermission.get(),
                              nsIPermissionManager::ALLOW_ACTION,
                              nsIPermissionManager::EXPIRE_NEVER, 0);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

class MOZ_STACK_CLASS GetDataStoreInfosData
{
public:
  GetDataStoreInfosData(nsClassHashtable<nsStringHashKey, HashApp>& aAccessStores,
                        const nsAString& aName, uint32_t aAppId,
                        nsTArray<DataStoreInfo>& aStores)
    : mAccessStores(aAccessStores)
    , mName(aName)
    , mAppId(aAppId)
    , mStores(aStores)
  {}

  nsClassHashtable<nsStringHashKey, HashApp>& mAccessStores;
  nsString mName;
  uint32_t mAppId;
  nsTArray<DataStoreInfo>& mStores;
};

PLDHashOperator
GetDataStoreInfosEnumerator(const uint32_t& aAppId,
                            DataStoreInfo* aInfo,
                            void* aUserData)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  auto* data = static_cast<GetDataStoreInfosData*>(aUserData);
  if (aAppId == data->mAppId) {
    return PL_DHASH_NEXT;
  }

  HashApp* apps;
  if (!data->mAccessStores.Get(data->mName, &apps)) {
    return PL_DHASH_NEXT;
  }

  DataStoreInfo* accessInfo = nullptr;
  if (!apps->Get(data->mAppId, &accessInfo)) {
    return PL_DHASH_NEXT;
  }

  bool readOnly = aInfo->mReadOnly || accessInfo->mReadOnly;
  DataStoreInfo* accessStore = data->mStores.AppendElement();
  accessStore->Init(aInfo->mName, aInfo->mOriginURL,
                    aInfo->mManifestURL, readOnly,
                    aInfo->mEnabled);

  return PL_DHASH_NEXT;
}


class MOZ_STACK_CLASS AddPermissionsData
{
public:
  AddPermissionsData(const nsAString& aPermission, bool aReadOnly)
    : mPermission(aPermission)
    , mReadOnly(aReadOnly)
    , mResult(NS_OK)
  {}

  nsString mPermission;
  bool mReadOnly;
  nsresult mResult;
};

PLDHashOperator
AddPermissionsEnumerator(const uint32_t& aAppId,
                         DataStoreInfo* aInfo,
                         void* userData)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  auto* data = static_cast<AddPermissionsData*>(userData);

  
  bool readOnly = data->mReadOnly || aInfo->mReadOnly;

  data->mResult = ResetPermission(aAppId, aInfo->mOriginURL,
                                  aInfo->mManifestURL,
                                  data->mPermission,
                                  readOnly);
  return NS_FAILED(data->mResult) ? PL_DHASH_STOP : PL_DHASH_NEXT;
}


class MOZ_STACK_CLASS AddAccessPermissionsData
{
public:
  AddAccessPermissionsData(uint32_t aAppId, const nsAString& aName,
                           const nsAString& aOriginURL, bool aReadOnly)
    : mAppId(aAppId)
    , mName(aName)
    , mOriginURL(aOriginURL)
    , mReadOnly(aReadOnly)
    , mResult(NS_OK)
  {}

  uint32_t mAppId;
  nsString mName;
  nsString mOriginURL;
  bool mReadOnly;
  nsresult mResult;
};

PLDHashOperator
AddAccessPermissionsEnumerator(const uint32_t& aAppId,
                               DataStoreInfo* aInfo,
                               void* userData)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  auto* data = static_cast<AddAccessPermissionsData*>(userData);

  nsString permission;
  GeneratePermissionName(permission, data->mName, aInfo->mManifestURL);

  
  bool readOnly = aInfo->mReadOnly || data->mReadOnly;

  data->mResult = ResetPermission(data->mAppId, data->mOriginURL,
                                  aInfo->mManifestURL,
                                  permission, readOnly);
  return NS_FAILED(data->mResult) ? PL_DHASH_STOP : PL_DHASH_NEXT;
}

} 



class PendingRequest
{
public:
  void Init(nsPIDOMWindow* aWindow, Promise* aPromise,
            const nsTArray<DataStoreInfo>& aStores,
            const nsTArray<nsString>& aPendingDataStores)
  {
    mWindow = aWindow;
    mPromise = aPromise;
    mStores = aStores;
    mPendingDataStores = aPendingDataStores;
  }

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<Promise> mPromise;
  nsTArray<DataStoreInfo> mStores;

  
  
  nsTArray<nsString> mPendingDataStores;
};



class RevisionAddedEnableStoreCallback MOZ_FINAL :
  public DataStoreRevisionCallback
{
private:
  ~RevisionAddedEnableStoreCallback() {}
public:
  NS_INLINE_DECL_REFCOUNTING(RevisionAddedEnableStoreCallback);

  RevisionAddedEnableStoreCallback(uint32_t aAppId,
                                   const nsAString& aName,
                                   const nsAString& aManifestURL)
    : mAppId(aAppId)
    , mName(aName)
    , mManifestURL(aManifestURL)
  {
    AssertIsInMainProcess();
    MOZ_ASSERT(NS_IsMainThread());
  }

  void
  Run(const nsAString& aRevisionId)
  {
    AssertIsInMainProcess();
    MOZ_ASSERT(NS_IsMainThread());

    nsRefPtr<DataStoreService> service = DataStoreService::Get();
    MOZ_ASSERT(service);

    service->EnableDataStore(mAppId, mName, mManifestURL);
  }

private:
  uint32_t mAppId;
  nsString mName;
  nsString mManifestURL;
};



class FirstRevisionIdCallback MOZ_FINAL : public DataStoreDBCallback
                                        , public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS

  FirstRevisionIdCallback(uint32_t aAppId, const nsAString& aName,
                          const nsAString& aManifestURL)
    : mAppId(aAppId)
    , mName(aName)
    , mManifestURL(aManifestURL)
  {
    AssertIsInMainProcess();
    MOZ_ASSERT(NS_IsMainThread());
  }

  void
  Run(DataStoreDB* aDb, bool aSuccess)
  {
    AssertIsInMainProcess();
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aDb);

    if (!aSuccess) {
      NS_WARNING("Failed to create the first revision.");
      return;
    }

    mTxn = aDb->Transaction();

    ErrorResult rv;
    nsRefPtr<IDBObjectStore> store =
      mTxn->ObjectStore(NS_LITERAL_STRING(DATASTOREDB_REVISION), rv);
    if (NS_WARN_IF(rv.Failed())) {
      return;
    }

    
    
    mRequest = store->OpenCursor(nullptr, JS::UndefinedHandleValue,
                                 IDBCursorDirection::Prev, rv);
    if (NS_WARN_IF(rv.Failed())) {
      return;
    }

    nsresult res;
    res = mRequest->EventTarget::AddEventListener(NS_LITERAL_STRING("success"),
                                                  this, false);
    if (NS_WARN_IF(NS_FAILED(res))) {
      return;
    }
  }

  
  NS_IMETHOD
  HandleEvent(nsIDOMEvent* aEvent)
  {
    AssertIsInMainProcess();
    MOZ_ASSERT(NS_IsMainThread());

    nsString type;
    nsresult rv = aEvent->GetType(type);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!type.EqualsASCII("success")) {
      return NS_ERROR_FAILURE;
    }

    mRequest->RemoveEventListener(NS_LITERAL_STRING("success"), this, false);

    
    
    AutoSafeJSContext cx;

    ErrorResult error;
    JS::Rooted<JS::Value> result(cx);
    mRequest->GetResult(cx, &result, error);
    if (NS_WARN_IF(error.Failed())) {
      return error.ErrorCode();
    }

    
    
    if (result.isObject()) {
      nsRefPtr<DataStoreService> service = DataStoreService::Get();
      MOZ_ASSERT(service);

      return service->EnableDataStore(mAppId, mName, mManifestURL);
    }

    MOZ_ASSERT(mTxn);
    nsRefPtr<IDBObjectStore> store =
      mTxn->ObjectStore(NS_LITERAL_STRING(DATASTOREDB_REVISION), error);
    if (NS_WARN_IF(error.Failed())) {
      return error.ErrorCode();
    }
    MOZ_ASSERT(store);

    nsRefPtr<RevisionAddedEnableStoreCallback> callback =
      new RevisionAddedEnableStoreCallback(mAppId, mName, mManifestURL);

    
    nsRefPtr<DataStoreRevision> revision = new DataStoreRevision();
    return revision->AddRevision(cx, store, 0, DataStoreRevision::RevisionVoid,
                                 callback);
  }

private:
  ~FirstRevisionIdCallback() {}

  nsRefPtr<IDBRequest> mRequest;

  nsRefPtr<IDBTransaction> mTxn;
  nsRefPtr<DataStoreRevision> mRevision;

  uint32_t mAppId;
  nsString mName;
  nsString mManifestURL;
};

NS_IMPL_ISUPPORTS(FirstRevisionIdCallback, nsIDOMEventListener)







class RetrieveRevisionsCounter
{
private:
  ~RetrieveRevisionsCounter() {}
public:
  NS_INLINE_DECL_REFCOUNTING(RetrieveRevisionsCounter);

  RetrieveRevisionsCounter(uint32_t aId, Promise* aPromise, uint32_t aCount)
    : mPromise(aPromise)
    , mId(aId)
    , mCount(aCount)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  void
  AppendDataStore(JSContext* aCx, DataStore* aDataStore,
                  nsIDataStore* aDataStoreIf)
  {
    MOZ_ASSERT(NS_IsMainThread());

    mResults.AppendElement(aDataStore);

    
    JSFunction* func = js::NewFunctionWithReserved(aCx, JSCallback,
                                                   0 , 0 ,
                                                   nullptr, nullptr);
    if (!func) {
      return;
    }

    JS::Rooted<JSObject*> obj(aCx, JS_GetFunctionObject(func));
    if (!obj) {
      return;
    }

    
    
    js::SetFunctionNativeReserved(obj, 0, JS::Int32Value(mId));

    JS::Rooted<JS::Value> value(aCx, JS::ObjectValue(*obj));
    nsresult rv = aDataStoreIf->RetrieveRevisionId(value);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }
  }

private:
  static bool
  JSCallback(JSContext* aCx, unsigned aArgc, JS::Value* aVp)
  {
    MOZ_ASSERT(NS_IsMainThread());

    JS::CallArgs args = CallArgsFromVp(aArgc, aVp);

    JS::Rooted<JS::Value> value(aCx,
                                js::GetFunctionNativeReserved(&args.callee(), 0));
    uint32_t id = value.toInt32();

    nsRefPtr<DataStoreService> service = DataStoreService::Get();
    MOZ_ASSERT(service);

    nsRefPtr<RetrieveRevisionsCounter> counter = service->GetCounter(id);
    MOZ_ASSERT(counter);

    
    
    --counter->mCount;
    if (!counter->mCount) {
      service->RemoveCounter(id);
      counter->mPromise->MaybeResolve(counter->mResults);
    }

    return true;
  }

  nsRefPtr<Promise> mPromise;
  nsTArray<nsRefPtr<DataStore>> mResults;

  uint32_t mId;
  uint32_t mCount;
};

 already_AddRefed<DataStoreService>
DataStoreService::GetOrCreate()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!gDataStoreService) {
    nsRefPtr<DataStoreService> service = new DataStoreService();
    if (NS_WARN_IF(NS_FAILED(service->Init()))) {
      return nullptr;
    }

    gDataStoreService = service;
  }

  nsRefPtr<DataStoreService> service = gDataStoreService.get();
  return service.forget();
}

 already_AddRefed<DataStoreService>
DataStoreService::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<DataStoreService> service = gDataStoreService.get();
  return service.forget();
}

 void
DataStoreService::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (gDataStoreService) {
    if (IsMainProcess()) {
      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      if (obs) {
        obs->RemoveObserver(gDataStoreService, "webapps-clear-data");
      }
    }

    gDataStoreService = nullptr;
  }
}

NS_INTERFACE_MAP_BEGIN(DataStoreService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDataStoreService)
  NS_INTERFACE_MAP_ENTRY(nsIDataStoreService)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(DataStoreService)
NS_IMPL_RELEASE(DataStoreService)

DataStoreService::DataStoreService()
{
  MOZ_ASSERT(NS_IsMainThread());
}

DataStoreService::~DataStoreService()
{
  MOZ_ASSERT(NS_IsMainThread());
}

nsresult
DataStoreService::Init()
{
  if (!IsMainProcess()) {
    return NS_OK;
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = obs->AddObserver(this, "webapps-clear-data", false);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
DataStoreService::InstallDataStore(uint32_t aAppId,
                                   const nsAString& aName,
                                   const nsAString& aOriginURL,
                                   const nsAString& aManifestURL,
                                   bool aReadOnly)
{
  ASSERT_PARENT_PROCESS()
  MOZ_ASSERT(NS_IsMainThread());

  HashApp* apps = nullptr;
  if (!mStores.Get(aName, &apps)) {
    apps = new HashApp();
    mStores.Put(aName, apps);
  }

  DataStoreInfo* info = nullptr;
  if (!apps->Get(aAppId, &info)) {
    info = new DataStoreInfo(aName, aOriginURL, aManifestURL, aReadOnly, false);
    apps->Put(aAppId, info);
  } else {
    info->Update(aName, aOriginURL, aManifestURL, aReadOnly);
  }

  nsresult rv = AddPermissions(aAppId, aName, aOriginURL, aManifestURL,
                               aReadOnly);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  return CreateFirstRevisionId(aAppId, aName, aManifestURL);
}

NS_IMETHODIMP
DataStoreService::InstallAccessDataStore(uint32_t aAppId,
                                         const nsAString& aName,
                                         const nsAString& aOriginURL,
                                         const nsAString& aManifestURL,
                                         bool aReadOnly)
{
  ASSERT_PARENT_PROCESS()
  MOZ_ASSERT(NS_IsMainThread());

  HashApp* apps = nullptr;
  if (!mAccessStores.Get(aName, &apps)) {
    apps = new HashApp();
    mAccessStores.Put(aName, apps);
  }

  DataStoreInfo* info = nullptr;
  if (!apps->Get(aAppId, &info)) {
    info = new DataStoreInfo(aName, aOriginURL, aManifestURL, aReadOnly, false);
    apps->Put(aAppId, info);
  } else {
    info->Update(aName, aOriginURL, aManifestURL, aReadOnly);
  }

  return AddAccessPermissions(aAppId, aName, aOriginURL, aManifestURL,
                              aReadOnly);
}

NS_IMETHODIMP
DataStoreService::GetDataStores(nsIDOMWindow* aWindow,
                                const nsAString& aName,
                                nsISupports** aDataStores)
{
  
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  if (!window) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(window);
  ErrorResult rv;
  nsRefPtr<Promise> promise = Promise::Create(global, rv);
  if (rv.Failed()) {
    return rv.ErrorCode();
  }

  nsCOMPtr<nsIDocument> document = window->GetDoc();
  MOZ_ASSERT(document);

  nsCOMPtr<nsIPrincipal> principal = document->NodePrincipal();
  MOZ_ASSERT(principal);

  nsTArray<DataStoreInfo> stores;

  
  
  if (IsMainProcess()) {
    uint32_t appId;
    nsresult rv = principal->GetAppId(&appId);
    if (NS_FAILED(rv)) {
      RejectPromise(window, promise, rv);
      promise.forget(aDataStores);
      return NS_OK;
    }

    rv = GetDataStoreInfos(aName, appId, principal, stores);
    if (NS_FAILED(rv)) {
      RejectPromise(window, promise, rv);
      promise.forget(aDataStores);
      return NS_OK;
    }
  }

  else {
    
    
    ContentChild* contentChild = ContentChild::GetSingleton();

    nsTArray<DataStoreSetting> array;
    if (!contentChild->SendDataStoreGetStores(nsAutoString(aName),
                                              IPC::Principal(principal),
                                              &array)) {
      RejectPromise(window, promise, NS_ERROR_FAILURE);
      promise.forget(aDataStores);
      return NS_OK;
    }

    for (uint32_t i = 0; i < array.Length(); ++i) {
      DataStoreInfo* info = stores.AppendElement();
      info->Init(array[i].name(), array[i].originURL(),
                 array[i].manifestURL(), array[i].readOnly(),
                 array[i].enabled());
    }
  }

  GetDataStoresCreate(window, promise, stores);
  promise.forget(aDataStores);
  return NS_OK;
}

void
DataStoreService::GetDataStoresCreate(nsPIDOMWindow* aWindow, Promise* aPromise,
                                      const nsTArray<DataStoreInfo>& aStores)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aStores.Length()) {
    GetDataStoresResolve(aWindow, aPromise, aStores);
    return;
  }

  nsTArray<nsString> pendingDataStores;
  for (uint32_t i = 0; i < aStores.Length(); ++i) {
    if (!aStores[i].mEnabled) {
      pendingDataStores.AppendElement(aStores[i].mManifestURL);
    }
  }

  if (!pendingDataStores.Length()) {
    GetDataStoresResolve(aWindow, aPromise, aStores);
    return;
  }

  PendingRequests* requests;
  if (!mPendingRequests.Get(aStores[0].mName, &requests)) {
    requests = new PendingRequests();
    mPendingRequests.Put(aStores[0].mName, requests);
  }

  PendingRequest* request = requests->AppendElement();
  request->Init(aWindow, aPromise, aStores, pendingDataStores);
}

void
DataStoreService::GetDataStoresResolve(nsPIDOMWindow* aWindow,
                                       Promise* aPromise,
                                       const nsTArray<DataStoreInfo>& aStores)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aStores.Length()) {
    nsTArray<nsRefPtr<DataStore>> results;
    aPromise->MaybeResolve(results);
    return;
  }

  AutoSafeJSContext cx;

  
  
  nsRefPtr<RetrieveRevisionsCounter> counter =
    new RetrieveRevisionsCounter(++gCounterID, aPromise, aStores.Length());
  mPendingCounters.Put(gCounterID, counter);

  for (uint32_t i = 0; i < aStores.Length(); ++i) {
    nsCOMPtr<nsIDataStore> dataStore =
      do_CreateInstance("@mozilla.org/dom/datastore;1");
    if (NS_WARN_IF(!dataStore)) {
      return;
    }

    nsresult rv = dataStore->Init(aWindow, aStores[i].mName,
                                  aStores[i].mManifestURL,
                                  aStores[i].mReadOnly);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    nsCOMPtr<nsIXPConnectWrappedJS> xpcwrappedjs = do_QueryInterface(dataStore);
    if (NS_WARN_IF(!xpcwrappedjs)) {
      return;
    }

    JS::Rooted<JSObject*> dataStoreJS(cx, xpcwrappedjs->GetJSObject());
    if (NS_WARN_IF(!dataStoreJS)) {
      return;
    }

    JSAutoCompartment ac(cx, dataStoreJS);
    nsRefPtr<DataStoreImpl> dataStoreObj = new DataStoreImpl(dataStoreJS,
                                                             aWindow);

    nsRefPtr<DataStore> exposedStore = new DataStore(aWindow);

    ErrorResult error;
    exposedStore->SetDataStoreImpl(*dataStoreObj, error);
    if (error.Failed()) {
      return;
    }

    JS::Rooted<JSObject*> obj(cx, exposedStore->WrapObject(cx));
    MOZ_ASSERT(obj);

    JS::Rooted<JS::Value> exposedObject(cx, JS::ObjectValue(*obj));
    dataStore->SetExposedObject(exposedObject);

    counter->AppendDataStore(cx, exposedStore, dataStore);
  }
}



nsresult
DataStoreService::GetDataStoreInfos(const nsAString& aName,
                                    uint32_t aAppId,
                                    nsIPrincipal* aPrincipal,
                                    nsTArray<DataStoreInfo>& aStores)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIAppsService> appsService =
    do_GetService("@mozilla.org/AppsService;1");
  if (NS_WARN_IF(!appsService)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<mozIApplication> app;
  nsresult rv = appsService->GetAppByLocalId(aAppId, getter_AddRefs(app));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!app) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (!DataStoreService::CheckPermission(aPrincipal)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  aStores.Clear();

  HashApp* apps = nullptr;
  if (!mStores.Get(aName, &apps)) {
    return NS_OK;
  }

  DataStoreInfo* info = nullptr;
  if (apps->Get(aAppId, &info)) {
    DataStoreInfo* owned = aStores.AppendElement();
    owned->Init(info->mName, info->mOriginURL, info->mManifestURL, false,
                info->mEnabled);
  }

  GetDataStoreInfosData data(mAccessStores, aName, aAppId, aStores);
  apps->EnumerateRead(GetDataStoreInfosEnumerator, &data);
  return NS_OK;
}

bool
DataStoreService::CheckPermission(nsIPrincipal* aPrincipal)
{
  
  bool enabled = false;
  Preferences::GetBool("dom.datastore.enabled", &enabled);
  if (!enabled) {
    return false;
  }

  
  if (Preferences::GetBool("dom.testing.datastore_enabled_for_hosted_apps", false)) {
    return true;
  }

  if (!aPrincipal) {
    return false;
  }

  uint16_t status;
  if (NS_FAILED(aPrincipal->GetAppStatus(&status))) {
    return false;
  }

  
  return status == nsIPrincipal::APP_STATUS_CERTIFIED;
}

NS_IMETHODIMP
DataStoreService::CheckPermission(nsIPrincipal* aPrincipal,
                                  bool* aResult)
{
  MOZ_ASSERT(NS_IsMainThread());

  *aResult = DataStoreService::CheckPermission(aPrincipal);

  return NS_OK;
}


void
DataStoreService::DeleteDataStores(uint32_t aAppId)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  mStores.Enumerate(DeleteDataStoresEnumerator, &aAppId);
  mAccessStores.Enumerate(DeleteDataStoresEnumerator, &aAppId);
}

NS_IMETHODIMP
DataStoreService::Observe(nsISupports* aSubject,
                          const char* aTopic,
                          const char16_t* aData)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  if (strcmp(aTopic, "webapps-clear-data")) {
    return NS_OK;
  }

  nsCOMPtr<mozIApplicationClearPrivateDataParams> params =
    do_QueryInterface(aSubject);
  MOZ_ASSERT(params);

  
  bool browserOnly;
  nsresult rv = params->GetBrowserOnly(&browserOnly);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (browserOnly) {
    return NS_OK;
  }

  uint32_t appId;
  rv = params->GetAppId(&appId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DeleteDataStores(appId);

  return NS_OK;
}

nsresult
DataStoreService::AddPermissions(uint32_t aAppId,
                                 const nsAString& aName,
                                 const nsAString& aOriginURL,
                                 const nsAString& aManifestURL,
                                 bool aReadOnly)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  
  nsString permission;
  GeneratePermissionName(permission, aName, aManifestURL);

  
  
  nsresult rv = ResetPermission(aAppId, aOriginURL, aManifestURL, permission,
                                aReadOnly);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  HashApp* apps;
  if (!mAccessStores.Get(aName, &apps)) {
    return NS_OK;
  }

  AddPermissionsData data(permission, aReadOnly);
  apps->EnumerateRead(AddPermissionsEnumerator, &data);
  return data.mResult;
}

nsresult
DataStoreService::AddAccessPermissions(uint32_t aAppId, const nsAString& aName,
                                       const nsAString& aOriginURL,
                                       const nsAString& aManifestURL,
                                       bool aReadOnly)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  
  
  HashApp* apps = nullptr;
  if (!mStores.Get(aName, &apps)) {
    return NS_OK;
  }

  AddAccessPermissionsData data(aAppId, aName, aOriginURL, aReadOnly);
  apps->EnumerateRead(AddAccessPermissionsEnumerator, &data);
  return data.mResult;
}



nsresult
DataStoreService::CreateFirstRevisionId(uint32_t aAppId,
                                        const nsAString& aName,
                                        const nsAString& aManifestURL)
{
  AssertIsInMainProcess();
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<DataStoreDB> db = new DataStoreDB(aManifestURL, aName);

  nsRefPtr<FirstRevisionIdCallback> callback =
    new FirstRevisionIdCallback(aAppId, aName, aManifestURL);

  Sequence<nsString> dbs;
  dbs.AppendElement(NS_LITERAL_STRING(DATASTOREDB_REVISION));

  return db->Open(IDBTransactionMode::Readwrite, dbs, callback);
}

nsresult
DataStoreService::EnableDataStore(uint32_t aAppId, const nsAString& aName,
                                  const nsAString& aManifestURL)
{
  MOZ_ASSERT(NS_IsMainThread());

  {
    HashApp* apps = nullptr;
    DataStoreInfo* info = nullptr;
    if (mStores.Get(aName, &apps) && apps->Get(aAppId, &info)) {
      info->Enable();
    }
  }

  
  if (IsMainProcess()) {
    nsTArray<ContentParent*> children;
    ContentParent::GetAll(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      if (children[i]->NeedsDataStoreInfos()) {
        unused << children[i]->SendDataStoreNotify(aAppId, nsAutoString(aName),
                                                   nsAutoString(aManifestURL));
      }
    }
  }

  
  PendingRequests* requests;
  if (!mPendingRequests.Get(aName, &requests)) {
    return NS_OK;
  }

  for (uint32_t i = 0; i < requests->Length();) {
    PendingRequest& request = requests->ElementAt(i);
    nsTArray<nsString>::index_type pos =
      request.mPendingDataStores.IndexOf(aManifestURL);
    if (pos != request.mPendingDataStores.NoIndex) {
      request.mPendingDataStores.RemoveElementAt(pos);

      
      if (request.mPendingDataStores.IsEmpty()) {
        GetDataStoresResolve(request.mWindow, request.mPromise,
                             request.mStores);
        requests->RemoveElementAt(i);
        continue;
      }
    }

    ++i;
  }

  
  if (requests->IsEmpty()) {
    mPendingRequests.Remove(aName);
  }

  return NS_OK;
}

already_AddRefed<RetrieveRevisionsCounter>
DataStoreService::GetCounter(uint32_t aId) const
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<RetrieveRevisionsCounter> counter;
  return mPendingCounters.Get(aId, getter_AddRefs(counter))
           ? counter.forget() : nullptr;
}

void
DataStoreService::RemoveCounter(uint32_t aId)
{
  MOZ_ASSERT(NS_IsMainThread());
  mPendingCounters.Remove(aId);
}

nsresult
DataStoreService::GetDataStoresFromIPC(const nsAString& aName,
                                       nsIPrincipal* aPrincipal,
                                       nsTArray<DataStoreSetting>* aValue)
{
  MOZ_ASSERT(IsMainProcess());
  MOZ_ASSERT(NS_IsMainThread());

  uint32_t appId;
  nsresult rv = aPrincipal->GetAppId(&appId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsTArray<DataStoreInfo> stores;
  rv = GetDataStoreInfos(aName, appId, aPrincipal, stores);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (uint32_t i = 0; i < stores.Length(); ++i) {
    DataStoreSetting* data = aValue->AppendElement();
    data->name() = stores[i].mName;
    data->originURL() = stores[i].mOriginURL;
    data->manifestURL() = stores[i].mManifestURL;
    data->readOnly() = stores[i].mReadOnly;
    data->enabled() = stores[i].mEnabled;
  }

  return NS_OK;
}

nsresult
DataStoreService::GenerateUUID(nsAString& aID)
{
  nsresult rv;

  if (!mUUIDGenerator) {
    mUUIDGenerator = do_GetService("@mozilla.org/uuid-generator;1", &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  nsID id;
  rv = mUUIDGenerator->GenerateUUIDInPlace(&id);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  char chars[NSID_LENGTH];
  id.ToProvidedString(chars);
  CopyASCIItoUTF16(chars, aID);

  return NS_OK;
}

} 
} 
