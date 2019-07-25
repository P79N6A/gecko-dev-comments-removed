




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
#include "prprf.h"
#include "mozilla/storage.h"
#include "nsXULAppAPI.h"
#include "nsIPrincipal.h"

static nsPermissionManager *gPermissionManager = nsnull;

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
      NS_RUNTIMEABORT("Content Process is NULL!");
    return cpc;
  }

  return nsnull;
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
  ENSURE_NOT_CHILD_PROCESS_()



#define PL_ARENA_CONST_ALIGN_MASK 3
#include "plarena.h"

static PLArenaPool *gHostArena = nsnull;



#define HOST_ARENA_SIZE 512



static char *
ArenaStrDup(const char* str, PLArenaPool* aArena)
{
  void* mem;
  const PRUint32 size = strlen(str) + 1;
  PL_ARENA_ALLOCATE(mem, aArena, size);
  if (mem)
    memcpy(mem, str, size);
  return static_cast<char*>(mem);
}

nsHostEntry::nsHostEntry(const char* aHost)
{
  mHost = ArenaStrDup(aHost, gHostArena);
}


nsHostEntry::nsHostEntry(const nsHostEntry& toCopy)
 : mHost(toCopy.mHost)
 , mPermissions(toCopy.mPermissions)
{
}














class CloseDatabaseListener : public mozIStorageCompletionCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGECOMPLETIONCALLBACK
  




  CloseDatabaseListener(nsPermissionManager* aManager,
                        bool aRebuildOnSuccess);

protected:
  nsRefPtr<nsPermissionManager> mManager;
  bool mRebuildOnSuccess;
};

NS_IMPL_ISUPPORTS1(CloseDatabaseListener, mozIStorageCompletionCallback)

CloseDatabaseListener::CloseDatabaseListener(nsPermissionManager* aManager,
                                             bool aRebuildOnSuccess)
  : mManager(aManager)
  , mRebuildOnSuccess(aRebuildOnSuccess)
{
}

NS_IMETHODIMP
CloseDatabaseListener::Complete()
{
  
  nsRefPtr<nsPermissionManager> manager = mManager.forget();
  if (mRebuildOnSuccess && !manager->mIsShuttingDown) {
    return manager->InitDB(true);
  }
  return NS_OK;
}












class DeleteFromMozHostListener : public mozIStorageStatementCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK

  


  DeleteFromMozHostListener(nsPermissionManager* aManager);

protected:
  nsRefPtr<nsPermissionManager> mManager;
};

NS_IMPL_ISUPPORTS1(DeleteFromMozHostListener, mozIStorageStatementCallback)

DeleteFromMozHostListener::
DeleteFromMozHostListener(nsPermissionManager* aManager)
  : mManager(aManager)
{
}

NS_IMETHODIMP DeleteFromMozHostListener::HandleResult(mozIStorageResultSet *)
{
  MOZ_NOT_REACHED("Should not get any results");
  return NS_OK;
}

NS_IMETHODIMP DeleteFromMozHostListener::HandleError(mozIStorageError *)
{
  
  return NS_OK;
}

NS_IMETHODIMP DeleteFromMozHostListener::HandleCompletion(PRUint16 aReason)
{
  
  nsRefPtr<nsPermissionManager> manager = mManager.forget();

  if (aReason == REASON_ERROR) {
    manager->CloseDB(true);
  }

  return NS_OK;
}




static const char kPermissionsFileName[] = "permissions.sqlite";
#define HOSTS_SCHEMA_VERSION 2

static const char kHostpermFileName[] = "hostperm.1";

static const char kPermissionChangeNotification[] = PERM_CHANGE_NOTIFICATION;

NS_IMPL_ISUPPORTS3(nsPermissionManager, nsIPermissionManager, nsIObserver, nsISupportsWeakReference)

nsPermissionManager::nsPermissionManager()
 : mLargestID(0)
 , mIsShuttingDown(false)
{
}

nsPermissionManager::~nsPermissionManager()
{
  RemoveAllFromMemory();
  gPermissionManager = nsnull;
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

  mHostTable.Init();

  mObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    mObserverService->AddObserver(this, "profile-before-change", true);
    mObserverService->AddObserver(this, "profile-do-change", true);
  }

  if (IsChildProcess()) {
    
    InfallibleTArray<IPC::Permission> perms;
    ChildProcess()->SendReadPermissions(&perms);

    for (PRUint32 i = 0; i < perms.Length(); i++) {
      const IPC::Permission &perm = perms[i];
      AddInternal(perm.host, perm.type, perm.capability, 0, perm.expireType,
                  perm.expireTime, eNotify, eNoDBOperation);
    }

    
    return NS_OK;
  }

  
  
  
  InitDB(false);

  return NS_OK;
}

