





#include "IndexedDatabaseManager.h"
#include "DatabaseInfo.h"

#include "nsIDOMScriptObjectFactory.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsISHEntry.h"
#include "nsISimpleEnumerator.h"
#include "nsITimer.h"

#include "mozilla/LazyIdleThread.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/storage.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsContentUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "test_quota.h"
#include "xpcpublic.h"

#include "AsyncConnectionHelper.h"
#include "CheckQuotaHelper.h"
#include "IDBDatabase.h"
#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IDBKeyRange.h"
#include "OpenDatabaseHelper.h"
#include "TransactionThreadPool.h"



#define DEFAULT_THREAD_TIMEOUT_MS 30000



#define DEFAULT_SHUTDOWN_TIMER_MS 30000


#define DEFAULT_QUOTA_MB 50


#define PREF_INDEXEDDB_QUOTA "dom.indexedDB.warningQuota"

USING_INDEXEDDB_NAMESPACE
using namespace mozilla::services;
using mozilla::Preferences;

static NS_DEFINE_CID(kDOMSOF_CID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

namespace {

PRInt32 gShutdown = 0;
PRInt32 gClosed = 0;


IndexedDatabaseManager* gInstance = nsnull;

PRInt32 gIndexedDBQuotaMB = DEFAULT_QUOTA_MB;

bool
GetBaseFilename(const nsAString& aFilename,
                nsAString& aBaseFilename)
{
  NS_ASSERTION(!aFilename.IsEmpty(), "Bad argument!");

  NS_NAMED_LITERAL_STRING(sqlite, ".sqlite");
  nsAString::size_type filenameLen = aFilename.Length();
  nsAString::size_type sqliteLen = sqlite.Length();

  if (sqliteLen > filenameLen ||
      Substring(aFilename, filenameLen - sqliteLen, sqliteLen) != sqlite) {
    return false;
  }

  aBaseFilename = Substring(aFilename, 0, filenameLen - sqliteLen);

  return true;
}

class QuotaCallback MOZ_FINAL : public mozIStorageQuotaCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  QuotaExceeded(const nsACString& aFilename,
                PRInt64 aCurrentSizeLimit,
                PRInt64 aCurrentTotalSize,
                nsISupports* aUserData,
                PRInt64* _retval)
  {
    if (IndexedDatabaseManager::QuotaIsLifted()) {
      *_retval = 0;
      return NS_OK;
    }

    return NS_ERROR_FAILURE;
  }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(QuotaCallback, mozIStorageQuotaCallback)


PLDHashOperator
EnumerateToTArray(const nsACString& aKey,
                  nsTArray<IDBDatabase*>* aValue,
                  void* aUserArg)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");
  NS_ASSERTION(aUserArg, "Null pointer!");

  nsTArray<IDBDatabase*>* array =
    static_cast<nsTArray<IDBDatabase*>*>(aUserArg);

  if (!array->AppendElements(*aValue)) {
    NS_WARNING("Out of memory!");
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

PLDHashOperator
InvalidateAllFileManagers(const nsACString& aKey,
                          nsTArray<nsRefPtr<FileManager> >* aValue,
                          void* aUserArg)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  for (PRUint32 i = 0; i < aValue->Length(); i++) {
    nsRefPtr<FileManager> fileManager = aValue->ElementAt(i);
    fileManager->Invalidate();
  }

  return PL_DHASH_NEXT;
}

} 

IndexedDatabaseManager::IndexedDatabaseManager()
: mCurrentWindowIndex(BAD_TLS_INDEX),
  mQuotaHelperMutex("IndexedDatabaseManager.mQuotaHelperMutex"),
  mFileMutex("IndexedDatabaseManager.mFileMutex")
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance, "More than one instance!");
}

IndexedDatabaseManager::~IndexedDatabaseManager()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance || gInstance == this, "Different instances!");
  gInstance = nsnull;
}

bool IndexedDatabaseManager::sIsMainProcess = false;


already_AddRefed<IndexedDatabaseManager>
IndexedDatabaseManager::GetOrCreate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (IsShuttingDown()) {
    NS_ERROR("Calling GetOrCreateInstance() after shutdown!");
    return nsnull;
  }

  nsRefPtr<IndexedDatabaseManager> instance(gInstance);

  if (!instance) {
    sIsMainProcess = XRE_GetProcessType() == GeckoProcessType_Default;

    instance = new IndexedDatabaseManager();

    instance->mLiveDatabases.Init();
    instance->mQuotaHelperHash.Init();
    instance->mFileManagers.Init();

    
    NS_ASSERTION(instance->mCurrentWindowIndex == BAD_TLS_INDEX, "Huh?");

    if (PR_NewThreadPrivateIndex(&instance->mCurrentWindowIndex, nsnull) !=
        PR_SUCCESS) {
      NS_ERROR("PR_NewThreadPrivateIndex failed, IndexedDB disabled");
      instance->mCurrentWindowIndex = BAD_TLS_INDEX;
      return nsnull;
    }

    nsresult rv;

    if (sIsMainProcess) {
      nsCOMPtr<nsIFile> dbBaseDirectory;
      rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                  getter_AddRefs(dbBaseDirectory));
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = dbBaseDirectory->Append(NS_LITERAL_STRING("indexedDB"));
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = dbBaseDirectory->GetPath(instance->mDatabaseBasePath);
      NS_ENSURE_SUCCESS(rv, nsnull);

      
      
      instance->mIOThread = new LazyIdleThread(DEFAULT_THREAD_TIMEOUT_MS,
                                                LazyIdleThread::ManualShutdown);

      
      instance->mQuotaCallbackSingleton = new QuotaCallback();

      
      
      instance->mShutdownTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
      NS_ENSURE_TRUE(instance->mShutdownTimer, nsnull);
    }

    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    NS_ENSURE_TRUE(obs, nsnull);

    
    rv = obs->AddObserver(instance, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
    NS_ENSURE_SUCCESS(rv, nsnull);

    if (NS_FAILED(Preferences::AddIntVarCache(&gIndexedDBQuotaMB,
                                              PREF_INDEXEDDB_QUOTA,
                                              DEFAULT_QUOTA_MB))) {
      NS_WARNING("Unable to respond to quota pref changes!");
      gIndexedDBQuotaMB = DEFAULT_QUOTA_MB;
    }

    
    gInstance = instance;
  }

  return instance.forget();
}


