




#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/BasePrincipal.h"
#include "mozilla/unused.h"
#include "nsPermissionManager.h"
#include "nsPermission.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsCOMArray.h"
#include "nsArrayEnumerator.h"
#include "nsTArray.h"
#include "nsReadableUtils.h"
#include "nsILineInputStream.h"
#include "nsIIDNService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "prprf.h"
#include "mozilla/storage.h"
#include "mozilla/Attributes.h"
#include "nsXULAppAPI.h"
#include "nsIPrincipal.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIAppsService.h"
#include "mozIApplication.h"
#include "mozIApplicationClearPrivateDataParams.h"
#include "nsIEffectiveTLDService.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"
#include "mozilla/net/NeckoMessageUtils.h"
#include "mozilla/Preferences.h"
#include "nsReadLine.h"
#include "mozilla/Telemetry.h"
#include "nsIConsoleService.h"
#include "nsINavHistoryService.h"
#include "nsToolkitCompsCID.h"

static nsPermissionManager *gPermissionManager = nullptr;

using mozilla::dom::ContentParent;
using mozilla::dom::ContentChild;
using mozilla::unused; 

static bool
IsChildProcess()
{
  return XRE_IsContentProcess();
}





static ContentChild*
ChildProcess()
{
  if (IsChildProcess()) {
    ContentChild* cpc = ContentChild::GetSingleton();
    if (!cpc)
      NS_RUNTIMEABORT("Content Process is nullptr!");
    return cpc;
  }

  return nullptr;
}

static void
LogToConsole(const nsAString& aMsg)
{
  nsCOMPtr<nsIConsoleService> console(do_GetService("@mozilla.org/consoleservice;1"));
  if (!console) {
    NS_WARNING("Failed to log message to console.");
    return;
  }

  nsAutoString msg(aMsg);
  console->LogStringMessage(msg.get());
}

#define ENSURE_NOT_CHILD_PROCESS_(onError) \
  PR_BEGIN_MACRO \
  if (IsChildProcess()) { \
    NS_ERROR("Cannot perform action in content process!"); \
    onError \
  } \
  PR_END_MACRO

#define ENSURE_NOT_CHILD_PROCESS \
  ENSURE_NOT_CHILD_PROCESS_({ return NS_ERROR_NOT_AVAILABLE; })

#define ENSURE_NOT_CHILD_PROCESS_NORET \
  ENSURE_NOT_CHILD_PROCESS_(;)



namespace {

nsresult
GetPrincipalFromOrigin(const nsACString& aOrigin, nsIPrincipal** aPrincipal)
{
  nsAutoCString originNoSuffix;
  mozilla::OriginAttributes attrs;
  if (!attrs.PopulateFromOrigin(aOrigin, originNoSuffix)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), originNoSuffix);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrincipal> principal = mozilla::BasePrincipal::CreateCodebasePrincipal(uri, attrs);
  principal.forget(aPrincipal);
  return NS_OK;
}


nsresult
GetPrincipal(nsIURI* aURI, uint32_t aAppId, bool aIsInBrowserElement, nsIPrincipal** aPrincipal)
{
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

  return secMan->GetAppCodebasePrincipal(aURI, aAppId, aIsInBrowserElement, aPrincipal);
}

nsresult
GetPrincipal(nsIURI* aURI, nsIPrincipal** aPrincipal)
{
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

  return secMan->GetNoAppCodebasePrincipal(aURI, aPrincipal);
}

nsCString
GetNextSubDomainForHost(const nsACString& aHost)
{
  nsCOMPtr<nsIEffectiveTLDService> tldService =
    do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID);
  if (!tldService) {
    NS_ERROR("Should have a tld service!");
    return EmptyCString();
  }

  nsCString subDomain;
  nsresult rv = tldService->GetNextSubDomain(aHost, subDomain);
  
  
  if (NS_FAILED(rv)) {
    return EmptyCString();
  }

  return subDomain;
}

class AppClearDataObserver final : public nsIObserver {
  ~AppClearDataObserver() {}

public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHODIMP
  Observe(nsISupports *aSubject, const char *aTopic, const char16_t *data) override
  {
    MOZ_ASSERT(!nsCRT::strcmp(aTopic, "webapps-clear-data"));

    nsCOMPtr<mozIApplicationClearPrivateDataParams> params =
      do_QueryInterface(aSubject);
    if (!params) {
      NS_ERROR("'webapps-clear-data' notification's subject should be a mozIApplicationClearPrivateDataParams");
      return NS_ERROR_UNEXPECTED;
    }

    uint32_t appId;
    nsresult rv = params->GetAppId(&appId);
    NS_ENSURE_SUCCESS(rv, rv);

    bool browserOnly;
    rv = params->GetBrowserOnly(&browserOnly);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPermissionManager> permManager = do_GetService("@mozilla.org/permissionmanager;1");
    return permManager->RemovePermissionsForApp(appId, browserOnly);
  }
};

NS_IMPL_ISUPPORTS(AppClearDataObserver, nsIObserver)

class MOZ_STACK_CLASS UpgradeHostToOriginHelper {
public:
  virtual nsresult Insert(const nsACString& aOrigin, const nsAFlatCString& aType,
                          uint32_t aPermission, uint32_t aExpireType, int64_t aExpireTime,
                          int64_t aModificationTime) = 0;
};