nsresult
nsPermissionManager::InitDB(bool aRemoveFile)
{
  nsCOMPtr<nsIFile> permissionsFile;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(permissionsFile));
  if (!permissionsFile)
    return NS_ERROR_UNEXPECTED;

  nsresult rv = permissionsFile->AppendNative(NS_LITERAL_CSTRING(kPermissionsFileName));
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
    
    PRInt32 dbSchemaVersion;
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

        rv = mDBConn->SetSchemaVersion(HOSTS_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      
    
    case HOSTS_SCHEMA_VERSION:
      break;

    case 0:
      {
        NS_WARNING("couldn't get schema version!");
          
        
        
        
        
        
        rv = mDBConn->SetSchemaVersion(HOSTS_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      

    
    
    
    
    
    
    default:
      {
        
        nsCOMPtr<mozIStorageStatement> stmt;
        rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT host, type, permission, expireType, expireTime FROM moz_hosts"),
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

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_hosts "
    "(id, host, type, permission, expireType, expireTime) "
    "VALUES (?1, ?2, ?3, ?4, ?5, ?6)"), getter_AddRefs(mStmtInsert));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_hosts "
    "WHERE id = ?1"), getter_AddRefs(mStmtDelete));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_hosts "
    "SET permission = ?2, expireType= ?3, expireTime = ?4 WHERE id = ?1"),
    getter_AddRefs(mStmtUpdate));
  NS_ENSURE_SUCCESS(rv, rv);

  
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
    ")"));
}

NS_IMETHODIMP
nsPermissionManager::Add(nsIURI     *aURI,
                         const char *aType,
                         PRUint32    aPermission,
                         PRUint32    aExpireType,
                         PRInt64     aExpireTime)
{
  ENSURE_NOT_CHILD_PROCESS;

  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_TRUE(aExpireType == nsIPermissionManager::EXPIRE_NEVER ||
                 aExpireType == nsIPermissionManager::EXPIRE_TIME ||
                 aExpireType == nsIPermissionManager::EXPIRE_SESSION,
                 NS_ERROR_INVALID_ARG);

  nsresult rv;

  
  if (aExpireType == nsIPermissionManager::EXPIRE_TIME &&
      aExpireTime <= PR_Now() / 1000)
    return NS_OK;

  nsCAutoString host;
  rv = GetHost(aURI, host);
  NS_ENSURE_SUCCESS(rv, rv);

  return AddInternal(host, nsDependentCString(aType), aPermission, 0, 
                     aExpireType, aExpireTime, eNotify, eWriteToDB);
}

NS_IMETHODIMP
nsPermissionManager::AddFromPrincipal(nsIPrincipal* aPrincipal,
                                      const char* aType, PRUint32 aPermission,
                                      PRUint32 aExpireType, PRInt64 aExpireTime)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);

  nsCOMPtr<nsIURI> uri;
  aPrincipal->GetURI(getter_AddRefs(uri));

  return Add(uri, aType, aPermission, aExpireType, aExpireTime);
}

