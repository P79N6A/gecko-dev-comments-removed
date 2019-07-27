




#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"
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
#include "nsIEffectiveTLDService.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"
#include "mozilla/net/NeckoMessageUtils.h"
#include "mozilla/Preferences.h"
#include "nsReadLine.h"

static nsPermissionManager *gPermissionManager = nullptr;

using mozilla::dom::ContentParent;
using mozilla::dom::ContentChild;
using mozilla::unused; 

static bool
IsChildProcess()
{
  return XRE_GetProcessType() == GeckoProcessType_Content;
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
GetPrincipal(const nsACString& aHost, uint32_t aAppId, bool aIsInBrowserElement,
             nsIPrincipal** aPrincipal)
{
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHost);
  if (NS_FAILED(rv)) {
    
    
    
    
    nsCString scheme;
    if (aHost.FindChar('@') == -1)
      scheme = NS_LITERAL_CSTRING("http://");
    else
      scheme = NS_LITERAL_CSTRING("mailto:");
    rv = NS_NewURI(getter_AddRefs(uri), scheme + aHost);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return secMan->GetAppCodebasePrincipal(uri, aAppId, aIsInBrowserElement, aPrincipal);
}

nsresult
GetPrincipal(nsIURI* aURI, nsIPrincipal** aPrincipal)
{
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

  return secMan->GetNoAppCodebasePrincipal(aURI, aPrincipal);
}

nsresult
GetPrincipal(const nsACString& aHost, nsIPrincipal** aPrincipal)
{
  return GetPrincipal(aHost, nsIScriptSecurityManager::NO_APP_ID, false, aPrincipal);
}

nsresult
GetHostForPrincipal(nsIPrincipal* aPrincipal, nsACString& aHost)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  uri = NS_GetInnermostURI(uri);
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

  rv = uri->GetAsciiHost(aHost);
  if (NS_SUCCEEDED(rv) && !aHost.IsEmpty()) {
    return NS_OK;
  }

  
  
  bool isMailTo = false;
  if (NS_SUCCEEDED(uri->SchemeIs("mailto", &isMailTo)) && isMailTo) {
    rv = uri->GetPath(aHost);
    NS_ENSURE_SUCCESS(rv, rv);

    int32_t spart = aHost.FindChar('?', 0);
    if (spart >= 0) {
      aHost.Cut(spart, aHost.Length() - spart);
    }
    return NS_OK;
  }

  
  rv = aPrincipal->GetOrigin(getter_Copies(aHost));
  if (NS_SUCCEEDED(rv) && !aHost.IsEmpty()) {
    return NS_OK;
  }

  return NS_ERROR_UNEXPECTED;
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

class AppClearDataObserver MOZ_FINAL : public nsIObserver {
  ~AppClearDataObserver() {}

public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHODIMP
  Observe(nsISupports *aSubject, const char *aTopic, const char16_t *data) MOZ_OVERRIDE
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

static bool
IsExpandedPrincipal(nsIPrincipal* aPrincipal)
{
  nsCOMPtr<nsIExpandedPrincipal> ep = do_QueryInterface(aPrincipal);
  return !!ep;
}

} 



nsPermissionManager::PermissionKey::PermissionKey(nsIPrincipal* aPrincipal)
{
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(GetHostForPrincipal(aPrincipal, mHost)));
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aPrincipal->GetAppId(&mAppId)));
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aPrincipal->GetIsInBrowserElement(&mIsInBrowserElement)));
}











class CloseDatabaseListener MOZ_FINAL : public mozIStorageCompletionCallback
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












class DeleteFromMozHostListener MOZ_FINAL : public mozIStorageStatementCallback
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




static const char kPermissionsFileName[] = "permissions.sqlite";
#define HOSTS_SCHEMA_VERSION 4

static const char kHostpermFileName[] = "hostperm.1";



static const char kDefaultsUrlPrefName[] = "permissions.manager.defaultsUrl";

static const char kPermissionChangeNotification[] = PERM_CHANGE_NOTIFICATION;

NS_IMPL_ISUPPORTS(nsPermissionManager, nsIPermissionManager, nsIObserver, nsISupportsWeakReference)