class UpgradeHostToOriginDBMigration final : public UpgradeHostToOriginHelper {
public:
  UpgradeHostToOriginDBMigration(mozIStorageConnection* aDBConn, int64_t* aID) : mDBConn(aDBConn)
                                                                               , mID(aID)
  {
    mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_hosts_new "
      "(id, origin, type, permission, expireType, expireTime, modificationTime) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)"), getter_AddRefs(mStmt));
  }

  nsresult
  Insert(const nsACString& aOrigin, const nsAFlatCString& aType,
         uint32_t aPermission, uint32_t aExpireType, int64_t aExpireTime,
         int64_t aModificationTime) final
  {
    nsresult rv = mStmt->BindInt64ByIndex(0, *mID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStmt->BindUTF8StringByIndex(1, aOrigin);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStmt->BindUTF8StringByIndex(2, aType);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStmt->BindInt32ByIndex(3, aPermission);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStmt->BindInt32ByIndex(4, aExpireType);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStmt->BindInt64ByIndex(5, aExpireTime);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStmt->BindInt64ByIndex(6, aModificationTime);
    NS_ENSURE_SUCCESS(rv, rv);

    
    (*mID)++;

    rv = mStmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  nsCOMPtr<mozIStorageStatement> mStmt;
  nsCOMPtr<mozIStorageConnection> mDBConn;
  int64_t* mID;
};

class UpgradeHostToOriginHostfileImport final : public UpgradeHostToOriginHelper {
public:
  UpgradeHostToOriginHostfileImport(nsPermissionManager* aPm,
                                    nsPermissionManager::DBOperationType aOperation,
                                    int64_t aID) : mPm(aPm)
                                                 , mOperation(aOperation)
                                                 , mID(aID)
  {}

  nsresult
  Insert(const nsACString& aOrigin, const nsAFlatCString& aType,
         uint32_t aPermission, uint32_t aExpireType, int64_t aExpireTime,
         int64_t aModificationTime) final
  {
    nsCOMPtr<nsIPrincipal> principal;
    nsresult rv = GetPrincipalFromOrigin(aOrigin, getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    return mPm->AddInternal(principal, aType, aPermission, mID,
                            aExpireType, aExpireTime, aModificationTime,
                            nsPermissionManager::eDontNotify, mOperation);
  }

private:
  nsRefPtr<nsPermissionManager> mPm;
  nsPermissionManager::DBOperationType mOperation;
  int64_t mID;
};

nsresult
UpgradeHostToOriginAndInsert(const nsACString& aHost, const nsAFlatCString& aType,
                             uint32_t aPermission, uint32_t aExpireType, int64_t aExpireTime,
                             int64_t aModificationTime, uint32_t aAppId, bool aIsInBrowserElement,
                             UpgradeHostToOriginHelper* aHelper)
{
  if (aHost.EqualsLiteral("<file>")) {
    
    NS_WARNING("The magic host <file> is no longer supported. "
               "It is being removed from the permissions database.");
    return NS_OK;
  }

  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHost);
  if (NS_SUCCEEDED(rv)) {
    
    
    
    bool nullpScheme = false;
    if (NS_SUCCEEDED(uri->SchemeIs("moz-nullprincipal", &nullpScheme)) && nullpScheme) {
      NS_WARNING("A moz-nullprincipal: permission is being discarded.");
      return NS_OK;
    }

    nsCOMPtr<nsIPrincipal> principal;
    rv = GetPrincipal(uri, aAppId, aIsInBrowserElement, getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString origin;
    rv = principal->GetOrigin(origin);
    NS_ENSURE_SUCCESS(rv, rv);

    return aHelper->Insert(origin, aType, aPermission,
                           aExpireType, aExpireTime, aModificationTime);
    return NS_OK;
  }

  
  
  
  
  bool foundHistory = false;

  nsCOMPtr<nsINavHistoryService> histSrv = do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);

  if (histSrv) {
    nsCOMPtr<nsINavHistoryQuery> histQuery;
    rv = histSrv->GetNewQuery(getter_AddRefs(histQuery));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = histQuery->SetDomain(aHost);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = histQuery->SetDomainIsHost(false);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsINavHistoryQueryOptions> histQueryOpts;
    rv = histSrv->GetNewQueryOptions(getter_AddRefs(histQueryOpts));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = histQueryOpts->SetResultType(nsINavHistoryQueryOptions::RESULTS_AS_URI);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = histQueryOpts->SetQueryType(nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = histQueryOpts->SetIncludeHidden(true);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsINavHistoryResult> histResult;
    rv = histSrv->ExecuteQuery(histQuery, histQueryOpts, getter_AddRefs(histResult));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsINavHistoryContainerResultNode> histResultContainer;
    rv = histResult->GetRoot(getter_AddRefs(histResultContainer));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = histResultContainer->SetContainerOpen(true);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t childCount = 0;
    rv = histResultContainer->GetChildCount(&childCount);
    NS_ENSURE_SUCCESS(rv, rv);

    nsTHashtable<nsCStringHashKey> insertedOrigins;
    for (uint32_t i = 0; i < childCount; i++) {
      nsCOMPtr<nsINavHistoryResultNode> child;
      histResultContainer->GetChild(i, getter_AddRefs(child));
      if (NS_FAILED(rv)) continue;

      uint32_t type;
      rv = child->GetType(&type);
      if (NS_FAILED(rv) || type != nsINavHistoryResultNode::RESULT_TYPE_URI) {
        NS_WARNING("Unexpected non-RESULT_TYPE_URI node in "
                   "UpgradeHostToOriginAndInsert()");
        continue;
      }

      nsAutoCString uriSpec;
      rv = child->GetUri(uriSpec);
      if (NS_FAILED(rv)) continue;

      nsCOMPtr<nsIURI> uri;
      rv = NS_NewURI(getter_AddRefs(uri), uriSpec);
      if (NS_FAILED(rv)) continue;

      
      rv = uri->SetHost(aHost);
      if (NS_FAILED(rv)) continue;

      
      nsCOMPtr<nsIPrincipal> principal;
      rv = GetPrincipal(uri, aAppId, aIsInBrowserElement, getter_AddRefs(principal));
      if (NS_FAILED(rv)) continue;

      nsAutoCString origin;
      rv = principal->GetOrigin(origin);
      if (NS_FAILED(rv)) continue;

      
      if (insertedOrigins.Contains(origin)) {
        continue;
      }

      foundHistory = true;
      rv = aHelper->Insert(origin, aType, aPermission,
                           aExpireType, aExpireTime, aModificationTime);
      NS_WARN_IF(NS_FAILED(rv));
      insertedOrigins.PutEntry(origin);
    }

    rv = histResultContainer->SetContainerOpen(false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  if (!foundHistory) {
    rv = NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING("http://") + aHost);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIPrincipal> principal;
      rv = GetPrincipal(uri, aAppId, aIsInBrowserElement, getter_AddRefs(principal));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoCString origin;
      rv = principal->GetOrigin(origin);
      NS_ENSURE_SUCCESS(rv, rv);

      aHelper->Insert(origin, aType, aPermission,
                      aExpireType, aExpireTime, aModificationTime);
    }
    rv = NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING("https://") + aHost);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIPrincipal> principal;
      rv = GetPrincipal(uri, aAppId, aIsInBrowserElement, getter_AddRefs(principal));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoCString origin;
      rv = principal->GetOrigin(origin);
      NS_ENSURE_SUCCESS(rv, rv);

      aHelper->Insert(origin, aType, aPermission,
                      aExpireType, aExpireTime, aModificationTime);
    }
  }

  return NS_OK;
}

static bool
IsExpandedPrincipal(nsIPrincipal* aPrincipal)
{
  nsCOMPtr<nsIExpandedPrincipal> ep = do_QueryInterface(aPrincipal);
  return !!ep;
}

} 



nsPermissionManager::PermissionKey::PermissionKey(nsIPrincipal* aPrincipal)
{
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aPrincipal->GetOrigin(mOrigin)));
}











class CloseDatabaseListener final : public mozIStorageCompletionCallback
{
  ~CloseDatabaseListener() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGECOMPLETIONCALLBACK
  




  CloseDatabaseListener(nsPermissionManager* aManager,
                        bool aRebuildOnSuccess);

protected:
  nsRefPtr<nsPermissionManager> mManager;
  bool mRebuildOnSuccess;
};

NS_IMPL_ISUPPORTS(CloseDatabaseListener, mozIStorageCompletionCallback)

CloseDatabaseListener::CloseDatabaseListener(nsPermissionManager* aManager,
                                             bool aRebuildOnSuccess)
  : mManager(aManager)
  , mRebuildOnSuccess(aRebuildOnSuccess)
{
}

NS_IMETHODIMP
CloseDatabaseListener::Complete(nsresult, nsISupports*)
{
  
  nsRefPtr<nsPermissionManager> manager = mManager.forget();
  if (mRebuildOnSuccess && !manager->mIsShuttingDown) {
    return manager->InitDB(true);
  }
  return NS_OK;
}












class DeleteFromMozHostListener final : public mozIStorageStatementCallback
{
  ~DeleteFromMozHostListener() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK

  


  explicit DeleteFromMozHostListener(nsPermissionManager* aManager);

protected:
  nsRefPtr<nsPermissionManager> mManager;
};

NS_IMPL_ISUPPORTS(DeleteFromMozHostListener, mozIStorageStatementCallback)

DeleteFromMozHostListener::
DeleteFromMozHostListener(nsPermissionManager* aManager)
  : mManager(aManager)
{
}

NS_IMETHODIMP DeleteFromMozHostListener::HandleResult(mozIStorageResultSet *)
{
  MOZ_CRASH("Should not get any results");
}

NS_IMETHODIMP DeleteFromMozHostListener::HandleError(mozIStorageError *)
{
  
  return NS_OK;
}

NS_IMETHODIMP DeleteFromMozHostListener::HandleCompletion(uint16_t aReason)
{
  
  nsRefPtr<nsPermissionManager> manager = mManager.forget();

  if (aReason == REASON_ERROR) {
    manager->CloseDB(true);
  }

  return NS_OK;
}

 void
nsPermissionManager::AppClearDataObserverInit()
{
  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  observerService->AddObserver(new AppClearDataObserver(), "webapps-clear-data",  false);
}




#define PERMISSIONS_FILE_NAME "permissions.sqlite"
#define HOSTS_SCHEMA_VERSION 6

#define HOSTPERM_FILE_NAME "hostperm.1"



static const char kDefaultsUrlPrefName[] = "permissions.manager.defaultsUrl";

static const char kPermissionChangeNotification[] = PERM_CHANGE_NOTIFICATION;

NS_IMPL_ISUPPORTS(nsPermissionManager, nsIPermissionManager, nsIObserver, nsISupportsWeakReference)

nsPermissionManager::nsPermissionManager()
 : mMemoryOnlyDB(false)
 , mLargestID(0)
 , mIsShuttingDown(false)
{
}

nsPermissionManager::~nsPermissionManager()
{
  RemoveAllFromMemory();
  gPermissionManager = nullptr;
}


nsIPermissionManager*
nsPermissionManager::GetXPCOMSingleton()
{
  if (gPermissionManager) {
    NS_ADDREF(gPermissionManager);
    return gPermissionManager;
  }

  
  
  
  
  
  
  gPermissionManager = new nsPermissionManager();
  if (gPermissionManager) {
    NS_ADDREF(gPermissionManager);
    if (NS_FAILED(gPermissionManager->Init())) {
      NS_RELEASE(gPermissionManager);
    }
  }

  return gPermissionManager;
}