nsresult
nsPermissionManager::AddInternal(const nsAFlatCString &aHost,
                                 const nsAFlatCString &aType,
                                 PRUint32              aPermission,
                                 PRInt64               aID,
                                 PRUint32              aExpireType,
                                 PRInt64               aExpireTime,
                                 NotifyOperationType   aNotifyOperation,
                                 DBOperationType       aDBOperation)
{
  if (!IsChildProcess()) {
    IPC::Permission permission((aHost),
                               (aType),
                               aPermission, aExpireType, aExpireTime);

    nsTArray<ContentParent*> cplist;
    ContentParent::GetAll(cplist);
    for (PRUint32 i = 0; i < cplist.Length(); ++i) {
      ContentParent* cp = cplist[i];
      if (cp->NeedsPermissionsUpdate())
        unused << cp->SendAddPermission(permission);
    }
  }

  if (!gHostArena) {
    gHostArena = new PLArenaPool;
    if (!gHostArena)
      return NS_ERROR_OUT_OF_MEMORY;    
    PL_INIT_ARENA_POOL(gHostArena, "PermissionHostArena", HOST_ARENA_SIZE);
  }

  
  PRInt32 typeIndex = GetTypeIndex(aType.get(), true);
  NS_ENSURE_TRUE(typeIndex != -1, NS_ERROR_OUT_OF_MEMORY);

  
  
  nsHostEntry *entry = mHostTable.PutEntry(aHost.get());
  if (!entry) return NS_ERROR_FAILURE;
  if (!entry->GetKey()) {
    mHostTable.RawRemoveEntry(entry);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  OperationType op;
  PRInt32 index = entry->GetPermissionIndex(typeIndex);
  if (index == -1) {
    if (aPermission == nsIPermissionManager::UNKNOWN_ACTION)
      op = eOperationNone;
    else
      op = eOperationAdding;

  } else {
    nsPermissionEntry oldPermissionEntry = entry->GetPermissions()[index];

    
    
    
    
    
    if (aPermission == oldPermissionEntry.mPermission && 
        aExpireType == oldPermissionEntry.mExpireType &&
        (aExpireType != nsIPermissionManager::EXPIRE_TIME || 
         aExpireTime == oldPermissionEntry.mExpireTime))
      op = eOperationNone;
    else if (aPermission == nsIPermissionManager::UNKNOWN_ACTION)
      op = eOperationRemoving;
    else
      op = eOperationChanging;
  }

  
  
  PRInt64 id;
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

      entry->GetPermissions().AppendElement(nsPermissionEntry(typeIndex, aPermission, id, aExpireType, aExpireTime));

      if (aDBOperation == eWriteToDB && aExpireType != nsIPermissionManager::EXPIRE_SESSION)
        UpdateDB(op, mStmtInsert, id, aHost, aType, aPermission, aExpireType, aExpireTime);

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(aHost,
                                      mTypeArray[typeIndex],
                                      aPermission,
                                      aExpireType,
                                      aExpireTime,
                                      NS_LITERAL_STRING("added").get());
      }

      break;
    }

  case eOperationRemoving:
    {
      nsPermissionEntry oldPermissionEntry = entry->GetPermissions()[index];
      id = oldPermissionEntry.mID;
      entry->GetPermissions().RemoveElementAt(index);

      
      if (entry->GetPermissions().IsEmpty())
        mHostTable.RawRemoveEntry(entry);

      if (aDBOperation == eWriteToDB)
        UpdateDB(op, mStmtDelete, id, EmptyCString(), EmptyCString(), 0, 
                 nsIPermissionManager::EXPIRE_NEVER, 0);

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(aHost,
                                      mTypeArray[typeIndex],
                                      oldPermissionEntry.mPermission,
                                      oldPermissionEntry.mExpireType,
                                      oldPermissionEntry.mExpireTime,
                                      NS_LITERAL_STRING("deleted").get());
      }

      break;
    }

  case eOperationChanging:
    {
      id = entry->GetPermissions()[index].mID;
      entry->GetPermissions()[index].mPermission = aPermission;

      if (aDBOperation == eWriteToDB && aExpireType != nsIPermissionManager::EXPIRE_SESSION)
        UpdateDB(op, mStmtUpdate, id, EmptyCString(), EmptyCString(), aPermission, aExpireType, aExpireTime);

      if (aNotifyOperation == eNotify) {
        NotifyObserversWithPermission(aHost,
                                      mTypeArray[typeIndex],
                                      aPermission,
                                      aExpireType,
                                      aExpireTime,
                                      NS_LITERAL_STRING("changed").get());
      }

      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::Remove(const nsACString &aHost,
                            const char       *aType)
{
  ENSURE_NOT_CHILD_PROCESS;

  NS_ENSURE_ARG_POINTER(aType);

  
  return AddInternal(PromiseFlatCString(aHost),
                     nsDependentCString(aType),
                     nsIPermissionManager::UNKNOWN_ACTION,
                     0,
                     nsIPermissionManager::EXPIRE_NEVER,
                     0,
                     eNotify,
                     eWriteToDB);
}

NS_IMETHODIMP
nsPermissionManager::RemoveFromPrincipal(nsIPrincipal* aPrincipal,
                                         const char* aType)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);

  nsCOMPtr<nsIURI> uri;
  aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

  nsCAutoString host;
  uri->GetHost(host);

  return Remove(host, aType);
}

NS_IMETHODIMP
nsPermissionManager::RemoveAll()
{
  ENSURE_NOT_CHILD_PROCESS;
  return RemoveAllInternal(true);
}

void
nsPermissionManager::CloseDB(bool aRebuildOnSuccess)
{
  
  mStmtInsert = nsnull;
  mStmtDelete = nsnull;
  mStmtUpdate = nsnull;
  if (mDBConn) {
    mozIStorageCompletionCallback* cb = new CloseDatabaseListener(this,
           aRebuildOnSuccess);
    mozilla::DebugOnly<nsresult> rv = mDBConn->AsyncClose(cb);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    mDBConn = nsnull; 
  }
}