IndexedDatabaseManager*
IndexedDatabaseManager::Get()
{
  
  return gInstance;
}


IndexedDatabaseManager*
IndexedDatabaseManager::FactoryCreate()
{
  
  
  return GetOrCreate().get();
}

nsresult
IndexedDatabaseManager::GetDirectoryForOrigin(const nsACString& aASCIIOrigin,
                                              nsIFile** aDirectory) const
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> directory =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = directory->InitWithPath(GetBaseDirectory());
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertASCIItoUTF16 originSanitized(aASCIIOrigin);
  originSanitized.ReplaceChar(":/", '+');

  rv = directory->Append(originSanitized);
  NS_ENSURE_SUCCESS(rv, rv);

  directory.forget(reinterpret_cast<nsILocalFile**>(aDirectory));
  return NS_OK;
}


already_AddRefed<nsIAtom>
IndexedDatabaseManager::GetDatabaseId(const nsACString& aOrigin,
                                      const nsAString& aName)
{
  nsCString str(aOrigin);
  str.Append("*");
  str.Append(NS_ConvertUTF16toUTF8(aName));

  nsCOMPtr<nsIAtom> atom = do_GetAtom(str);
  NS_ENSURE_TRUE(atom, nsnull);

  return atom.forget();
}

bool
IndexedDatabaseManager::RegisterDatabase(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  if (IsShuttingDown()) {
    return false;
  }

  
  nsTArray<IDBDatabase*>* array;
  if (!mLiveDatabases.Get(aDatabase->Origin(), &array)) {
    nsAutoPtr<nsTArray<IDBDatabase*> > newArray(new nsTArray<IDBDatabase*>());
    mLiveDatabases.Put(aDatabase->Origin(), newArray);
    array = newArray.forget();
  }
  if (!array->AppendElement(aDatabase)) {
    NS_WARNING("Out of memory?");
    return false;
  }

  aDatabase->mRegistered = true;
  return true;
}

void
IndexedDatabaseManager::UnregisterDatabase(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  
  nsTArray<IDBDatabase*>* array;
  if (mLiveDatabases.Get(aDatabase->Origin(), &array) &&
      array->RemoveElement(aDatabase)) {
    if (array->IsEmpty()) {
      mLiveDatabases.Remove(aDatabase->Origin());
    }
    return;
  }
  NS_ERROR("Didn't know anything about this database!");
}

void
IndexedDatabaseManager::OnUsageCheckComplete(AsyncUsageRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aRunnable, "Null pointer!");
  NS_ASSERTION(!aRunnable->mURI, "Should have been cleared!");
  NS_ASSERTION(!aRunnable->mCallback, "Should have been cleared!");

  if (!mUsageRunnables.RemoveElement(aRunnable)) {
    NS_ERROR("Don't know anything about this runnable!");
  }
}