nsresult
nsPermissionManager::Init()
{
  nsresult rv;

  
  
  mMemoryOnlyDB = mozilla::Preferences::GetBool("permissions.memory_only", false);

  mObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    mObserverService->AddObserver(this, "profile-before-change", true);
    mObserverService->AddObserver(this, "profile-do-change", true);
    mObserverService->AddObserver(this, "xpcom-shutdown", true);
  }

  if (IsChildProcess()) {
    
    return FetchPermissions();
  }

  
  
  
  InitDB(false);

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::RefreshPermission() {
  NS_ENSURE_TRUE(IsChildProcess(), NS_ERROR_FAILURE);

  nsresult rv = RemoveAllFromMemory();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = FetchPermissions();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsPermissionManager::OpenDatabase(nsIFile* aPermissionsFile)
{
  nsresult rv;
  nsCOMPtr<mozIStorageService> storage = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  if (!storage) {
    return NS_ERROR_UNEXPECTED;
  }
  
  if (mMemoryOnlyDB) {
    rv = storage->OpenSpecialDatabase("memory", getter_AddRefs(mDBConn));
  } else {
    rv = storage->OpenDatabase(aPermissionsFile, getter_AddRefs(mDBConn));
  }
  return rv;
}

nsresult
nsPermissionManager::InitDB(bool aRemoveFile)
{
  nsCOMPtr<nsIFile> permissionsFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_PERMISSION_PARENT_DIR, getter_AddRefs(permissionsFile));
  if (NS_FAILED(rv)) {
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(permissionsFile));
  }
  NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

  rv = permissionsFile->AppendNative(NS_LITERAL_CSTRING(PERMISSIONS_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRemoveFile) {
    bool exists = false;
    rv = permissionsFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (exists) {
      rv = permissionsFile->Remove(false);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = OpenDatabase(permissionsFile);
  if (rv == NS_ERROR_UNEXPECTED) {
    return rv;
  } else if (rv == NS_ERROR_FILE_CORRUPTED) {
    LogToConsole(NS_LITERAL_STRING("permissions.sqlite is corrupted! Try again!"));

    
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::PERMISSIONS_SQL_CORRUPTED, 1);

    
    rv = permissionsFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);
    LogToConsole(NS_LITERAL_STRING("Corrupted permissions.sqlite has been removed."));

    rv = OpenDatabase(permissionsFile);
    NS_ENSURE_SUCCESS(rv, rv);
    LogToConsole(NS_LITERAL_STRING("OpenDatabase to permissions.sqlite is successful!"));
  }

  bool ready;
  mDBConn->GetConnectionReady(&ready);
  if (!ready) {
    LogToConsole(NS_LITERAL_STRING("Fail to get connection to permissions.sqlite! Try again!"));

    
    rv = permissionsFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);
    LogToConsole(NS_LITERAL_STRING("Defective permissions.sqlite has been removed."));

    
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::DEFECTIVE_PERMISSIONS_SQL_REMOVED, 1);

    rv = OpenDatabase(permissionsFile);
    NS_ENSURE_SUCCESS(rv, rv);
    LogToConsole(NS_LITERAL_STRING("OpenDatabase to permissions.sqlite is successful!"));

    mDBConn->GetConnectionReady(&ready);
    if (!ready)
      return NS_ERROR_UNEXPECTED;
  }

  LogToConsole(NS_LITERAL_STRING("Get a connection to permissions.sqlite."));

  bool tableExists = false;
  mDBConn->TableExists(NS_LITERAL_CSTRING("moz_perms"), &tableExists);
  if (!tableExists) {
    mDBConn->TableExists(NS_LITERAL_CSTRING("moz_hosts"), &tableExists);
  }
  if (!tableExists) {
    rv = CreateTable();
    NS_ENSURE_SUCCESS(rv, rv);
    LogToConsole(NS_LITERAL_STRING("DB table(moz_perms) is created!"));
  } else {
    
    int32_t dbSchemaVersion;
    rv = mDBConn->GetSchemaVersion(&dbSchemaVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    switch (dbSchemaVersion) {
    
    
    
    

    case 1:
      {
        
        
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
              "ALTER TABLE moz_hosts ADD expireType INTEGER"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
              "ALTER TABLE moz_hosts ADD expireTime INTEGER"));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

    
    case 0:
    case 2:
      {
        
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
              "ALTER TABLE moz_hosts ADD appId INTEGER"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
              "ALTER TABLE moz_hosts ADD isInBrowserElement INTEGER"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDBConn->SetSchemaVersion(HOSTS_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

    
    case 3:
      {
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
              "ALTER TABLE moz_hosts ADD modificationTime INTEGER"));
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        

        rv = mDBConn->SetSchemaVersion(HOSTS_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

    
    case 4:
      {
        bool tableExists = false;
        mDBConn->TableExists(NS_LITERAL_CSTRING("moz_hosts_new"), &tableExists);
        if (tableExists) {
          rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE moz_hosts_new"));
          NS_ENSURE_SUCCESS(rv, rv);
        }
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "CREATE TABLE moz_hosts_new ("
            " id INTEGER PRIMARY KEY"
            ",origin TEXT"
            ",type TEXT"
            ",permission INTEGER"
            ",expireType INTEGER"
            ",expireTime INTEGER"
            ",modificationTime INTEGER"
          ")"));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<mozIStorageStatement> stmt;
        rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT host, type, permission, expireType, expireTime, modificationTime, appId, isInBrowserElement "
          "FROM moz_hosts"), getter_AddRefs(stmt));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<mozIStoragePendingStatement> pending;

        int64_t id = 0;
        nsAutoCString host, type;
        uint32_t permission;
        uint32_t expireType;
        int64_t expireTime;
        int64_t modificationTime;
        uint32_t appId;
        bool isInBrowserElement;
        bool hasResult;

        rv = mDBConn->BeginTransaction();
        NS_ENSURE_SUCCESS(rv, rv);

        while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
          
          rv = stmt->GetUTF8String(0, host);
          if (NS_FAILED(rv)) {
            continue;
          }
          rv = stmt->GetUTF8String(1, type);
          if (NS_FAILED(rv)) {
            continue;
          }
          permission = stmt->AsInt32(2);
          expireType = stmt->AsInt32(3);
          expireTime = stmt->AsInt64(4);
          modificationTime = stmt->AsInt64(5);
          if (stmt->AsInt64(6) < 0) {
            continue;
          }
          appId = static_cast<uint32_t>(stmt->AsInt64(6));
          isInBrowserElement = static_cast<bool>(stmt->AsInt32(7));

          UpgradeHostToOriginDBMigration upHelper(mDBConn, &id);
          rv = UpgradeHostToOriginAndInsert(host, type, permission,
                                            expireType, expireTime,
                                            modificationTime, appId,
                                            isInBrowserElement,
                                            &upHelper);
          if (NS_FAILED(rv)) {
            NS_WARNING("Unexpected failure when upgrading migrating permission from host to origin");
          }
        }

        rv = mDBConn->CommitTransaction();
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        
        bool v4TableExists = false;
        mDBConn->TableExists(NS_LITERAL_CSTRING("moz_hosts_v4"), &v4TableExists);
        if (!v4TableExists) {
          rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("ALTER TABLE moz_hosts RENAME TO moz_hosts_v4"));
          NS_ENSURE_SUCCESS(rv, rv);
        } else {
          NS_WARNING("moz_hosts was not renamed to moz_hosts_v4, as a moz_hosts_v4 table already exists");

          rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE moz_hosts"));
          NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("ALTER TABLE moz_hosts_new RENAME TO moz_hosts"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDBConn->SetSchemaVersion(HOSTS_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    case 5:
      {
        bool permsTableExists = false;
        mDBConn->TableExists(NS_LITERAL_CSTRING("moz_perms"), &permsTableExists);
        if (!permsTableExists) {
          
          rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("ALTER TABLE moz_hosts RENAME TO moz_perms"));
          NS_ENSURE_SUCCESS(rv, rv);
        } else {
          NS_WARNING("moz_hosts was not renamed to moz_perms, as a moz_perms table already exists");

          
          
          
          
          
          
          
          rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE moz_hosts"));
          NS_ENSURE_SUCCESS(rv, rv);
        }

#ifdef DEBUG
        
        bool hostsTableExists = false;
        mDBConn->TableExists(NS_LITERAL_CSTRING("moz_hosts"), &hostsTableExists);
        MOZ_ASSERT(!hostsTableExists);
#endif

        
        bool v4TableExists = false;
        mDBConn->TableExists(NS_LITERAL_CSTRING("moz_hosts_v4"), &v4TableExists);
        if (v4TableExists) {
          rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("ALTER TABLE moz_hosts_v4 RENAME TO moz_hosts"));
          NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = mDBConn->SetSchemaVersion(HOSTS_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }

    
    case HOSTS_SCHEMA_VERSION:
      break;

    
    
    
    
    
    
    default:
      {
        
        nsCOMPtr<mozIStorageStatement> stmt;
        rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT origin, type, permission, expireType, expireTime, modificationTime FROM moz_perms"),
          getter_AddRefs(stmt));
        if (NS_SUCCEEDED(rv))
          break;

        
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE moz_perms"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = CreateTable();
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
    }
  }

  
  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_perms "
    "(id, origin, type, permission, expireType, expireTime, modificationTime) "
    "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)"), getter_AddRefs(mStmtInsert));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_perms "
    "WHERE id = ?1"), getter_AddRefs(mStmtDelete));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_perms "
    "SET permission = ?2, expireType= ?3, expireTime = ?4, modificationTime = ?5 WHERE id = ?1"),
    getter_AddRefs(mStmtUpdate));
  NS_ENSURE_SUCCESS(rv, rv);

  
  ImportDefaults();
  
  if (tableExists)
    return Read();

  return Import();
}