nsPermissionManager::nsPermissionManager()
 : mLargestID(0)
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

  mObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    mObserverService->AddObserver(this, "profile-before-change", true);
    mObserverService->AddObserver(this, "profile-do-change", true);
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
nsPermissionManager::InitDB(bool aRemoveFile)
{
  nsCOMPtr<nsIFile> permissionsFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_PERMISSION_PARENT_DIR, getter_AddRefs(permissionsFile));
  if (NS_FAILED(rv)) {
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(permissionsFile));
  }
  NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

  rv = permissionsFile->AppendNative(NS_LITERAL_CSTRING(kPermissionsFileName));
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

  nsCOMPtr<mozIStorageService> storage = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  if (!storage)
    return NS_ERROR_UNEXPECTED;

  
  rv = storage->OpenDatabase(permissionsFile, getter_AddRefs(mDBConn));
  NS_ENSURE_SUCCESS(rv, rv);

  bool ready;
  mDBConn->GetConnectionReady(&ready);
  if (!ready) {
    
    rv = permissionsFile->Remove(false);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = storage->OpenDatabase(permissionsFile, getter_AddRefs(mDBConn));
    NS_ENSURE_SUCCESS(rv, rv);

    mDBConn->GetConnectionReady(&ready);
    if (!ready)
      return NS_ERROR_UNEXPECTED;
  }

  bool tableExists = false;
  mDBConn->TableExists(NS_LITERAL_CSTRING("moz_hosts"), &tableExists);
  if (!tableExists) {
    rv = CreateTable();
    NS_ENSURE_SUCCESS(rv, rv);

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

      

    
    case HOSTS_SCHEMA_VERSION:
      break;

    
    
    
    
    
    
    default:
      {
        
        nsCOMPtr<mozIStorageStatement> stmt;
        rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT host, type, permission, expireType, expireTime, modificationTime, appId, isInBrowserElement FROM moz_hosts"),
          getter_AddRefs(stmt));
        if (NS_SUCCEEDED(rv))
          break;

        
        rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE moz_hosts"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = CreateTable();
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
    }
  }

  
  mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA synchronous = OFF"));

  
  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_hosts "
    "(id, host, type, permission, expireType, expireTime, modificationTime, appId, isInBrowserElement) "
    "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)"), getter_AddRefs(mStmtInsert));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_hosts "
    "WHERE id = ?1"), getter_AddRefs(mStmtDelete));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_hosts "
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
    "CREATE TABLE moz_hosts ("
      " id INTEGER PRIMARY KEY"
      ",host TEXT"
      ",type TEXT"
      ",permission INTEGER"
      ",expireType INTEGER"
      ",expireTime INTEGER"
      ",modificationTime INTEGER"
      ",appId INTEGER"
      ",isInBrowserElement INTEGER"
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
  nsAutoCString host;
  nsresult rv = GetHostForPrincipal(aPrincipal, host);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!IsChildProcess()) {
    uint32_t appId;
    rv = aPrincipal->GetAppId(&appId);
    NS_ENSURE_SUCCESS(rv, rv);

    bool isInBrowserElement;
    rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
    NS_ENSURE_SUCCESS(rv, rv);

    IPC::Permission permission(host, appId, isInBrowserElement, aType,
                               aPermission, aExpireType, aExpireTime);

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
        uint32_t appId;
        rv = aPrincipal->GetAppId(&appId);
        NS_ENSURE_SUCCESS(rv, rv);

        bool isInBrowserElement;
        rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
        NS_ENSURE_SUCCESS(rv, rv);

        UpdateDB(op, mStmtInsert, id, host, aType, aPermission, aExpireType, aExpireTime, aModificationTime, appId, isInBrowserElement);
      }

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(host,
                                      entry->GetKey()->mAppId,
                                      entry->GetKey()->mIsInBrowserElement,
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
                 nsIPermissionManager::EXPIRE_NEVER, 0, 0, 0, false);

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(host,
                                      entry->GetKey()->mAppId,
                                      entry->GetKey()->mIsInBrowserElement,
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
                 aPermission, aExpireType, aExpireTime, aModificationTime, 0, false);

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(host,
                                      entry->GetKey()->mAppId,
                                      entry->GetKey()->mIsInBrowserElement,
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

        UpdateDB(eOperationAdding, mStmtInsert, id, host, aType, aPermission,
                 aExpireType, aExpireTime, aModificationTime, appId, isInBrowserElement);
      }

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(host,
                                      entry->GetKey()->mAppId,
                                      entry->GetKey()->mIsInBrowserElement,
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
nsPermissionManager::Remove(const nsACString &aHost,
                            const char       *aType)
{
  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = GetPrincipal(aHost, getter_AddRefs(principal));
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
         "DELETE FROM moz_hosts"
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

  nsAutoCString host;
  nsresult rv = GetHostForPrincipal(aPrincipal, host);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t typeIndex = GetTypeIndex(aType, false);
  
  
  if (typeIndex == -1) return NS_OK;

  uint32_t appId;
  rv = aPrincipal->GetAppId(&appId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isInBrowserElement;
  rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
  NS_ENSURE_SUCCESS(rv, rv);

  PermissionHashKey* entry = GetPermissionHashKey(host, appId, isInBrowserElement,
                                                  typeIndex, aExactHostMatch);
  if (!entry) {
    return NS_OK;
  }

  
  
  int32_t idx = entry->GetPermissionIndex(typeIndex);
  if (-1 == idx) {
    return NS_OK;
  }

  PermissionEntry& perm = entry->GetPermissions()[idx];
  nsCOMPtr<nsIPermission> r = new nsPermission(entry->GetKey()->mHost,
                                               entry->GetKey()->mAppId,
                                               entry->GetKey()->mIsInBrowserElement,
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

  nsAutoCString host;
  nsresult rv = GetHostForPrincipal(aPrincipal, host);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t typeIndex = GetTypeIndex(aType, false);
  
  
  if (typeIndex == -1) return NS_OK;

  uint32_t appId;
  rv = aPrincipal->GetAppId(&appId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isInBrowserElement;
  rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
  NS_ENSURE_SUCCESS(rv, rv);

  PermissionHashKey* entry = GetPermissionHashKey(host, appId, isInBrowserElement,
                                                  typeIndex, aExactHostMatch);
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
nsPermissionManager::GetPermissionHashKey(const nsACString& aHost,
                                          uint32_t aAppId,
                                          bool aIsInBrowserElement,
                                          uint32_t aType,
                                          bool aExactHostMatch)
{
  PermissionHashKey* entry = nullptr;

  nsRefPtr<PermissionKey> key = new PermissionKey(aHost, aAppId, aIsInBrowserElement);
  entry = mPermissionTable.GetEntry(key);

  if (entry) {
    PermissionEntry permEntry = entry->GetPermission(aType);

    
    
    if ((permEntry.mExpireType == nsIPermissionManager::EXPIRE_TIME ||
         (permEntry.mExpireType == nsIPermissionManager::EXPIRE_SESSION &&
          permEntry.mExpireTime != 0)) &&
        permEntry.mExpireTime <= (PR_Now() / 1000)) {
      nsCOMPtr<nsIPrincipal> principal;
      if (NS_FAILED(GetPrincipal(aHost, aAppId, aIsInBrowserElement, getter_AddRefs(principal)))) {
        return nullptr;
      }

      entry = nullptr;
      RemoveFromPrincipal(principal, mTypeArray[aType].get());
    } else if (permEntry.mPermission == nsIPermissionManager::UNKNOWN_ACTION) {
      entry = nullptr;
    }
  }

  if (entry) {
    return entry;
  }

  
  
  
  
  
  
  
  

  if (StringBeginsWith(aHost, NS_LITERAL_CSTRING("file://"))) {
    return GetPermissionHashKey(NS_LITERAL_CSTRING("<file>"), aAppId, aIsInBrowserElement, aType, true);
  }

  if (!aExactHostMatch) {
    nsCString domain = GetNextSubDomainForHost(aHost);
    if (!domain.IsEmpty()) {
      return GetPermissionHashKey(domain, aAppId, aIsInBrowserElement, aType, aExactHostMatch);
    }
  }

  
  return nullptr;
}


struct nsGetEnumeratorData
{
  nsGetEnumeratorData(nsCOMArray<nsIPermission> *aArray,
                      const nsTArray<nsCString> *aTypes,
                      int64_t aSince = 0)
   : array(aArray)
   , types(aTypes)
   , since(aSince) {}

  nsCOMArray<nsIPermission> *array;
  const nsTArray<nsCString> *types;
  int64_t since;
};

static PLDHashOperator
AddPermissionsToList(nsPermissionManager::PermissionHashKey* entry, void *arg)
{
  nsGetEnumeratorData *data = static_cast<nsGetEnumeratorData *>(arg);

  for (uint32_t i = 0; i < entry->GetPermissions().Length(); ++i) {
    nsPermissionManager::PermissionEntry& permEntry = entry->GetPermissions()[i];

    
    
    
    if (permEntry.mPermission == nsIPermissionManager::UNKNOWN_ACTION) {
      continue;
    }

    nsPermission *perm = new nsPermission(entry->GetKey()->mHost,
                                          entry->GetKey()->mAppId,
                                          entry->GetKey()->mIsInBrowserElement,
                                          data->types->ElementAt(permEntry.mType),
                                          permEntry.mPermission,
                                          permEntry.mExpireType,
                                          permEntry.mExpireTime);

    data->array->AppendObject(perm);
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP nsPermissionManager::GetEnumerator(nsISimpleEnumerator **aEnum)
{
  
  nsCOMArray<nsIPermission> array;
  nsGetEnumeratorData data(&array, &mTypeArray);

  mPermissionTable.EnumerateEntries(AddPermissionsToList, &data);

  return NS_NewArrayEnumerator(aEnum, array);
}

NS_IMETHODIMP nsPermissionManager::Observe(nsISupports *aSubject, const char *aTopic, const char16_t *someData)
{
  ENSURE_NOT_CHILD_PROCESS;

  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    
    
    mIsShuttingDown = true;
    RemoveAllFromMemory();
    CloseDB(false);
  }
  else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    
    InitDB(false);
  }

  return NS_OK;
}

static PLDHashOperator
AddPermissionsModifiedSinceToList(
  nsPermissionManager::PermissionHashKey* entry, void* arg)
{
  nsGetEnumeratorData* data = static_cast<nsGetEnumeratorData *>(arg);

  for (size_t i = 0; i < entry->GetPermissions().Length(); ++i) {
    const nsPermissionManager::PermissionEntry& permEntry = entry->GetPermissions()[i];

    if (data->since > permEntry.mModificationTime) {
      continue;
    }

    nsPermission* perm = new nsPermission(entry->GetKey()->mHost,
                                          entry->GetKey()->mAppId,
                                          entry->GetKey()->mIsInBrowserElement,
                                          data->types->ElementAt(permEntry.mType),
                                          permEntry.mPermission,
                                          permEntry.mExpireType,
                                          permEntry.mExpireTime);

    data->array->AppendObject(perm);
  }
  return PL_DHASH_NEXT;
}

nsresult
nsPermissionManager::RemoveAllModifiedSince(int64_t aModificationTime)
{
  ENSURE_NOT_CHILD_PROCESS;

  
  nsCOMArray<nsIPermission> array;
  nsGetEnumeratorData data(&array, &mTypeArray, aModificationTime);

  mPermissionTable.EnumerateEntries(AddPermissionsModifiedSinceToList, &data);

  for (int32_t i = 0; i<array.Count(); ++i) {
    nsAutoCString host;
    bool isInBrowserElement = false;
    nsAutoCString type;
    uint32_t appId = 0;

    array[i]->GetHost(host);
    array[i]->GetIsInBrowserElement(&isInBrowserElement);
    array[i]->GetType(type);
    array[i]->GetAppId(&appId);

    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipal(host, appId, isInBrowserElement,
                               getter_AddRefs(principal)))) {
      NS_ERROR("GetPrincipal() failed!");
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

PLDHashOperator
nsPermissionManager::GetPermissionsForApp(nsPermissionManager::PermissionHashKey* entry, void* arg)
{
  GetPermissionsForAppStruct* data = static_cast<GetPermissionsForAppStruct*>(arg);

  for (uint32_t i = 0; i < entry->GetPermissions().Length(); ++i) {
    nsPermissionManager::PermissionEntry& permEntry = entry->GetPermissions()[i];

    if (entry->GetKey()->mAppId != data->appId ||
        (data->browserOnly && !entry->GetKey()->mIsInBrowserElement)) {
      continue;
    }

    data->permissions.AppendObject(new nsPermission(entry->GetKey()->mHost,
                                                    entry->GetKey()->mAppId,
                                                    entry->GetKey()->mIsInBrowserElement,
                                                    gPermissionManager->mTypeArray.ElementAt(permEntry.mType),
                                                    permEntry.mPermission,
                                                    permEntry.mExpireType,
                                                    permEntry.mExpireTime));
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsPermissionManager::RemovePermissionsForApp(uint32_t aAppId, bool aBrowserOnly)
{
  ENSURE_NOT_CHILD_PROCESS;
  NS_ENSURE_ARG(aAppId != nsIScriptSecurityManager::NO_APP_ID);

  
  
  
  
  
  
  

  nsAutoCString sql;
  sql.AppendLiteral("DELETE FROM moz_hosts WHERE appId=");
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

  GetPermissionsForAppStruct data(aAppId, aBrowserOnly);
  mPermissionTable.EnumerateEntries(GetPermissionsForApp, &data);

  for (int32_t i=0; i<data.permissions.Count(); ++i) {
    nsAutoCString host;
    bool isInBrowserElement;
    nsAutoCString type;

    data.permissions[i]->GetHost(host);
    data.permissions[i]->GetIsInBrowserElement(&isInBrowserElement);
    data.permissions[i]->GetType(type);

    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(GetPrincipal(host, aAppId, isInBrowserElement,
                               getter_AddRefs(principal)))) {
      NS_ERROR("GetPrincipal() failed!");
      continue;
    }

    AddInternal(principal,
                type,
                nsIPermissionManager::UNKNOWN_ACTION,
                0,
                nsIPermissionManager::EXPIRE_NEVER,
                0,
                0,
                nsPermissionManager::eNotify,
                nsPermissionManager::eNoDBOperation);
  }

  return NS_OK;
}

PLDHashOperator
nsPermissionManager::RemoveExpiredPermissionsForAppEnumerator(
  nsPermissionManager::PermissionHashKey* entry, void* arg)
{
  uint32_t* appId = static_cast<uint32_t*>(arg);

  for (uint32_t i = 0; i < entry->GetPermissions().Length(); ++i) {
    if (entry->GetKey()->mAppId != *appId) {
      continue;
    }

    nsPermissionManager::PermissionEntry& permEntry = entry->GetPermissions()[i];
    if (permEntry.mExpireType != nsIPermissionManager::EXPIRE_SESSION) {
      continue;
    }

    if (permEntry.mNonSessionExpireType == nsIPermissionManager::EXPIRE_SESSION) {
      PermissionEntry oldPermissionEntry = entry->GetPermissions()[i];

      entry->GetPermissions().RemoveElementAt(i);

      gPermissionManager->NotifyObserversWithPermission(entry->GetKey()->mHost,
                                                        entry->GetKey()->mAppId,
                                                        entry->GetKey()->mIsInBrowserElement,
                                                        gPermissionManager->mTypeArray.ElementAt(oldPermissionEntry.mType),
                                                        oldPermissionEntry.mPermission,
                                                        oldPermissionEntry.mExpireType,
                                                        oldPermissionEntry.mExpireTime,
                                                        MOZ_UTF16("deleted"));
      --i;
      continue;
    }

    permEntry.mPermission = permEntry.mNonSessionPermission;
    permEntry.mExpireType = permEntry.mNonSessionExpireType;
    permEntry.mExpireTime = permEntry.mNonSessionExpireTime;

    gPermissionManager->NotifyObserversWithPermission(entry->GetKey()->mHost,
                                                      entry->GetKey()->mAppId,
                                                      entry->GetKey()->mIsInBrowserElement,
                                                      gPermissionManager->mTypeArray.ElementAt(permEntry.mType),
                                                      permEntry.mPermission,
                                                      permEntry.mExpireType,
                                                      permEntry.mExpireTime,
                                                      MOZ_UTF16("changed"));
  }

  return PL_DHASH_NEXT;
}

nsresult
nsPermissionManager::RemoveExpiredPermissionsForApp(uint32_t aAppId)
{
  ENSURE_NOT_CHILD_PROCESS;

  if (aAppId != nsIScriptSecurityManager::NO_APP_ID) {
    mPermissionTable.EnumerateEntries(RemoveExpiredPermissionsForAppEnumerator, &aAppId);
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
nsPermissionManager::NotifyObserversWithPermission(const nsACString &aHost,
                                                   uint32_t          aAppId,
                                                   bool              aIsInBrowserElement,
                                                   const nsCString  &aType,
                                                   uint32_t          aPermission,
                                                   uint32_t          aExpireType,
                                                   int64_t           aExpireTime,
                                                   const char16_t  *aData)
{
  nsCOMPtr<nsIPermission> permission =
    new nsPermission(aHost, aAppId, aIsInBrowserElement, aType, aPermission,
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
          "DELETE FROM moz_hosts WHERE expireType = ?1 AND expireTime <= ?2"),
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
    "SELECT id, host, type, permission, expireType, expireTime, modificationTime, appId, isInBrowserElement "
    "FROM moz_hosts"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t id;
  nsAutoCString host, type;
  uint32_t permission;
  uint32_t expireType;
  int64_t expireTime;
  int64_t modificationTime;
  uint32_t appId;
  bool isInBrowserElement;
  bool hasResult;
  bool readError = false;

  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    
    
    id = stmt->AsInt64(0);
    if (id > mLargestID)
      mLargestID = id;

    rv = stmt->GetUTF8String(1, host);
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

    if (stmt->AsInt64(7) < 0) {
      readError = true;
      continue;
    }

    appId = static_cast<uint32_t>(stmt->AsInt64(7));
    isInBrowserElement = static_cast<bool>(stmt->AsInt32(8));

    nsCOMPtr<nsIPrincipal> principal;
    nsresult rv = GetPrincipal(host, appId, isInBrowserElement, getter_AddRefs(principal));
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




nsresult
nsPermissionManager::Import()
{
  nsresult rv;

  nsCOMPtr<nsIFile> permissionsFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(permissionsFile));
  if (NS_FAILED(rv)) return rv;

  rv = permissionsFile->AppendNative(NS_LITERAL_CSTRING(kHostpermFileName));
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

      nsresult error;
      uint32_t permission = lineArray[2].ToInteger(&error);
      if (NS_FAILED(error))
        continue;

      
      if (!IsASCII(lineArray[3])) {
        rv = NormalizeToACE(lineArray[3]);
        if (NS_FAILED(rv))
          continue;
      }

      nsCOMPtr<nsIPrincipal> principal;
      nsresult rv = GetPrincipal(lineArray[3], getter_AddRefs(principal));
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      int64_t modificationTime = 0;

      rv = AddInternal(principal, lineArray[1], permission, id,
                       nsIPermissionManager::EXPIRE_NEVER, 0,
                       modificationTime,
                       eDontNotify, operation);
      NS_ENSURE_SUCCESS(rv, rv);
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
                              const nsACString &aHost,
                              const nsACString &aType,
                              uint32_t aPermission,
                              uint32_t aExpireType,
                              int64_t aExpireTime,
                              int64_t aModificationTime,
                              uint32_t aAppId,
                              bool aIsInBrowserElement)
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

      rv = aStmt->BindUTF8StringByIndex(1, aHost);
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
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt64ByIndex(7, aAppId);
      if (NS_FAILED(rv)) break;

      rv = aStmt->BindInt64ByIndex(8, aIsInBrowserElement);
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

  nsAutoCString host;
  nsresult rv = GetHostForPrincipal(aPrincipal, host);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t typeIndex = GetTypeIndex(aType, false);
  
  
  if (typeIndex == -1) return NS_OK;

  uint32_t appId;
  rv = aPrincipal->GetAppId(&appId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isInBrowserElement;
  rv = aPrincipal->GetIsInBrowserElement(&isInBrowserElement);
  NS_ENSURE_SUCCESS(rv, rv);

  PermissionHashKey* entry = GetPermissionHashKey(host, appId, isInBrowserElement,
                                                  typeIndex, aExactHostMatch);
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
    nsresult rv = GetPrincipal(perm.host, perm.appId,
                               perm.isInBrowserElement, getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    uint64_t modificationTime = 0;
    AddInternal(principal, perm.type, perm.capability, 0, perm.expireType,
                perm.expireTime, modificationTime, eNotify, eNoDBOperation,
                true );
  }
  return NS_OK;
}