nsresult
IndexedDatabaseManager::WaitForOpenAllowed(const nsACString& aOrigin,
                                           nsIAtom* aId,
                                           nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");
  NS_ASSERTION(aRunnable, "Null pointer!");

  nsAutoPtr<SynchronizedOp> op(new SynchronizedOp(aOrigin, aId));

  
  bool delayed = false;
  for (PRUint32 index = mSynchronizedOps.Length(); index > 0; index--) {
    nsAutoPtr<SynchronizedOp>& existingOp = mSynchronizedOps[index - 1];
    if (op->MustWaitFor(*existingOp)) {
      existingOp->DelayRunnable(aRunnable);
      delayed = true;
      break;
    }
  }

  
  if (!delayed) {
    nsresult rv = NS_DispatchToCurrentThread(aRunnable);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  mSynchronizedOps.AppendElement(op.forget());

  return NS_OK;
}

void
IndexedDatabaseManager::AllowNextSynchronizedOp(const nsACString& aOrigin,
                                                nsIAtom* aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");

  PRUint32 count = mSynchronizedOps.Length();
  for (PRUint32 index = 0; index < count; index++) {
    nsAutoPtr<SynchronizedOp>& op = mSynchronizedOps[index];
    if (op->mOrigin.Equals(aOrigin)) {
      if (op->mId == aId) {
        NS_ASSERTION(op->mDatabases.IsEmpty(), "How did this happen?");

        op->DispatchDelayedRunnables();

        mSynchronizedOps.RemoveElementAt(index);
        return;
      }

      
      
      NS_ASSERTION(op->mId && aId, "Why didn't we match earlier?");
    }
  }

  NS_NOTREACHED("Why didn't we find a SynchronizedOp?");
}

nsresult
IndexedDatabaseManager::AcquireExclusiveAccess(const nsACString& aOrigin, 
                                               IDBDatabase* aDatabase,
                                               AsyncConnectionHelper* aHelper,
                                               WaitingOnDatabasesCallback aCallback,
                                               void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aHelper, "Why are you talking to me?");

  
  SynchronizedOp* op = nsnull;
  PRUint32 count = mSynchronizedOps.Length();
  for (PRUint32 index = 0; index < count; index++) {
    SynchronizedOp* currentop = mSynchronizedOps[index].get();
    if (currentop->mOrigin.Equals(aOrigin)) {
      if (!currentop->mId ||
          (aDatabase && currentop->mId == aDatabase->Id())) {
        
        NS_ASSERTION(!currentop->mHelper,
                     "SynchronizedOp already has a helper?!?");
        op = currentop;
        break;
      }
    }
  }

  NS_ASSERTION(op, "We didn't find a SynchronizedOp?");

  nsTArray<IDBDatabase*>* array;
  mLiveDatabases.Get(aOrigin, &array);

  
  
  
  nsTArray<nsRefPtr<IDBDatabase> > liveDatabases;

  if (array) {
    PRUint32 count = array->Length();
    for (PRUint32 index = 0; index < count; index++) {
      IDBDatabase*& database = array->ElementAt(index);
      if (!database->IsClosed() &&
          (!aDatabase ||
           (aDatabase &&
            database != aDatabase &&
            database->Id() == aDatabase->Id()))) {
        liveDatabases.AppendElement(database);
      }
    }
  }

  if (liveDatabases.IsEmpty()) {
    IndexedDatabaseManager::DispatchHelper(aHelper);
    return NS_OK;
  }

  NS_ASSERTION(op->mDatabases.IsEmpty(), "How do we already have databases here?");
  op->mDatabases.AppendElements(liveDatabases);
  op->mHelper = aHelper;

  
  aCallback(liveDatabases, aClosure);

  NS_ASSERTION(liveDatabases.IsEmpty(),
               "Should have done something with the array!");
  return NS_OK;
}


bool
IndexedDatabaseManager::IsShuttingDown()
{
  return !!gShutdown;
}


bool
IndexedDatabaseManager::IsClosed()
{
  return !!gClosed;
}

void
IndexedDatabaseManager::AbortCloseDatabasesForWindow(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWindow, "Null pointer!");

  nsAutoTArray<IDBDatabase*, 50> liveDatabases;
  mLiveDatabases.EnumerateRead(EnumerateToTArray, &liveDatabases);

  TransactionThreadPool* pool = TransactionThreadPool::Get();

  for (PRUint32 index = 0; index < liveDatabases.Length(); index++) {
    IDBDatabase*& database = liveDatabases[index];
    if (database->GetOwner() == aWindow) {
      if (NS_FAILED(database->Close())) {
        NS_WARNING("Failed to close database for dying window!");
      }

      if (pool) {
        pool->AbortTransactionsForDatabase(database);
      }
    }
  }
}

bool
IndexedDatabaseManager::HasOpenTransactions(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWindow, "Null pointer!");

  nsAutoTArray<IDBDatabase*, 50> liveDatabases;
  mLiveDatabases.EnumerateRead(EnumerateToTArray, &liveDatabases);

  TransactionThreadPool* pool = TransactionThreadPool::Get();
  if (!pool) {
    return false;
  }

  for (PRUint32 index = 0; index < liveDatabases.Length(); index++) {
    IDBDatabase*& database = liveDatabases[index];
    if (database->GetOwner() == aWindow &&
        pool->HasTransactionsForDatabase(database)) {
      return true;
    }
  }
  
  return false;
}

void
IndexedDatabaseManager::OnDatabaseClosed(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  
  PRUint32 count = mSynchronizedOps.Length();
  for (PRUint32 index = 0; index < count; index++) {
    nsAutoPtr<SynchronizedOp>& op = mSynchronizedOps[index];

    if (op->mOrigin == aDatabase->Origin() &&
        (op->mId == aDatabase->Id() || !op->mId)) {
      
      
      if (op->mDatabases.RemoveElement(aDatabase)) {
        
        NS_ASSERTION(op->mHelper, "How did we get rid of the helper before "
                     "removing the last database?");
        if (op->mDatabases.IsEmpty()) {
          
          
          
          

          TransactionThreadPool* pool = TransactionThreadPool::GetOrCreate();
          if (!pool) {
            NS_ERROR("IndexedDB is totally broken.");
            return;
          }

          nsRefPtr<WaitForTransactionsToFinishRunnable> waitRunnable =
            new WaitForTransactionsToFinishRunnable(op);

          nsAutoTArray<nsRefPtr<IDBDatabase>, 1> array;
          array.AppendElement(aDatabase);

          
          if (!pool->WaitForAllDatabasesToComplete(array, waitRunnable)) {
            NS_WARNING("Failed to wait for transaction to complete!");
          }
        }
        break;
      }
    }
  }
}