nsresult
nsPermissionManager::CreateTable()
{
  
  nsresult rv = mDBConn->SetSchemaVersion(HOSTS_SCHEMA_VERSION);
  if (NS_FAILED(rv)) return rv;

  
  
  
  return mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE moz_perms ("
      " id INTEGER PRIMARY KEY"
      ",origin TEXT"
      ",type TEXT"
      ",permission INTEGER"
      ",expireType INTEGER"
      ",expireTime INTEGER"
      ",modificationTime INTEGER"
    ")"));
}

NS_IMETHODIMP
nsPermissionManager::Add(nsIURI     *aURI,
                         const char *aType,
                         uint32_t    aPermission,
                         uint32_t    aExpireType,
                         int64_t     aExpireTime)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = GetPrincipal(aURI, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  return AddFromPrincipal(principal, aType, aPermission, aExpireType, aExpireTime);
}

NS_IMETHODIMP
nsPermissionManager::AddFromPrincipal(nsIPrincipal* aPrincipal,
                                      const char* aType, uint32_t aPermission,
                                      uint32_t aExpireType, int64_t aExpireTime)
{
  ENSURE_NOT_CHILD_PROCESS;
  NS_ENSURE_ARG_POINTER(aPrincipal);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_TRUE(aExpireType == nsIPermissionManager::EXPIRE_NEVER ||
                 aExpireType == nsIPermissionManager::EXPIRE_TIME ||
                 aExpireType == nsIPermissionManager::EXPIRE_SESSION,
                 NS_ERROR_INVALID_ARG);

  
  
  if ((aExpireType == nsIPermissionManager::EXPIRE_TIME ||
       (aExpireType == nsIPermissionManager::EXPIRE_SESSION && aExpireTime != 0)) &&
      aExpireTime <= (PR_Now() / 1000)) {
    return NS_OK;
  }

  
  
  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    return NS_OK;
  }

  
  
  bool isNullPrincipal;
  nsresult rv = aPrincipal->GetIsNullPrincipal(&isNullPrincipal);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isNullPrincipal) {
    return NS_OK;
  }

  
  if (IsExpandedPrincipal(aPrincipal)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  int64_t modificationTime = 0;

  return AddInternal(aPrincipal, nsDependentCString(aType), aPermission, 0,
                     aExpireType, aExpireTime, modificationTime, eNotify, eWriteToDB);
}

nsresult
nsPermissionManager::AddInternal(nsIPrincipal* aPrincipal,
                                 const nsAFlatCString &aType,
                                 uint32_t              aPermission,
                                 int64_t               aID,
                                 uint32_t              aExpireType,
                                 int64_t               aExpireTime,
                                 int64_t               aModificationTime,
                                 NotifyOperationType   aNotifyOperation,
                                 DBOperationType       aDBOperation,
                                 const bool            aIgnoreSessionPermissions)
{
  nsAutoCString origin;
  nsresult rv = aPrincipal->GetOrigin(origin);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!IsChildProcess()) {
    IPC::Permission permission(origin, aType, aPermission,
                               aExpireType, aExpireTime);

    nsTArray<ContentParent*> cplist;
    ContentParent::GetAll(cplist);
    for (uint32_t i = 0; i < cplist.Length(); ++i) {
      ContentParent* cp = cplist[i];
      
      
      
      if (cp->IsPreallocated() &&
          aExpireType == nsIPermissionManager::EXPIRE_SESSION)
        continue;
      if (cp->NeedsPermissionsUpdate())
        unused << cp->SendAddPermission(permission);
    }
  }

  
  int32_t typeIndex = GetTypeIndex(aType.get(), true);
  NS_ENSURE_TRUE(typeIndex != -1, NS_ERROR_OUT_OF_MEMORY);

  
  
  nsRefPtr<PermissionKey> key = new PermissionKey(aPrincipal);
  PermissionHashKey* entry = mPermissionTable.PutEntry(key);
  if (!entry) return NS_ERROR_FAILURE;
  if (!entry->GetKey()) {
    mPermissionTable.RawRemoveEntry(entry);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  OperationType op;
  int32_t index = entry->GetPermissionIndex(typeIndex);
  if (index == -1) {
    if (aPermission == nsIPermissionManager::UNKNOWN_ACTION)
      op = eOperationNone;
    else
      op = eOperationAdding;

  } else {
    PermissionEntry oldPermissionEntry = entry->GetPermissions()[index];

    
    
    
    
    
    if (aPermission == oldPermissionEntry.mPermission &&
        aExpireType == oldPermissionEntry.mExpireType &&
        (aExpireType == nsIPermissionManager::EXPIRE_NEVER ||
         aExpireTime == oldPermissionEntry.mExpireTime))
      op = eOperationNone;
    else if (oldPermissionEntry.mID == cIDPermissionIsDefault)
      
      
      
      
      op = eOperationReplacingDefault;
    else if (aID == cIDPermissionIsDefault)
      
      
      
      op = eOperationNone;
    else if (aPermission == nsIPermissionManager::UNKNOWN_ACTION)
      op = eOperationRemoving;
    else
      op = eOperationChanging;
  }

  
  MOZ_ASSERT(!IsChildProcess() || aModificationTime == 0);

  
  
  int64_t id;
  if (aModificationTime == 0) {
    aModificationTime = PR_Now() / 1000;
  }

  switch (op) {
  case eOperationNone:
    {
      
      return NS_OK;
    }

  case eOperationAdding:
    {
      if (aDBOperation == eWriteToDB) {
        
        id = ++mLargestID;
      } else {
        
        id = aID;
      }

      
      
      
      
      if (aIgnoreSessionPermissions &&
          aExpireType == nsIPermissionManager::EXPIRE_SESSION) {
        aPermission = nsIPermissionManager::PROMPT_ACTION;
        aExpireType = nsIPermissionManager::EXPIRE_NEVER;
      }

      entry->GetPermissions().AppendElement(PermissionEntry(id, typeIndex, aPermission,
                                                            aExpireType, aExpireTime,
                                                            aModificationTime));

      if (aDBOperation == eWriteToDB && aExpireType != nsIPermissionManager::EXPIRE_SESSION) {
        UpdateDB(op, mStmtInsert, id, origin, aType, aPermission, aExpireType, aExpireTime, aModificationTime);
      }

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(aPrincipal,
                                      mTypeArray[typeIndex],
                                      aPermission,
                                      aExpireType,
                                      aExpireTime,
                                      MOZ_UTF16("added"));
      }

      break;
    }

  case eOperationRemoving:
    {
      PermissionEntry oldPermissionEntry = entry->GetPermissions()[index];
      id = oldPermissionEntry.mID;
      entry->GetPermissions().RemoveElementAt(index);

      if (aDBOperation == eWriteToDB)
        
        
        UpdateDB(op, mStmtDelete, id, EmptyCString(), EmptyCString(), 0,
                 nsIPermissionManager::EXPIRE_NEVER, 0, 0);

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(aPrincipal,
                                      mTypeArray[typeIndex],
                                      oldPermissionEntry.mPermission,
                                      oldPermissionEntry.mExpireType,
                                      oldPermissionEntry.mExpireTime,
                                      MOZ_UTF16("deleted"));
      }

      
      if (entry->GetPermissions().IsEmpty()) {
        mPermissionTable.RawRemoveEntry(entry);
      }

      break;
    }

  case eOperationChanging:
    {
      id = entry->GetPermissions()[index].mID;

      
      
      
      if (entry->GetPermissions()[index].mExpireType != nsIPermissionManager::EXPIRE_SESSION &&
          aExpireType == nsIPermissionManager::EXPIRE_SESSION) {
        entry->GetPermissions()[index].mNonSessionPermission = entry->GetPermissions()[index].mPermission;
        entry->GetPermissions()[index].mNonSessionExpireType = entry->GetPermissions()[index].mExpireType;
        entry->GetPermissions()[index].mNonSessionExpireTime = entry->GetPermissions()[index].mExpireTime;
      } else if (aExpireType != nsIPermissionManager::EXPIRE_SESSION) {
        entry->GetPermissions()[index].mNonSessionPermission = aPermission;
        entry->GetPermissions()[index].mNonSessionExpireType = aExpireType;
        entry->GetPermissions()[index].mNonSessionExpireTime = aExpireTime;
      }

      entry->GetPermissions()[index].mPermission = aPermission;
      entry->GetPermissions()[index].mExpireType = aExpireType;
      entry->GetPermissions()[index].mExpireTime = aExpireTime;
      entry->GetPermissions()[index].mModificationTime = aModificationTime;

      if (aDBOperation == eWriteToDB && aExpireType != nsIPermissionManager::EXPIRE_SESSION)
        
        
        UpdateDB(op, mStmtUpdate, id, EmptyCString(), EmptyCString(),
                 aPermission, aExpireType, aExpireTime, aModificationTime);

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(aPrincipal,
                                      mTypeArray[typeIndex],
                                      aPermission,
                                      aExpireType,
                                      aExpireTime,
                                      MOZ_UTF16("changed"));
      }

      break;
    }
  case eOperationReplacingDefault:
    {
      
      
      
      
      

      
      
      

      
      
      id = ++mLargestID;

      
      NS_ENSURE_TRUE(entry->GetPermissions()[index].mExpireType != nsIPermissionManager::EXPIRE_SESSION,
                     NS_ERROR_UNEXPECTED);
      
      
      
      NS_ENSURE_TRUE(aExpireType == EXPIRE_NEVER, NS_ERROR_UNEXPECTED);

      
      entry->GetPermissions()[index].mID = id;
      entry->GetPermissions()[index].mPermission = aPermission;
      entry->GetPermissions()[index].mExpireType = aExpireType;
      entry->GetPermissions()[index].mExpireTime = aExpireTime;
      entry->GetPermissions()[index].mModificationTime = aModificationTime;

      
      if (aDBOperation == eWriteToDB) {
        uint32_t appId;
        rv = aPrincipal->GetAppId(&appId);
        NS_ENSURE_SUCCESS(rv, rv);

        bool isInBrowserElement;
        rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
        NS_ENSURE_SUCCESS(rv, rv);

        UpdateDB(eOperationAdding, mStmtInsert, id, origin, aType, aPermission,
                 aExpireType, aExpireTime, aModificationTime);
      }

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(aPrincipal,
                                      mTypeArray[typeIndex],
                                      aPermission,
                                      aExpireType,
                                      aExpireTime,
                                      MOZ_UTF16("changed"));
      }

    }
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::Remove(nsIURI*     aURI,
                            const char* aType)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = GetPrincipal(aURI, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  return RemoveFromPrincipal(principal, aType);
}