nsresult
nsPermissionManager::RemoveAllInternal(bool aNotifyObservers)
{
  
  
  
  RemoveAllFromMemory();
  if (aNotifyObservers) {
    NotifyObservers(nsnull, NS_LITERAL_STRING("cleared").get());
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
                                         PRUint32   *aPermission)
{
  return CommonTestPermission(aURI, aType, aPermission, true);
}

NS_IMETHODIMP
nsPermissionManager::TestPermission(nsIURI     *aURI,
                                    const char *aType,
                                    PRUint32   *aPermission)
{
  return CommonTestPermission(aURI, aType, aPermission, false);
}

NS_IMETHODIMP
nsPermissionManager::TestPermissionFromPrincipal(nsIPrincipal* aPrincipal,
                                                 const char* aType,
                                                 PRUint32* aPermission)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);

  nsCOMPtr<nsIURI> uri;
  aPrincipal->GetURI(getter_AddRefs(uri));

  return TestPermission(uri, aType, aPermission);
}

NS_IMETHODIMP
nsPermissionManager::TestExactPermissionFromPrincipal(nsIPrincipal* aPrincipal,
                                                      const char* aType,
                                                      PRUint32* aPermission)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);

  nsCOMPtr<nsIURI> uri;
  aPrincipal->GetURI(getter_AddRefs(uri));

  return TestExactPermission(uri, aType, aPermission);
}

nsresult
nsPermissionManager::CommonTestPermission(nsIURI     *aURI,
                                          const char *aType,
                                          PRUint32   *aPermission,
                                          bool        aExactHostMatch)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aType);

  
  *aPermission = nsIPermissionManager::UNKNOWN_ACTION;

  nsCAutoString host;
  nsresult rv = GetHost(aURI, host);
  
  
  if (NS_FAILED(rv)) {
    bool isFile;
    rv = aURI->SchemeIs("file", &isFile);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isFile) {
      host.AssignLiteral("<file>");
    }
    else {
      return NS_OK;
    }
  }
  
  PRInt32 typeIndex = GetTypeIndex(aType, false);
  
  
  if (typeIndex == -1) return NS_OK;

  nsHostEntry *entry = GetHostEntry(host, typeIndex, aExactHostMatch);
  if (entry)
    *aPermission = entry->GetPermission(typeIndex).mPermission;

  return NS_OK;
}






nsHostEntry *
nsPermissionManager::GetHostEntry(const nsAFlatCString &aHost,
                                  PRUint32              aType,
                                  bool                  aExactHostMatch)
{
  PRUint32 offset = 0;
  nsHostEntry *entry;
  PRInt64 now = PR_Now() / 1000;

  do {
    entry = mHostTable.GetEntry(aHost.get() + offset);
    if (entry) {
      nsPermissionEntry permEntry = entry->GetPermission(aType);

      
      if (permEntry.mExpireType == nsIPermissionManager::EXPIRE_TIME &&
          permEntry.mExpireTime <= now)
        Remove(aHost, mTypeArray[aType].get());
      else if (permEntry.mPermission != nsIPermissionManager::UNKNOWN_ACTION)
        break;

      
      entry = nsnull;
    }
    if (aExactHostMatch)
      break; 

    offset = aHost.FindChar('.', offset) + 1;

  
  
  } while (offset > 0);
  return entry;
}


struct nsGetEnumeratorData
{
  nsGetEnumeratorData(nsCOMArray<nsIPermission> *aArray, const nsTArray<nsCString> *aTypes)
   : array(aArray)
   , types(aTypes) {}

  nsCOMArray<nsIPermission> *array;
  const nsTArray<nsCString> *types;
};

static PLDHashOperator
AddPermissionsToList(nsHostEntry *entry, void *arg)
{
  nsGetEnumeratorData *data = static_cast<nsGetEnumeratorData *>(arg);

  for (PRUint32 i = 0; i < entry->GetPermissions().Length(); ++i) {
    nsPermissionEntry &permEntry = entry->GetPermissions()[i];

    nsPermission *perm = new nsPermission(entry->GetHost(), 
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

  mHostTable.EnumerateEntries(AddPermissionsToList, &data);

  return NS_NewArrayEnumerator(aEnum, array);
}

NS_IMETHODIMP nsPermissionManager::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  ENSURE_NOT_CHILD_PROCESS;

  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    
    
    mIsShuttingDown = true;
    if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
      
      RemoveAllInternal(false);
    } else {
      RemoveAllFromMemory();
      CloseDB(false);
    }
  }
  else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    
    InitDB(false);
  }

  return NS_OK;
}