void
IndexedDatabaseManager::SetCurrentWindowInternal(nsPIDOMWindow* aWindow)
{
  if (aWindow) {
#ifdef DEBUG
    NS_ASSERTION(!PR_GetThreadPrivate(mCurrentWindowIndex),
                 "Somebody forgot to clear the current window!");
#endif
    PR_SetThreadPrivate(mCurrentWindowIndex, aWindow);
  }
  else {
    
    
    
    
    PR_SetThreadPrivate(mCurrentWindowIndex, nsnull);
  }
}


PRUint32
IndexedDatabaseManager::GetIndexedDBQuotaMB()
{
  return PRUint32(NS_MAX(gIndexedDBQuotaMB, 0));
}

nsresult
IndexedDatabaseManager::EnsureOriginIsInitialized(const nsACString& aOrigin,
                                                  nsIFile** aDirectory)
{
#ifdef DEBUG
  {
    bool correctThread;
    NS_ASSERTION(NS_SUCCEEDED(mIOThread->IsOnCurrentThread(&correctThread)) &&
                 correctThread,
                 "Running on the wrong thread!");
  }
#endif

  nsCOMPtr<nsIFile> directory;
  nsresult rv = GetDirectoryForOrigin(aOrigin, getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = directory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    bool isDirectory;
    rv = directory->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(isDirectory, NS_ERROR_UNEXPECTED);
  }
  else {
    rv = directory->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mFileManagers.Get(aOrigin)) {
    NS_ADDREF(*aDirectory = directory);
    return NS_OK;
  }

  
  nsCOMPtr<nsIFile> patternFile;
  rv = directory->Clone(getter_AddRefs(patternFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = patternFile->Append(NS_LITERAL_STRING("*"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString pattern;
  rv = patternFile->GetNativePath(pattern);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageServiceQuotaManagement> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(ss, NS_ERROR_FAILURE);

  rv = ss->SetQuotaForFilenamePattern(pattern,
                                      GetIndexedDBQuotaMB() * 1024 * 1024,
                                      mQuotaCallbackSingleton, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  nsAutoTArray<nsString, 20> subdirsToProcess;
  nsAutoTArray<nsCOMPtr<nsIFile> , 20> unknownFiles;

  nsAutoPtr<nsTArray<nsRefPtr<FileManager> > > fileManagers(
    new nsTArray<nsRefPtr<FileManager> >());

  nsTHashtable<nsStringHashKey> validSubdirs;
  validSubdirs.Init(20);

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
    NS_ENSURE_TRUE(file, NS_NOINTERFACE);

    nsString leafName;
    rv = file->GetLeafName(leafName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (StringEndsWith(leafName, NS_LITERAL_STRING(".sqlite-journal"))) {
      continue;
    }

    bool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isDirectory) {
      if (!validSubdirs.GetEntry(leafName)) {
        subdirsToProcess.AppendElement(leafName);
      }
      continue;
    }

    nsString dbBaseFilename;
    if (!GetBaseFilename(leafName, dbBaseFilename)) {
      unknownFiles.AppendElement(file);
      continue;
    }

    nsCOMPtr<nsIFile> fileManagerDirectory;
    rv = directory->Clone(getter_AddRefs(fileManagerDirectory));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = fileManagerDirectory->Append(dbBaseFilename);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<mozIStorageConnection> connection;
    rv = OpenDatabaseHelper::CreateDatabaseConnection(
      NullString(), file, fileManagerDirectory, getter_AddRefs(connection));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<mozIStorageStatement> stmt;
    rv = connection->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT name "
      "FROM database"
    ), getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!hasResult) {
      NS_ERROR("Database has no name!");
      return NS_ERROR_UNEXPECTED;
    }

    nsString databaseName;
    rv = stmt->GetString(0, databaseName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<FileManager> fileManager = new FileManager(aOrigin, databaseName);

    rv = fileManager->Init(fileManagerDirectory, connection);
    NS_ENSURE_SUCCESS(rv, rv);

    fileManagers->AppendElement(fileManager);

    rv = ss->UpdateQuotaInformationForFile(file);
    NS_ENSURE_SUCCESS(rv, rv);

    validSubdirs.PutEntry(dbBaseFilename);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < subdirsToProcess.Length(); i++) {
    const nsString& subdir = subdirsToProcess[i];
    if (!validSubdirs.GetEntry(subdir)) {
      NS_WARNING("Unknown subdirectory found!");
      return NS_ERROR_UNEXPECTED;
    }
  }

  for (PRUint32 i = 0; i < unknownFiles.Length(); i++) {
    nsCOMPtr<nsIFile>& unknownFile = unknownFiles[i];

    
    
    bool exists;
    rv = unknownFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);

    if (exists) {
      nsString leafName;
      unknownFile->GetLeafName(leafName);

      
      if (!StringEndsWith(leafName, NS_LITERAL_STRING(".sqlite-journal"))) {
        NS_WARNING("Unknown file found!");
        return NS_ERROR_UNEXPECTED;
      }
    }
  }

  mFileManagers.Put(aOrigin, fileManagers);
  fileManagers.forget();

  NS_ADDREF(*aDirectory = directory);
  return NS_OK;
}

bool
IndexedDatabaseManager::QuotaIsLiftedInternal()
{
  nsPIDOMWindow* window = nsnull;
  nsRefPtr<CheckQuotaHelper> helper = nsnull;
  bool createdHelper = false;

  window =
    static_cast<nsPIDOMWindow*>(PR_GetThreadPrivate(mCurrentWindowIndex));

  
  
  NS_ASSERTION(window, "Why don't we have a Window here?");

  
  MutexAutoLock autoLock(mQuotaHelperMutex);

  mQuotaHelperHash.Get(window, getter_AddRefs(helper));

  if (!helper) {
    helper = new CheckQuotaHelper(window, mQuotaHelperMutex);
    createdHelper = true;

    mQuotaHelperHash.Put(window, helper);

    
    {
      MutexAutoUnlock autoUnlock(mQuotaHelperMutex);

      nsresult rv = NS_DispatchToMainThread(helper);
      NS_ENSURE_SUCCESS(rv, false);
    }

    
    
    
  }

  bool result = helper->PromptAndReturnQuotaIsDisabled();

  
  
  if (createdHelper) {
    mQuotaHelperHash.Remove(window);
  }

  return result;
}

void
IndexedDatabaseManager::CancelPromptsForWindowInternal(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<CheckQuotaHelper> helper;

  MutexAutoLock autoLock(mQuotaHelperMutex);

  mQuotaHelperHash.Get(aWindow, getter_AddRefs(helper));

  if (helper) {
    helper->Cancel();
  }
}


nsresult
IndexedDatabaseManager::GetASCIIOriginFromWindow(nsPIDOMWindow* aWindow,
                                                 nsCString& aASCIIOrigin)
{
  NS_ASSERTION(NS_IsMainThread(),
               "We're about to touch a window off the main thread!");

  if (!aWindow) {
    aASCIIOrigin.AssignLiteral("chrome");
    NS_ASSERTION(nsContentUtils::IsCallerChrome(),
                 "Null window but not chrome!");
    return NS_OK;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(sop, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();
  NS_ENSURE_TRUE(principal, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  if (nsContentUtils::IsSystemPrincipal(principal)) {
    aASCIIOrigin.AssignLiteral("chrome");
  }
  else {
    nsresult rv = nsContentUtils::GetASCIIOrigin(principal, aASCIIOrigin);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

    if (aASCIIOrigin.EqualsLiteral("null")) {
      NS_WARNING("IndexedDB databases not allowed for this principal!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  }

  return NS_OK;
}

#ifdef DEBUG

bool
IndexedDatabaseManager::IsMainProcess()
{
  NS_ASSERTION(gInstance,
               "IsMainProcess() called before indexedDB has been initialized!");
  NS_ASSERTION((XRE_GetProcessType() == GeckoProcessType_Default) ==
               sIsMainProcess, "XRE_GetProcessType changed its tune!");
  return sIsMainProcess;
}
#endif

already_AddRefed<FileManager>
IndexedDatabaseManager::GetOrCreateFileManager(const nsACString& aOrigin,
                                               const nsAString& aDatabaseName)
{
  nsTArray<nsRefPtr<FileManager> >* array;
  if (!mFileManagers.Get(aOrigin, &array)) {
    nsAutoPtr<nsTArray<nsRefPtr<FileManager> > > newArray(
      new nsTArray<nsRefPtr<FileManager> >());
    mFileManagers.Put(aOrigin, newArray);
    array = newArray.forget();
  }

  nsRefPtr<FileManager> fileManager;
  for (PRUint32 i = 0; i < array->Length(); i++) {
    nsRefPtr<FileManager> fm = array->ElementAt(i);

    if (fm->DatabaseName().Equals(aDatabaseName)) {
      fileManager = fm.forget();
      break;
    }
  }
  
  if (!fileManager) {
    fileManager = new FileManager(aOrigin, aDatabaseName);

    array->AppendElement(fileManager);
  }

  return fileManager.forget();
}

already_AddRefed<FileManager>
IndexedDatabaseManager::GetFileManager(const nsACString& aOrigin,
                                       const nsAString& aDatabaseName)
{
  nsTArray<nsRefPtr<FileManager> >* array;
  if (!mFileManagers.Get(aOrigin, &array)) {
    return nsnull;
  }

  for (PRUint32 i = 0; i < array->Length(); i++) {
    nsRefPtr<FileManager>& fileManager = array->ElementAt(i);

    if (fileManager->DatabaseName().Equals(aDatabaseName)) {
      nsRefPtr<FileManager> result = fileManager;
      return result.forget();
    }
  }
  
  return nsnull;
}

void
IndexedDatabaseManager::InvalidateFileManagersForOrigin(
                                                     const nsACString& aOrigin)
{
  nsTArray<nsRefPtr<FileManager> >* array;
  if (mFileManagers.Get(aOrigin, &array)) {
    for (PRUint32 i = 0; i < array->Length(); i++) {
      nsRefPtr<FileManager> fileManager = array->ElementAt(i);
      fileManager->Invalidate();
    }
    mFileManagers.Remove(aOrigin);
  }
}

void
IndexedDatabaseManager::InvalidateFileManager(const nsACString& aOrigin,
                                              const nsAString& aDatabaseName)
{
  nsTArray<nsRefPtr<FileManager> >* array;
  if (!mFileManagers.Get(aOrigin, &array)) {
    return;
  }

  for (PRUint32 i = 0; i < array->Length(); i++) {
    nsRefPtr<FileManager> fileManager = array->ElementAt(i);
    if (fileManager->DatabaseName().Equals(aDatabaseName)) {
      fileManager->Invalidate();
      array->RemoveElementAt(i);

      if (array->IsEmpty()) {
        mFileManagers.Remove(aOrigin);
      }

      break;
    }
  }
}

nsresult
IndexedDatabaseManager::AsyncDeleteFile(FileManager* aFileManager,
                                        PRInt64 aFileId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aFileManager);

  
  
  if (IsClearOriginPending(aFileManager->Origin())) {
    return NS_OK;
  }

  nsCOMPtr<nsIFile> directory = aFileManager->GetDirectory();
  NS_ENSURE_TRUE(directory, NS_ERROR_FAILURE);

  nsCOMPtr<nsIFile> file = aFileManager->GetFileForId(directory, aFileId);
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  nsString filePath;
  nsresult rv = file->GetPath(filePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<AsyncDeleteFileRunnable> runnable =
    new AsyncDeleteFileRunnable(filePath);

  rv = mIOThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
IndexedDatabaseManager::DispatchHelper(AsyncConnectionHelper* aHelper)
{
  nsresult rv = NS_OK;

  
  
  if (aHelper->HasTransaction()) {
    rv = aHelper->DispatchToTransactionPool();
  }
  else {
    
    IndexedDatabaseManager* manager = IndexedDatabaseManager::Get();
    NS_ASSERTION(manager, "We should definitely have a manager here");

    rv = aHelper->Dispatch(manager->IOThread());
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

bool
IndexedDatabaseManager::IsClearOriginPending(const nsACString& origin)
{
  
  
  PRUint32 count = mSynchronizedOps.Length();
  for (PRUint32 index = 0; index < count; index++) {
    nsAutoPtr<SynchronizedOp>& op = mSynchronizedOps[index];
    if (op->mOrigin.Equals(origin) && !op->mId) {
      return true;
    }
  }

  return false;
}

NS_IMPL_ISUPPORTS2(IndexedDatabaseManager, nsIIndexedDatabaseManager,
                                           nsIObserver)

NS_IMETHODIMP
IndexedDatabaseManager::GetUsageForURI(
                                     nsIURI* aURI,
                                     nsIIndexedDatabaseUsageCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aCallback);

  
  nsCString origin;
  nsresult rv = nsContentUtils::GetASCIIOrigin(aURI, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<AsyncUsageRunnable> runnable =
    new AsyncUsageRunnable(aURI, origin, aCallback);

  nsRefPtr<AsyncUsageRunnable>* newRunnable =
    mUsageRunnables.AppendElement(runnable);
  NS_ENSURE_TRUE(newRunnable, NS_ERROR_OUT_OF_MEMORY);

  
  
  if (origin.EqualsLiteral("null")) {
    rv = NS_DispatchToCurrentThread(runnable);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  
  
  if (IsClearOriginPending(origin)) {
    rv = NS_DispatchToCurrentThread(runnable);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  
  rv = mIOThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::CancelGetUsageForURI(
                                     nsIURI* aURI,
                                     nsIIndexedDatabaseUsageCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aCallback);

  
  
  for (PRUint32 index = 0; index < mUsageRunnables.Length(); index++) {
    nsRefPtr<AsyncUsageRunnable>& runnable = mUsageRunnables[index];

    bool equals;
    nsresult rv = runnable->mURI->Equals(aURI, &equals);
    NS_ENSURE_SUCCESS(rv, rv);

    if (equals && SameCOMIdentity(aCallback, runnable->mCallback)) {
      runnable->Cancel();
      break;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::ClearDatabasesForURI(nsIURI* aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);

  
  nsCString origin;
  nsresult rv = nsContentUtils::GetASCIIOrigin(aURI, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (origin.EqualsLiteral("null")) {
    return NS_OK;
  }

  
  
  if (IsClearOriginPending(origin)) {
    return NS_OK;
  }

  
  nsRefPtr<OriginClearRunnable> runnable =
    new OriginClearRunnable(origin, mIOThread);

  rv = WaitForOpenAllowed(origin, nsnull, runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsTArray<nsRefPtr<IDBDatabase> > liveDatabases;

  
  nsTArray<IDBDatabase*>* array;
  if (mLiveDatabases.Get(origin, &array)) {
    liveDatabases.AppendElements(*array);
  }

  
  for (PRUint32 index = 0; index < liveDatabases.Length(); index++) {
    liveDatabases[index]->Invalidate();
  }
  
  DatabaseInfo::RemoveAllForOrigin(origin);

  
  

  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    
    
    if (PR_ATOMIC_SET(&gShutdown, 1)) {
      NS_ERROR("Shutdown more than once?!");
    }

    if (sIsMainProcess) {
      
      if (NS_FAILED(mIOThread->Shutdown())) {
        NS_WARNING("Failed to shutdown IO thread!");
      }

      
      if (NS_FAILED(mShutdownTimer->Init(this, DEFAULT_SHUTDOWN_TIMER_MS,
                                         nsITimer::TYPE_ONE_SHOT))) {
        NS_WARNING("Failed to initialize shutdown timer!");
      }

      
      
      TransactionThreadPool::Shutdown();

      
      if (NS_FAILED(mShutdownTimer->Cancel())) {
        NS_WARNING("Failed to cancel shutdown timer!");
      }
    }

    mFileManagers.EnumerateRead(InvalidateAllFileManagers, nsnull);

    if (PR_ATOMIC_SET(&gClosed, 1)) {
      NS_ERROR("Close more than once?!");
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC)) {
    NS_ASSERTION(sIsMainProcess, "Should only happen in the main process!");

    NS_WARNING("Some database operations are taking longer than expected "
               "during shutdown and will be aborted!");

    
    nsAutoTArray<IDBDatabase*, 50> liveDatabases;
    mLiveDatabases.EnumerateRead(EnumerateToTArray, &liveDatabases);

    
    if (!liveDatabases.IsEmpty()) {
      PRUint32 count = liveDatabases.Length();
      for (PRUint32 index = 0; index < count; index++) {
        liveDatabases[index]->Invalidate();
      }
    }

    return NS_OK;
  }

  NS_NOTREACHED("Unknown topic!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(IndexedDatabaseManager::OriginClearRunnable,
                              nsIRunnable)

NS_IMETHODIMP
IndexedDatabaseManager::OriginClearRunnable::Run()
{
  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never fail!");

  if (NS_IsMainThread()) {
    
    if (mFirstCallback) {
      NS_ASSERTION(mThread, "Should have a thread here!");

      mFirstCallback = false;

      nsCOMPtr<nsIThread> thread;
      mThread.swap(thread);

      
      if (NS_FAILED(thread->Dispatch(this, NS_DISPATCH_NORMAL))) {
        NS_WARNING("Failed to dispatch to IO thread!");
        return NS_ERROR_FAILURE;
      }

      return NS_OK;
    }

    NS_ASSERTION(!mThread, "Should have been cleared already!");

    mgr->InvalidateFileManagersForOrigin(mOrigin);

    
    mgr->AllowNextSynchronizedOp(mOrigin, nsnull);

    return NS_OK;
  }

  NS_ASSERTION(!mThread, "Should have been cleared already!");

  
  nsCOMPtr<nsIFile> directory;
  nsresult rv = mgr->GetDirectoryForOrigin(mOrigin, getter_AddRefs(directory));
  if (NS_SUCCEEDED(rv)) {
    bool exists;
    rv = directory->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists) {
      rv = directory->Remove(true);
    }
  }
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to remove directory!");

  
  rv = NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

IndexedDatabaseManager::AsyncUsageRunnable::AsyncUsageRunnable(
                                     nsIURI* aURI,
                                     const nsACString& aOrigin,
                                     nsIIndexedDatabaseUsageCallback* aCallback)
: mURI(aURI),
  mOrigin(aOrigin),
  mCallback(aCallback),
  mUsage(0),
  mFileUsage(0),
  mCanceled(0)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aURI, "Null pointer!");
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");
  NS_ASSERTION(aCallback, "Null pointer!");
}

void
IndexedDatabaseManager::AsyncUsageRunnable::Cancel()
{
  if (PR_ATOMIC_SET(&mCanceled, 1)) {
    NS_ERROR("Canceled more than once?!");
  }
}

inline void
IncrementUsage(PRUint64* aUsage, PRUint64 aDelta)
{
  
  if ((LL_MAXINT - *aUsage) <= aDelta) {
    NS_WARNING("Database sizes exceed max we can report!");
    *aUsage = LL_MAXINT;
  }
  else {
    *aUsage += aDelta;
  }
}

nsresult
IndexedDatabaseManager::AsyncUsageRunnable::RunInternal()
{
  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never fail!");

  if (NS_IsMainThread()) {
    
    if (!mCanceled) {
      PRUint64 usage = mUsage;
      IncrementUsage(&usage, mFileUsage);
      mCallback->OnUsageResult(mURI, usage, mFileUsage);
    }

    
    mURI = nsnull;
    mCallback = nsnull;

    
    mgr->OnUsageCheckComplete(this);

    return NS_OK;
  }

  if (mCanceled) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIFile> directory;
  nsresult rv = mgr->GetDirectoryForOrigin(mOrigin, getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = directory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (exists && !mCanceled) {
    rv = GetUsageForDirectory(directory, &mUsage);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
IndexedDatabaseManager::AsyncUsageRunnable::GetUsageForDirectory(
                                     nsIFile* aDirectory,
                                     PRUint64* aUsage)
{
  NS_ASSERTION(aDirectory, "Null pointer!");
  NS_ASSERTION(aUsage, "Null pointer!");

  nsCOMPtr<nsISimpleEnumerator> entries;
  nsresult rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!entries) {
    return NS_OK;
  }

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) &&
         hasMore && !mCanceled) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> file(do_QueryInterface(entry));
    NS_ASSERTION(file, "Don't know what this is!");

    bool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isDirectory) {
      if (aUsage == &mFileUsage) {
        NS_WARNING("Unknown directory found!");
      }
      else {
        rv = GetUsageForDirectory(file, &mFileUsage);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      continue;
    }

    PRInt64 fileSize;
    rv = file->GetFileSize(&fileSize);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(fileSize > 0, "Negative size?!");

    IncrementUsage(aUsage, PRUint64(fileSize));
  }
  NS_ENSURE_SUCCESS(rv, rv);
 
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(IndexedDatabaseManager::AsyncUsageRunnable,
                              nsIRunnable)

NS_IMETHODIMP
IndexedDatabaseManager::AsyncUsageRunnable::Run()
{
  nsresult rv = RunInternal();

  if (!NS_IsMainThread()) {
    if (NS_FAILED(rv)) {
      mUsage = 0;
    }

    if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(IndexedDatabaseManager::WaitForTransactionsToFinishRunnable,
                              nsIRunnable)

NS_IMETHODIMP
IndexedDatabaseManager::WaitForTransactionsToFinishRunnable::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mOp && mOp->mHelper, "What?");

  
  nsRefPtr<AsyncConnectionHelper> helper;
  helper.swap(mOp->mHelper);

  mOp = nsnull;

  IndexedDatabaseManager::DispatchHelper(helper);

  
  

  return NS_OK;
}


IndexedDatabaseManager::SynchronizedOp::SynchronizedOp(const nsACString& aOrigin,
                                                       nsIAtom* aId)
: mOrigin(aOrigin), mId(aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  MOZ_COUNT_CTOR(IndexedDatabaseManager::SynchronizedOp);
}

IndexedDatabaseManager::SynchronizedOp::~SynchronizedOp()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  MOZ_COUNT_DTOR(IndexedDatabaseManager::SynchronizedOp);
}

bool
IndexedDatabaseManager::SynchronizedOp::MustWaitFor(const SynchronizedOp& aRhs)
  const
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  if (!aRhs.mOrigin.Equals(mOrigin)) {
    return false;
  }

  
  if (aRhs.mId == mId) {
    return true;
  }

  
  
  if (!aRhs.mId || !mId) {
    return true;
  }

  
  
  return false;
}

void
IndexedDatabaseManager::SynchronizedOp::DelayRunnable(nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mDelayedRunnables.IsEmpty() || !mId,
               "Only ClearOrigin operations can delay multiple runnables!");

  mDelayedRunnables.AppendElement(aRunnable);
}

void
IndexedDatabaseManager::SynchronizedOp::DispatchDelayedRunnables()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mHelper, "Any helper should be gone by now!");

  PRUint32 count = mDelayedRunnables.Length();
  for (PRUint32 index = 0; index < count; index++) {
    NS_DispatchToCurrentThread(mDelayedRunnables[index]);
  }

  mDelayedRunnables.Clear();
}

NS_IMETHODIMP
IndexedDatabaseManager::InitWindowless(const jsval& aObj, JSContext* aCx)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_ARG(!JSVAL_IS_PRIMITIVE(aObj));

  
  
  
  nsCOMPtr<nsIDOMScriptObjectFactory> sof(do_GetService(kDOMSOF_CID));

  JSObject* obj = JSVAL_TO_OBJECT(aObj);

  JSObject* global = JS_GetGlobalForObject(aCx, obj);

  nsRefPtr<IDBFactory> factory;
  nsresult rv = IDBFactory::Create(aCx, global, getter_AddRefs(factory));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  NS_ASSERTION(factory, "This should never fail for chrome!");

  jsval mozIndexedDBVal;
  rv = nsContentUtils::WrapNative(aCx, obj, factory, &mozIndexedDBVal);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!JS_DefineProperty(aCx, obj, "mozIndexedDB", mozIndexedDBVal, nsnull,
                         nsnull, JSPROP_ENUMERATE)) {
    return NS_ERROR_FAILURE;
  }

  JSObject* keyrangeObj = JS_NewObject(aCx, nsnull, nsnull, nsnull);
  NS_ENSURE_TRUE(keyrangeObj, NS_ERROR_OUT_OF_MEMORY);

  if (!IDBKeyRange::DefineConstructors(aCx, keyrangeObj)) {
    return NS_ERROR_FAILURE;
  }

  if (!JS_DefineProperty(aCx, obj, "IDBKeyRange", OBJECT_TO_JSVAL(keyrangeObj),
                         nsnull, nsnull, JSPROP_ENUMERATE)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(IndexedDatabaseManager::AsyncDeleteFileRunnable,
                              nsIRunnable)

NS_IMETHODIMP
IndexedDatabaseManager::AsyncDeleteFileRunnable::Run()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  int rc = sqlite3_quota_remove(NS_ConvertUTF16toUTF8(mFilePath).get());
  if (rc != SQLITE_OK) {
    NS_WARNING("Failed to delete stored file!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}