NS_IMETHODIMP
nsPermissionManager::RemoveFromPrincipal(nsIPrincipal* aPrincipal,
                                         const char* aType)
{
  ENSURE_NOT_CHILD_PROCESS;
  NS_ENSURE_ARG_POINTER(aPrincipal);
  NS_ENSURE_ARG_POINTER(aType);

  
  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    return NS_OK;
  }

  
  if (IsExpandedPrincipal(aPrincipal)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  return AddInternal(aPrincipal,
                     nsDependentCString(aType),
                     nsIPermissionManager::UNKNOWN_ACTION,
                     0,
                     nsIPermissionManager::EXPIRE_NEVER,
                     0,
                     0,
                     eNotify,
                     eWriteToDB);
}

NS_IMETHODIMP
nsPermissionManager::RemovePermission(nsIPermission* aPerm)
{
  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = aPerm->GetPrincipal(getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString type;
  rv = aPerm->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  return RemoveFromPrincipal(principal, type.get());
}

NS_IMETHODIMP
nsPermissionManager::RemoveAll()
{
  ENSURE_NOT_CHILD_PROCESS;
  return RemoveAllInternal(true);
}

NS_IMETHODIMP
nsPermissionManager::RemoveAllSince(int64_t aSince)
{
  ENSURE_NOT_CHILD_PROCESS;
  return RemoveAllModifiedSince(aSince);
}

void
nsPermissionManager::CloseDB(bool aRebuildOnSuccess)
{
  
  mStmtInsert = nullptr;
  mStmtDelete = nullptr;
  mStmtUpdate = nullptr;
  if (mDBConn) {
    mozIStorageCompletionCallback* cb = new CloseDatabaseListener(this,
           aRebuildOnSuccess);
    mozilla::DebugOnly<nsresult> rv = mDBConn->AsyncClose(cb);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    mDBConn = nullptr; 
  }
}

nsresult
nsPermissionManager::RemoveAllInternal(bool aNotifyObservers)
{
  
  
  
  RemoveAllFromMemory();

  
  ImportDefaults();

  if (aNotifyObservers) {
    NotifyObservers(nullptr, MOZ_UTF16("cleared"));
  }

  
  if (mDBConn) {
    nsCOMPtr<mozIStorageAsyncStatement> removeStmt;
    nsresult rv = mDBConn->
      CreateAsyncStatement(NS_LITERAL_CSTRING(
         "DELETE FROM moz_perms"
      ), getter_AddRefs(removeStmt));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    if (!removeStmt) {
      return NS_ERROR_UNEXPECTED;
    }
    nsCOMPtr<mozIStoragePendingStatement> pending;
    mozIStorageStatementCallback* cb = new DeleteFromMozHostListener(this);
    rv = removeStmt->ExecuteAsync(cb, getter_AddRefs(pending));
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::TestExactPermission(nsIURI     *aURI,
                                         const char *aType,
                                         uint32_t   *aPermission)
{
  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = GetPrincipal(aURI, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  return TestExactPermissionFromPrincipal(principal, aType, aPermission);
}

NS_IMETHODIMP
nsPermissionManager::TestExactPermissionFromPrincipal(nsIPrincipal* aPrincipal,
                                                      const char* aType,
                                                      uint32_t* aPermission)
{
  return CommonTestPermission(aPrincipal, aType, aPermission, true, true);
}

NS_IMETHODIMP
nsPermissionManager::TestExactPermanentPermission(nsIPrincipal* aPrincipal,
                                                  const char* aType,
                                                  uint32_t* aPermission)
{
  return CommonTestPermission(aPrincipal, aType, aPermission, true, false);
}

NS_IMETHODIMP
nsPermissionManager::TestPermission(nsIURI     *aURI,
                                    const char *aType,
                                    uint32_t   *aPermission)
{
  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = GetPrincipal(aURI, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  return TestPermissionFromPrincipal(principal, aType, aPermission);
}

NS_IMETHODIMP
nsPermissionManager::TestPermissionFromWindow(nsIDOMWindow* aWindow,
                                              const char* aType,
                                              uint32_t* aPermission)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(window, NS_NOINTERFACE);

  nsPIDOMWindow* innerWindow = window->IsInnerWindow() ?
    window.get() :
    window->GetCurrentInnerWindow();

  
  nsCOMPtr<nsIDocument> document = innerWindow->GetExtantDoc();
  NS_ENSURE_TRUE(document, NS_NOINTERFACE);

  nsCOMPtr<nsIPrincipal> principal = document->NodePrincipal();
  return TestPermissionFromPrincipal(principal, aType, aPermission);
}

NS_IMETHODIMP
nsPermissionManager::TestPermissionFromPrincipal(nsIPrincipal* aPrincipal,
                                                 const char* aType,
                                                 uint32_t* aPermission)
{
  return CommonTestPermission(aPrincipal, aType, aPermission, false, true);
}

NS_IMETHODIMP
nsPermissionManager::GetPermissionObject(nsIPrincipal* aPrincipal,
                                         const char* aType,
                                         bool aExactHostMatch,
                                         nsIPermission** aResult)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  NS_ENSURE_ARG_POINTER(aType);

  *aResult = nullptr;

  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    return NS_OK;
  }

  
  if (IsExpandedPrincipal(aPrincipal)) {
    return NS_ERROR_INVALID_ARG;
  }

  int32_t typeIndex = GetTypeIndex(aType, false);
  
  
  if (typeIndex == -1) return NS_OK;

  PermissionHashKey* entry = GetPermissionHashKey(aPrincipal, typeIndex, aExactHostMatch);
  if (!entry) {
    return NS_OK;
  }

  
  
  int32_t idx = entry->GetPermissionIndex(typeIndex);
  if (-1 == idx) {
    return NS_OK;
  }

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = GetPrincipalFromOrigin(entry->GetKey()->mOrigin, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  PermissionEntry& perm = entry->GetPermissions()[idx];
  nsCOMPtr<nsIPermission> r = new nsPermission(principal,
                                               mTypeArray.ElementAt(perm.mType),
                                               perm.mPermission,
                                               perm.mExpireType,
                                               perm.mExpireTime);
  r.forget(aResult);
  return NS_OK;
}

nsresult
nsPermissionManager::CommonTestPermission(nsIPrincipal* aPrincipal,
                                          const char *aType,
                                          uint32_t   *aPermission,
                                          bool        aExactHostMatch,
                                          bool        aIncludingSession)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  NS_ENSURE_ARG_POINTER(aType);

  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    *aPermission = nsIPermissionManager::ALLOW_ACTION;
    return NS_OK;
  }

  
  *aPermission = nsIPermissionManager::UNKNOWN_ACTION;

  
  
  nsCOMPtr<nsIExpandedPrincipal> ep = do_QueryInterface(aPrincipal);
  if (ep) {
    nsTArray<nsCOMPtr<nsIPrincipal>>* whitelist;
    nsresult rv = ep->GetWhiteList(&whitelist);
    NS_ENSURE_SUCCESS(rv, rv);

    for (size_t i = 0; i < whitelist->Length(); ++i) {
      uint32_t perm;
      rv = CommonTestPermission(whitelist->ElementAt(i), aType, &perm, aExactHostMatch,
                                aIncludingSession);
      NS_ENSURE_SUCCESS(rv, rv);
      if (perm == nsIPermissionManager::ALLOW_ACTION) {
        *aPermission = perm;
        return NS_OK;
      } else if (perm == nsIPermissionManager::PROMPT_ACTION) {
        
        *aPermission = perm;
      }
    }

    return NS_OK;
  }

  int32_t typeIndex = GetTypeIndex(aType, false);
  
  
  if (typeIndex == -1) return NS_OK;

  PermissionHashKey* entry = GetPermissionHashKey(aPrincipal, typeIndex, aExactHostMatch);
  if (!entry ||
      (!aIncludingSession &&
       entry->GetPermission(typeIndex).mNonSessionExpireType ==
         nsIPermissionManager::EXPIRE_SESSION)) {
    return NS_OK;
  }

  *aPermission = aIncludingSession
                   ? entry->GetPermission(typeIndex).mPermission
                   : entry->GetPermission(typeIndex).mNonSessionPermission;

  return NS_OK;
}







nsPermissionManager::PermissionHashKey*
nsPermissionManager::GetPermissionHashKey(nsIPrincipal* aPrincipal,
                                          uint32_t aType,
                                          bool aExactHostMatch)
{
  PermissionHashKey* entry = nullptr;

  nsRefPtr<PermissionKey> key = new PermissionKey(aPrincipal);
  entry = mPermissionTable.GetEntry(key);

  if (entry) {
    PermissionEntry permEntry = entry->GetPermission(aType);

    
    
    if ((permEntry.mExpireType == nsIPermissionManager::EXPIRE_TIME ||
         (permEntry.mExpireType == nsIPermissionManager::EXPIRE_SESSION &&
          permEntry.mExpireTime != 0)) &&
        permEntry.mExpireTime <= (PR_Now() / 1000)) {
      entry = nullptr;
      RemoveFromPrincipal(aPrincipal, mTypeArray[aType].get());
    } else if (permEntry.mPermission == nsIPermissionManager::UNKNOWN_ACTION) {
      entry = nullptr;
    }
  }

  if (entry) {
    return entry;
  }

  
  if (!aExactHostMatch) {
    nsCOMPtr<nsIURI> uri;
    nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) {
      return nullptr;
    }

    nsAutoCString host;
    rv = uri->GetHost(host);
    if (NS_FAILED(rv)) {
      return nullptr;
    }

    nsCString domain = GetNextSubDomainForHost(host);
    if (domain.IsEmpty()) {
      return nullptr;
    }

    
    nsCOMPtr<nsIURI> newURI;
    rv = uri->Clone(getter_AddRefs(newURI));
    if (NS_FAILED(rv)) {
      return nullptr;
    }

    rv = newURI->SetHost(domain);
    if (NS_FAILED(rv)) {
      return nullptr;
    }

    
    mozilla::OriginAttributes attrs = mozilla::BasePrincipal::Cast(aPrincipal)->OriginAttributesRef();
    nsCOMPtr<nsIPrincipal> principal = mozilla::BasePrincipal::CreateCodebasePrincipal(newURI, attrs);

    return GetPermissionHashKey(principal, aType, aExactHostMatch);
  }

  
  return nullptr;
}