nsresult
nsPermissionManager::RemoveAllFromMemory()
{
  mLargestID = 0;
  mTypeArray.Clear();
  mHostTable.Clear();
  if (gHostArena) {
    PL_FinishArenaPool(gHostArena);
    delete gHostArena;
  }
  gHostArena = nsnull;
  return NS_OK;
}


PRInt32
nsPermissionManager::GetTypeIndex(const char *aType,
                                  bool        aAdd)
{
  for (PRUint32 i = 0; i < mTypeArray.Length(); ++i)
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
                                                   const nsCString  &aType,
                                                   PRUint32          aPermission,
                                                   PRUint32          aExpireType,
                                                   PRInt64           aExpireTime,
                                                   const PRUnichar  *aData)
{
  nsCOMPtr<nsIPermission> permission =
    new nsPermission(aHost, aType, aPermission, aExpireType, aExpireTime);
  if (permission)
    NotifyObservers(permission, aData);
}







void
nsPermissionManager::NotifyObservers(nsIPermission   *aPermission,
                                     const PRUnichar *aData)
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
    "SELECT id, host, type, permission, expireType, expireTime "
    "FROM moz_hosts"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 id;
  nsCAutoString host, type;
  PRUint32 permission;
  PRUint32 expireType;
  PRInt64 expireTime;
  bool hasResult;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    
    
    id = stmt->AsInt64(0);
    if (id > mLargestID)
      mLargestID = id;

    rv = stmt->GetUTF8String(1, host);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->GetUTF8String(2, type);
    NS_ENSURE_SUCCESS(rv, rv);

    permission = stmt->AsInt32(3);
    expireType = stmt->AsInt32(4);

    
    expireTime = stmt->AsInt64(5);

    rv = AddInternal(host, type, permission, id, expireType, expireTime,
                     eDontNotify, eNoDBOperation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

static const char kMatchTypeHost[] = "host";

nsresult
nsPermissionManager::Import()
{
  ENSURE_NOT_CHILD_PROCESS;

  nsresult rv;

  nsCOMPtr<nsIFile> permissionsFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(permissionsFile));
  if (NS_FAILED(rv)) return rv;

  rv = permissionsFile->AppendNative(NS_LITERAL_CSTRING(kHostpermFileName));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream),
                                  permissionsFile);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mozStorageTransaction transaction(mDBConn, true);

  






  nsCAutoString buffer;
  bool isMore = true;
  while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
    if (buffer.IsEmpty() || buffer.First() == '#') {
      continue;
    }

    nsTArray<nsCString> lineArray;

    
    ParseString(buffer, '\t', lineArray);
    
    if (lineArray[0].EqualsLiteral(kMatchTypeHost) &&
        lineArray.Length() == 4) {
      
      PRInt32 error;
      PRUint32 permission = lineArray[2].ToInteger(&error);
      if (error)
        continue;

      
      if (!IsASCII(lineArray[3])) {
        rv = NormalizeToACE(lineArray[3]);
        if (NS_FAILED(rv))
          continue;
      }

      rv = AddInternal(lineArray[3], lineArray[1], permission, 0, 
                       nsIPermissionManager::EXPIRE_NEVER, 0, eDontNotify, eWriteToDB);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  permissionsFile->Remove(false);

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

nsresult
nsPermissionManager::GetHost(nsIURI *aURI, nsACString &aResult)
{
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aURI);
  if (!innerURI) return NS_ERROR_FAILURE;

  nsresult rv = innerURI->GetAsciiHost(aResult);

  if (NS_FAILED(rv) || aResult.IsEmpty())
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}

void
nsPermissionManager::UpdateDB(OperationType         aOp,
                              mozIStorageStatement* aStmt,
                              PRInt64               aID,
                              const nsACString     &aHost,
                              const nsACString     &aType,
                              PRUint32              aPermission,
                              PRUint32              aExpireType,
                              PRInt64               aExpireTime)
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
      break;
    }

  default:
    {
      NS_NOTREACHED("need a valid operation in UpdateDB()!");
      rv = NS_ERROR_UNEXPECTED;
      break;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    bool hasResult;
    rv = aStmt->ExecuteStep(&hasResult);
    aStmt->Reset();
  }

  if (NS_FAILED(rv))
    NS_WARNING("db change failed!");
}