NS_IMETHODIMP nsPermissionManager::GetEnumerator(nsISimpleEnumerator **aEnum)
{
  
  nsCOMArray<nsIPermission> array;

  for (auto iter = mPermissionTable.Iter(); !iter.Done(); iter.Next()) {
    PermissionHashKey* entry = iter.Get();
    for (const auto& permEntry : entry->GetPermissions()) {
      
      
      
      if (permEntry.mPermission == nsIPermissionManager::UNKNOWN_ACTION) {
        continue;
      }

      nsCOMPtr<nsIPrincipal> principal;
      nsresult rv = GetPrincipalFromOrigin(entry->GetKey()->mOrigin,
                                           getter_AddRefs(principal));
      if (NS_FAILED(rv)) {
        continue;
      }

      array.AppendObject(
        new nsPermission(principal,
                         mTypeArray.ElementAt(permEntry.mType),
                         permEntry.mPermission,
                         permEntry.mExpireType,
                         permEntry.mExpireTime));
    }
  }

  return NS_NewArrayEnumerator(aEnum, array);
}

NS_IMETHODIMP nsPermissionManager::Observe(nsISupports *aSubject, const char *aTopic, const char16_t *someData)
{
  if (!nsCRT::strcmp(aTopic, "xpcom-shutdown")) {
    mObserverService = nullptr;
    return NS_OK;
  }

  ENSURE_NOT_CHILD_PROCESS;

  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    
    
    mIsShuttingDown = true;
    RemoveAllFromMemory();
    CloseDB(false);
  } else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    
    InitDB(false);
  }

  return NS_OK;
}

nsresult
nsPermissionManager::RemoveAllModifiedSince(int64_t aModificationTime)
{
  ENSURE_NOT_CHILD_PROCESS;

  nsCOMArray<nsIPermission> array;
  for (auto iter = mPermissionTable.Iter(); !iter.Done(); iter.Next()) {
    PermissionHashKey* entry = iter.Get();
    for (const auto& permEntry : entry->GetPermissions()) {
      if (aModificationTime > permEntry.mModificationTime) {
        continue;
      }

      nsCOMPtr<nsIPrincipal> principal;
      nsresult rv = GetPrincipalFromOrigin(entry->GetKey()->mOrigin,
                                           getter_AddRefs(principal));
      if (NS_FAILED(rv)) {
        continue;
      }

      array.AppendObject(
        new nsPermission(principal,
                         mTypeArray.ElementAt(permEntry.mType),
                         permEntry.mPermission,
                         permEntry.mExpireType,
                         permEntry.mExpireTime));
    }
  }

  for (int32_t i = 0; i<array.Count(); ++i) {
    nsCOMPtr<nsIPrincipal> principal;
    nsAutoCString type;

    nsresult rv = array[i]->GetPrincipal(getter_AddRefs(principal));
    if (NS_FAILED(rv)) {
      NS_ERROR("GetPrincipal() failed!");
      continue;
    }

    rv = array[i]->GetType(type);
    if (NS_FAILED(rv)) {
      NS_ERROR("GetType() failed!");
      continue;
    }

    
    AddInternal(
      principal,
      type,
      nsIPermissionManager::UNKNOWN_ACTION,
      0,
      nsIPermissionManager::EXPIRE_NEVER, 0, 0,
      nsPermissionManager::eNotify,
      nsPermissionManager::eWriteToDB);
  }
  
  
  ImportDefaults();
  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::RemovePermissionsForApp(uint32_t aAppId, bool aBrowserOnly)
{
  ENSURE_NOT_CHILD_PROCESS;
  NS_ENSURE_ARG(aAppId != nsIScriptSecurityManager::NO_APP_ID);

  
  
  
  
  
  
  

  nsAutoCString sql;
  sql.AppendLiteral("DELETE FROM moz_perms WHERE appId=");
  sql.AppendInt(aAppId);

  if (aBrowserOnly) {
    sql.AppendLiteral(" AND isInBrowserElement=1");
  }

  nsCOMPtr<mozIStorageAsyncStatement> removeStmt;
  nsresult rv = mDBConn->CreateAsyncStatement(sql, getter_AddRefs(removeStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> pending;
  rv = removeStmt->ExecuteAsync(nullptr, getter_AddRefs(pending));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsIPermission> permissions;
  for (auto iter = mPermissionTable.Iter(); !iter.Done(); iter.Next()) {
    PermissionHashKey* entry = iter.Get();

    nsCOMPtr<nsIPrincipal> principal;
    nsresult rv = GetPrincipalFromOrigin(entry->GetKey()->mOrigin,
                                         getter_AddRefs(principal));
    if (NS_FAILED(rv)) {
      continue;
    }

    uint32_t appId;
    bool isInBrowserElement;
    principal->GetAppId(&appId);
    principal->GetIsInBrowserElement(&isInBrowserElement);

    if (appId != aAppId || (aBrowserOnly && !isInBrowserElement)) {
      continue;
    }

    for (const auto& permEntry : entry->GetPermissions()) {
      permissions.AppendObject(
        new nsPermission(principal,
                         mTypeArray.ElementAt(permEntry.mType),
                         permEntry.mPermission,
                         permEntry.mExpireType,
                         permEntry.mExpireTime));
    }
  }

  for (int32_t i = 0; i < permissions.Count(); ++i) {
    nsCOMPtr<nsIPrincipal> principal;
    nsAutoCString type;

    permissions[i]->GetPrincipal(getter_AddRefs(principal));
    permissions[i]->GetType(type);

    AddInternal(principal,
                type,
                nsIPermissionManager::UNKNOWN_ACTION,
                0,
                nsIPermissionManager::EXPIRE_NEVER,
                0,
                0,
                nsPermissionManager::eNotify,
                nsPermissionManager::eWriteToDB);
  }

  return NS_OK;
}

nsresult
nsPermissionManager::RemoveExpiredPermissionsForApp(uint32_t aAppId)
{
  ENSURE_NOT_CHILD_PROCESS;

  if (aAppId == nsIScriptSecurityManager::NO_APP_ID) {
    return NS_OK;
  }

  for (auto iter = mPermissionTable.Iter(); !iter.Done(); iter.Next()) {
    PermissionHashKey* entry = iter.Get();
    nsCOMPtr<nsIPrincipal> principal;
    GetPrincipalFromOrigin(entry->GetKey()->mOrigin, getter_AddRefs(principal));

    uint32_t appId;
    principal->GetAppId(&appId);

    if (appId != aAppId) {
      continue;
    }

    for (uint32_t i = 0; i < entry->GetPermissions().Length(); ++i) {
      PermissionEntry& permEntry = entry->GetPermissions()[i];
      if (permEntry.mExpireType != nsIPermissionManager::EXPIRE_SESSION) {
        continue;
      }

      if (permEntry.mNonSessionExpireType ==
            nsIPermissionManager::EXPIRE_SESSION) {
        PermissionEntry oldPermEntry = entry->GetPermissions()[i];

        entry->GetPermissions().RemoveElementAt(i);

        NotifyObserversWithPermission(principal,
                                      mTypeArray.ElementAt(oldPermEntry.mType),
                                      oldPermEntry.mPermission,
                                      oldPermEntry.mExpireType,
                                      oldPermEntry.mExpireTime,
                                      MOZ_UTF16("deleted"));

        --i;
        continue;
      }

      permEntry.mPermission = permEntry.mNonSessionPermission;
      permEntry.mExpireType = permEntry.mNonSessionExpireType;
      permEntry.mExpireTime = permEntry.mNonSessionExpireTime;

      NotifyObserversWithPermission(principal,
                                    mTypeArray.ElementAt(permEntry.mType),
                                    permEntry.mPermission,
                                    permEntry.mExpireType,
                                    permEntry.mExpireTime,
                                    MOZ_UTF16("changed"));
    }
  }

  return NS_OK;
}





nsresult
nsPermissionManager::RemoveAllFromMemory()
{
  mLargestID = 0;
  mTypeArray.Clear();
  mPermissionTable.Clear();

  return NS_OK;
}


int32_t
nsPermissionManager::GetTypeIndex(const char *aType,
                                  bool        aAdd)
{
  for (uint32_t i = 0; i < mTypeArray.Length(); ++i)
    if (mTypeArray[i].Equals(aType))
      return i;

  if (!aAdd) {
    
    return -1;
  }

  
  
  nsCString *elem = mTypeArray.AppendElement();
  if (!elem)
    return -1;

  elem->Assign(aType);
  return mTypeArray.Length() - 1;
}



void
nsPermissionManager::NotifyObserversWithPermission(nsIPrincipal*     aPrincipal,
                                                   const nsCString  &aType,
                                                   uint32_t          aPermission,
                                                   uint32_t          aExpireType,
                                                   int64_t           aExpireTime,
                                                   const char16_t  *aData)
{
  nsCOMPtr<nsIPermission> permission =
    new nsPermission(aPrincipal, aType, aPermission,
                     aExpireType, aExpireTime);
  if (permission)
    NotifyObservers(permission, aData);
}







void
nsPermissionManager::NotifyObservers(nsIPermission   *aPermission,
                                     const char16_t *aData)
{
  if (mObserverService)
    mObserverService->NotifyObservers(aPermission,
                                      kPermissionChangeNotification,
                                      aData);
}

nsresult
nsPermissionManager::Read()
{
  ENSURE_NOT_CHILD_PROCESS;

  nsresult rv;

  
  {
    
    nsCOMPtr<mozIStorageStatement> stmtDeleteExpired;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "DELETE FROM moz_perms WHERE expireType = ?1 AND expireTime <= ?2"),
          getter_AddRefs(stmtDeleteExpired));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmtDeleteExpired->BindInt32ByIndex(0, nsIPermissionManager::EXPIRE_TIME);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmtDeleteExpired->BindInt64ByIndex(1, PR_Now() / 1000);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasResult;
    rv = stmtDeleteExpired->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, origin, type, permission, expireType, expireTime, modificationTime "
    "FROM moz_perms"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t id;
  nsAutoCString origin, type;
  uint32_t permission;
  uint32_t expireType;
  int64_t expireTime;
  int64_t modificationTime;
  bool hasResult;
  bool readError = false;

  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    
    
    id = stmt->AsInt64(0);
    if (id > mLargestID)
      mLargestID = id;

    rv = stmt->GetUTF8String(1, origin);
    if (NS_FAILED(rv)) {
      readError = true;
      continue;
    }

    rv = stmt->GetUTF8String(2, type);
    if (NS_FAILED(rv)) {
      readError = true;
      continue;
    }

    permission = stmt->AsInt32(3);
    expireType = stmt->AsInt32(4);

    
    expireTime = stmt->AsInt64(5);
    modificationTime = stmt->AsInt64(6);

    nsCOMPtr<nsIPrincipal> principal;
    nsresult rv = GetPrincipalFromOrigin(origin, getter_AddRefs(principal));
    if (NS_FAILED(rv)) {
      readError = true;
      continue;
    }

    rv = AddInternal(principal, type, permission, id, expireType, expireTime,
                     modificationTime, eDontNotify, eNoDBOperation);
    if (NS_FAILED(rv)) {
      readError = true;
      continue;
    }
  }

  if (readError) {
    NS_ERROR("Error occured while reading the permissions database!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

static const char kMatchTypeHost[] = "host";
static const char kMatchTypeOrigin[] = "origin";




nsresult
nsPermissionManager::Import()
{
  nsresult rv;

  nsCOMPtr<nsIFile> permissionsFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(permissionsFile));
  if (NS_FAILED(rv)) return rv;

  rv = permissionsFile->AppendNative(NS_LITERAL_CSTRING(HOSTPERM_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream),
                                  permissionsFile);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = _DoImport(fileInputStream, mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  
  permissionsFile->Remove(false);
  return NS_OK;
}



nsresult
nsPermissionManager::ImportDefaults()
{
  nsCString defaultsURL = mozilla::Preferences::GetCString(kDefaultsUrlPrefName);
  if (defaultsURL.IsEmpty()) { 
    return NS_OK;
  }

  nsCOMPtr<nsIURI> defaultsURI;
  nsresult rv = NS_NewURI(getter_AddRefs(defaultsURI), defaultsURL);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     defaultsURI,
                     nsContentUtils::GetSystemPrincipal(),
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_OTHER);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> inputStream;
  rv = channel->Open(getter_AddRefs(inputStream));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = _DoImport(inputStream, nullptr);
  inputStream->Close();
  return rv;
}





nsresult
nsPermissionManager::_DoImport(nsIInputStream *inputStream, mozIStorageConnection *conn)
{
  ENSURE_NOT_CHILD_PROCESS;

  nsresult rv;
  
  
  
  mozStorageTransaction transaction(conn, true);

  
  DBOperationType operation = conn ? eWriteToDB : eNoDBOperation;
  
  
  int64_t id = conn ? 0 : cIDPermissionIsDefault;

  






  
  
  
  nsLineBuffer<char> lineBuffer;
  nsCString line;
  bool isMore = true;
  do {
    rv = NS_ReadLine(inputStream, &lineBuffer, line, &isMore);
    NS_ENSURE_SUCCESS(rv, rv);

    if (line.IsEmpty() || line.First() == '#') {
      continue;
    }

    nsTArray<nsCString> lineArray;

    
    ParseString(line, '\t', lineArray);

    if (lineArray[0].EqualsLiteral(kMatchTypeHost) &&
        lineArray.Length() == 4) {
      nsresult error = NS_OK;
      uint32_t permission = lineArray[2].ToInteger(&error);
      if (NS_FAILED(error))
        continue;

      
      
      int64_t modificationTime = 0;

      UpgradeHostToOriginHostfileImport upHelper(this, operation, id);
      error = UpgradeHostToOriginAndInsert(lineArray[3], lineArray[1], permission,
                                           nsIPermissionManager::EXPIRE_NEVER, 0,
                                           modificationTime, nsIScriptSecurityManager::NO_APP_ID,
                                           false, &upHelper);
      if (NS_FAILED(error)) {
        NS_WARNING("There was a problem importing a host permission");
      }
    } else if (lineArray[0].EqualsLiteral(kMatchTypeOrigin) &&
               lineArray.Length() == 4) {
      nsresult error = NS_OK;
      uint32_t permission = lineArray[2].ToInteger(&error);
      if (NS_FAILED(error))
        continue;

      nsCOMPtr<nsIPrincipal> principal;
      error = GetPrincipalFromOrigin(lineArray[3], getter_AddRefs(principal));
      if (NS_FAILED(error)) {
        NS_WARNING("Couldn't import an origin permission - malformed origin");
        continue;
      }

      
      
      int64_t modificationTime = 0;

      error = AddInternal(principal, lineArray[1], permission, id,
                          nsIPermissionManager::EXPIRE_NEVER, 0,
                          modificationTime,
                          eDontNotify, operation);
      if (NS_FAILED(error)) {
        NS_WARNING("There was a problem importing an origin permission");
      }
    }

  } while (isMore);

  return NS_OK;
}

nsresult
nsPermissionManager::NormalizeToACE(nsCString &aHost)
{
  
  if (!mIDNService) {
    nsresult rv;
    mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return mIDNService->ConvertUTF8toACE(aHost, aHost);
}

void
nsPermissionManager::UpdateDB(OperationType aOp,
                              mozIStorageAsyncStatement* aStmt,
                              int64_t aID,
                              const nsACString &aOrigin,
                              const nsACString &aType,
                              uint32_t aPermission,
                              uint32_t aExpireType,
                              int64_t aExpireTime,
                              int64_t aModificationTime)
{
  ENSURE_NOT_CHILD_PROCESS_NORET;

  nsresult rv;

  
  if (!aStmt)
    return;

  switch (aOp) {
  case eOperationAdding:
    {
      rv = aStmt->BindInt64ByIndex(0, aID);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindUTF8StringByIndex(1, aOrigin);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindUTF8StringByIndex(2, aType);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt32ByIndex(3, aPermission);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt32ByIndex(4, aExpireType);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt64ByIndex(5, aExpireTime);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt64ByIndex(6, aModificationTime);
      break;
    }

  case eOperationRemoving:
    {
      rv = aStmt->BindInt64ByIndex(0, aID);
      break;
    }

  case eOperationChanging:
    {
      rv = aStmt->BindInt64ByIndex(0, aID);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt32ByIndex(1, aPermission);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt32ByIndex(2, aExpireType);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt64ByIndex(3, aExpireTime);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt64ByIndex(4, aModificationTime);
      break;
    }

  default:
    {
      NS_NOTREACHED("need a valid operation in UpdateDB()!");
      rv = NS_ERROR_UNEXPECTED;
      break;
    }
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("db change failed!");
    return;
  }

  nsCOMPtr<mozIStoragePendingStatement> pending;
  rv = aStmt->ExecuteAsync(nullptr, getter_AddRefs(pending));
  MOZ_ASSERT(NS_SUCCEEDED(rv));
}

NS_IMETHODIMP
nsPermissionManager::AddrefAppId(uint32_t aAppId)
{
  if (aAppId == nsIScriptSecurityManager::NO_APP_ID) {
    return NS_OK;
  }

  bool found = false;
  for (uint32_t i = 0; i < mAppIdRefcounts.Length(); ++i) {
    if (mAppIdRefcounts[i].mAppId == aAppId) {
      ++mAppIdRefcounts[i].mCounter;
      found = true;
      break;
    }
  }

  if (!found) {
    ApplicationCounter app = { aAppId, 1 };
    mAppIdRefcounts.AppendElement(app);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::ReleaseAppId(uint32_t aAppId)
{
  

  if (aAppId == nsIScriptSecurityManager::NO_APP_ID) {
    return NS_OK;
  }

  for (uint32_t i = 0; i < mAppIdRefcounts.Length(); ++i) {
    if (mAppIdRefcounts[i].mAppId == aAppId) {
      --mAppIdRefcounts[i].mCounter;

      if (!mAppIdRefcounts[i].mCounter) {
        mAppIdRefcounts.RemoveElementAt(i);
        return RemoveExpiredPermissionsForApp(aAppId);
      }

      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::UpdateExpireTime(nsIPrincipal* aPrincipal,
                                     const char* aType,
                                     bool aExactHostMatch,
                                     uint64_t aSessionExpireTime,
                                     uint64_t aPersistentExpireTime)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  NS_ENSURE_ARG_POINTER(aType);

  uint64_t nowms = PR_Now() / 1000;
  if (aSessionExpireTime < nowms || aPersistentExpireTime < nowms) {
    return NS_ERROR_INVALID_ARG;
  }

  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    return NS_OK;
  }

  
  if (IsExpandedPrincipal(aPrincipal)) {
    return NS_ERROR_INVALID_ARG;
  }

  int32_t typeIndex = GetTypeIndex(aType, false);
  
  
  if (typeIndex == -1) return NS_OK;

  PermissionHashKey* entry = GetPermissionHashKey(aPrincipal, typeIndex, aExactHostMatch);
  if (!entry) {
    return NS_OK;
  }

  int32_t idx = entry->GetPermissionIndex(typeIndex);
  if (-1 == idx) {
    return NS_OK;
  }

  PermissionEntry& perm = entry->GetPermissions()[idx];
  if (perm.mExpireType == EXPIRE_TIME) {
    perm.mExpireTime = aPersistentExpireTime;
  } else if (perm.mExpireType == EXPIRE_SESSION && perm.mExpireTime != 0) {
    perm.mExpireTime = aSessionExpireTime;
  }
  return NS_OK;
}

nsresult
nsPermissionManager::FetchPermissions() {
  MOZ_ASSERT(IsChildProcess(), "FetchPermissions can only be invoked in child process");
  
  InfallibleTArray<IPC::Permission> perms;
  ChildProcess()->SendReadPermissions(&perms);

  for (uint32_t i = 0; i < perms.Length(); i++) {
    const IPC::Permission &perm = perms[i];

    nsCOMPtr<nsIPrincipal> principal;
    nsresult rv = GetPrincipalFromOrigin(perm.origin, getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    uint64_t modificationTime = 0;
    AddInternal(principal, perm.type, perm.capability, 0, perm.expireType,
                perm.expireTime, modificationTime, eNotify, eNoDBOperation,
                true );
  }
  return NS_OK;
}
