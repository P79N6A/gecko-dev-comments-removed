





#include "IndexedDatabaseManager.h"

#include "nsIConsoleService.h"
#include "nsIDiskSpaceWatcher.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsIScriptError.h"

#include "jsapi.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/CondVar.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/ErrorEventBinding.h"
#include "mozilla/dom/PBlobChild.h"
#include "mozilla/dom/quota/OriginOrPatternString.h"
#include "mozilla/dom/quota/QuotaManager.h"
#include "mozilla/dom/quota/Utilities.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "mozilla/storage.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"

#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IDBKeyRange.h"
#include "IDBRequest.h"


#include "mozilla/dom/IDBCursorBinding.h"
#include "mozilla/dom/IDBDatabaseBinding.h"
#include "mozilla/dom/IDBFactoryBinding.h"
#include "mozilla/dom/IDBIndexBinding.h"
#include "mozilla/dom/IDBKeyRangeBinding.h"
#include "mozilla/dom/IDBMutableFileBinding.h"
#include "mozilla/dom/IDBObjectStoreBinding.h"
#include "mozilla/dom/IDBOpenDBRequestBinding.h"
#include "mozilla/dom/IDBRequestBinding.h"
#include "mozilla/dom/IDBTransactionBinding.h"
#include "mozilla/dom/IDBVersionChangeEventBinding.h"

#define IDB_STR "indexedDB"



#define LOW_DISK_SPACE_DATA_FULL "full"
#define LOW_DISK_SPACE_DATA_FREE "free"

namespace mozilla {
namespace dom {
namespace indexedDB {

using namespace mozilla::dom::quota;

class FileManagerInfo
{
public:
  already_AddRefed<FileManager>
  GetFileManager(PersistenceType aPersistenceType,
                 const nsAString& aName) const;

  void
  AddFileManager(FileManager* aFileManager);

  bool
  HasFileManagers() const
  {
    AssertIsOnIOThread();

    return !mPersistentStorageFileManagers.IsEmpty() ||
           !mTemporaryStorageFileManagers.IsEmpty();
  }

  void
  InvalidateAllFileManagers() const;

  void
  InvalidateAndRemoveFileManagers(PersistenceType aPersistenceType);

  void
  InvalidateAndRemoveFileManager(PersistenceType aPersistenceType,
                                 const nsAString& aName);

private:
  nsTArray<nsRefPtr<FileManager> >&
  GetArray(PersistenceType aPersistenceType);

  const nsTArray<nsRefPtr<FileManager> >&
  GetImmutableArray(PersistenceType aPersistenceType) const
  {
    return const_cast<FileManagerInfo*>(this)->GetArray(aPersistenceType);
  }

  nsTArray<nsRefPtr<FileManager> > mPersistentStorageFileManagers;
  nsTArray<nsRefPtr<FileManager> > mTemporaryStorageFileManagers;
};

namespace {

const char kTestingPref[] = "dom.indexedDB.testing";

mozilla::StaticRefPtr<IndexedDatabaseManager> gDBManager;

mozilla::Atomic<bool> gInitialized(false);
mozilla::Atomic<bool> gClosed(false);
mozilla::Atomic<bool> gTestingMode(false);

class AsyncDeleteFileRunnable MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteFileRunnable(FileManager* aFileManager, int64_t aFileId);

private:
  ~AsyncDeleteFileRunnable() {}

  nsRefPtr<FileManager> mFileManager;
  int64_t mFileId;
};

class GetFileReferencesHelper MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  GetFileReferencesHelper(PersistenceType aPersistenceType,
                          const nsACString& aOrigin,
                          const nsAString& aDatabaseName,
                          int64_t aFileId)
  : mPersistenceType(aPersistenceType),
    mOrigin(aOrigin),
    mDatabaseName(aDatabaseName),
    mFileId(aFileId),
    mMutex(IndexedDatabaseManager::FileMutex()),
    mCondVar(mMutex, "GetFileReferencesHelper::mCondVar"),
    mMemRefCnt(-1),
    mDBRefCnt(-1),
    mSliceRefCnt(-1),
    mResult(false),
    mWaiting(true)
  { }

  nsresult
  DispatchAndReturnFileReferences(int32_t* aMemRefCnt,
                                  int32_t* aDBRefCnt,
                                  int32_t* aSliceRefCnt,
                                  bool* aResult);

private:
  ~GetFileReferencesHelper() {}

  PersistenceType mPersistenceType;
  nsCString mOrigin;
  nsString mDatabaseName;
  int64_t mFileId;

  mozilla::Mutex& mMutex;
  mozilla::CondVar mCondVar;
  int32_t mMemRefCnt;
  int32_t mDBRefCnt;
  int32_t mSliceRefCnt;
  bool mResult;
  bool mWaiting;
};

struct MOZ_STACK_CLASS InvalidateInfo
{
  InvalidateInfo(PersistenceType aPersistenceType, const nsACString& aPattern)
  : persistenceType(aPersistenceType), pattern(aPattern)
  { }

  PersistenceType persistenceType;
  const nsACString& pattern;
};

void
TestingPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!strcmp(aPrefName, kTestingPref));
  MOZ_ASSERT(!aClosure);

  gTestingMode = Preferences::GetBool(aPrefName);
}

} 

IndexedDatabaseManager::IndexedDatabaseManager()
: mFileMutex("IndexedDatabaseManager.mFileMutex")
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

IndexedDatabaseManager::~IndexedDatabaseManager()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

bool IndexedDatabaseManager::sIsMainProcess = false;
mozilla::Atomic<bool> IndexedDatabaseManager::sLowDiskSpaceMode(false);


IndexedDatabaseManager*
IndexedDatabaseManager::GetOrCreate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (IsClosed()) {
    NS_ERROR("Calling GetOrCreate() after shutdown!");
    return nullptr;
  }

  if (!gDBManager) {
    sIsMainProcess = XRE_GetProcessType() == GeckoProcessType_Default;

    if (sIsMainProcess && Preferences::GetBool("disk_space_watcher.enabled", false)) {
      
      nsCOMPtr<nsIDiskSpaceWatcher> watcher =
        do_GetService(DISKSPACEWATCHER_CONTRACTID);
      if (watcher) {
        bool isDiskFull;
        if (NS_SUCCEEDED(watcher->GetIsDiskFull(&isDiskFull))) {
          sLowDiskSpaceMode = isDiskFull;
        }
        else {
          NS_WARNING("GetIsDiskFull failed!");
        }
      }
      else {
        NS_WARNING("No disk space watcher component available!");
      }
    }

    nsRefPtr<IndexedDatabaseManager> instance(new IndexedDatabaseManager());

    nsresult rv = instance->Init();
    NS_ENSURE_SUCCESS(rv, nullptr);

    if (gInitialized.exchange(true)) {
      NS_ERROR("Initialized more than once?!");
    }

    gDBManager = instance;

    ClearOnShutdown(&gDBManager);
  }

  return gDBManager;
}


IndexedDatabaseManager*
IndexedDatabaseManager::Get()
{
  
  return gDBManager;
}

nsresult
IndexedDatabaseManager::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  QuotaManager* qm = QuotaManager::GetOrCreate();
  NS_ENSURE_STATE(qm);

  
  
  if (sIsMainProcess) {
    
    nsCOMPtr<mozIStorageService> ss =
      do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
    NS_ENSURE_STATE(ss);

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    NS_ENSURE_STATE(obs);

    nsresult rv =
      obs->AddObserver(this, DISKSPACEWATCHER_OBSERVER_TOPIC, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  Preferences::RegisterCallbackAndCall(TestingPrefChangedCallback,
                                       kTestingPref);

  return NS_OK;
}

void
IndexedDatabaseManager::Destroy()
{
  
  
  if (gInitialized && gClosed.exchange(true)) {
    NS_ERROR("Shutdown more than once?!");
  }

  Preferences::UnregisterCallback(TestingPrefChangedCallback, kTestingPref);

  delete this;
}


nsresult
IndexedDatabaseManager::FireWindowOnError(nsPIDOMWindow* aOwner,
                                          EventChainPostVisitor& aVisitor)
{
  NS_ENSURE_TRUE(aVisitor.mDOMEvent, NS_ERROR_UNEXPECTED);
  if (!aOwner) {
    return NS_OK;
  }

  if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault) {
    return NS_OK;
  }

  nsString type;
  nsresult rv = aVisitor.mDOMEvent->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nsDependentString(kErrorEventType) != type) {
    return NS_OK;
  }

  nsCOMPtr<EventTarget> eventTarget =
    aVisitor.mDOMEvent->InternalDOMEvent()->GetTarget();

  IDBRequest* request = static_cast<IDBRequest*>(eventTarget.get());
  NS_ENSURE_TRUE(request, NS_ERROR_UNEXPECTED);

  ErrorResult ret;
  nsRefPtr<DOMError> error = request->GetError(ret);
  if (ret.Failed()) {
    return ret.ErrorCode();
  }

  nsString errorName;
  if (error) {
    error->GetName(errorName);
  }

  ThreadsafeAutoJSContext cx;
  RootedDictionary<ErrorEventInit> init(cx);
  request->FillScriptErrorEvent(init);

  init.mMessage = errorName;
  init.mCancelable = true;
  init.mBubbles = true;

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aOwner));
  NS_ASSERTION(sgo, "How can this happen?!");

  nsEventStatus status = nsEventStatus_eIgnore;
  if (NS_FAILED(sgo->HandleScriptError(init, &status))) {
    NS_WARNING("Failed to dispatch script error event");
    status = nsEventStatus_eIgnore;
  }

  bool preventDefaultCalled = status == nsEventStatus_eConsumeNoDefault;
  if (preventDefaultCalled) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIScriptError> scriptError =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (NS_FAILED(scriptError->InitWithWindowID(errorName,
                                              init.mFilename,
                                              EmptyString(), init.mLineno,
                                              0, 0,
                                              "IndexedDB",
                                              aOwner->WindowID()))) {
    NS_WARNING("Failed to init script error!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIConsoleService> consoleService =
    do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return consoleService->LogMessage(scriptError);
}


bool
IndexedDatabaseManager::TabContextMayAccessOrigin(const TabContext& aContext,
                                                  const nsACString& aOrigin)
{
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");

  
  
  
  nsAutoCString pattern;
  QuotaManager::GetOriginPatternStringMaybeIgnoreBrowser(
                                                aContext.OwnOrContainingAppId(),
                                                aContext.IsBrowserElement(),
                                                pattern);

  return PatternMatchesOrigin(pattern, aOrigin);
}


bool
IndexedDatabaseManager::DefineIndexedDB(JSContext* aCx,
                                        JS::Handle<JSObject*> aGlobal)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(nsContentUtils::IsCallerChrome(), "Only for chrome!");
  MOZ_ASSERT(js::GetObjectClass(aGlobal)->flags & JSCLASS_DOM_GLOBAL,
             "Passed object is not a global object!");

  if (!IDBCursorBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBCursorWithValueBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBDatabaseBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBFactoryBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBIndexBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBKeyRangeBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBMutableFileBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBObjectStoreBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBOpenDBRequestBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBRequestBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBTransactionBinding::GetConstructorObject(aCx, aGlobal) ||
      !IDBVersionChangeEventBinding::GetConstructorObject(aCx, aGlobal))
  {
    return false;
  }

  nsRefPtr<IDBFactory> factory;
  if (NS_FAILED(IDBFactory::CreateForChromeJS(aCx,
                                              aGlobal,
                                              getter_AddRefs(factory)))) {
    return false;
  }

  MOZ_ASSERT(factory, "This should never fail for chrome!");

  JS::Rooted<JS::Value> indexedDB(aCx);
  js::AssertSameCompartment(aCx, aGlobal);
  if (!WrapNewBindingObject(aCx, factory, &indexedDB)) {
    return false;
  }

  return JS_DefineProperty(aCx, aGlobal, IDB_STR, indexedDB, JSPROP_ENUMERATE);
}


bool
IndexedDatabaseManager::IsClosed()
{
  return gClosed;
}

#ifdef DEBUG

bool
IndexedDatabaseManager::IsMainProcess()
{
  NS_ASSERTION(gDBManager,
               "IsMainProcess() called before indexedDB has been initialized!");
  NS_ASSERTION((XRE_GetProcessType() == GeckoProcessType_Default) ==
               sIsMainProcess, "XRE_GetProcessType changed its tune!");
  return sIsMainProcess;
}


bool
IndexedDatabaseManager::InLowDiskSpaceMode()
{
  NS_ASSERTION(gDBManager,
               "InLowDiskSpaceMode() called before indexedDB has been "
               "initialized!");
  return sLowDiskSpaceMode;
}
#endif


bool
IndexedDatabaseManager::InTestingMode()
{
  MOZ_ASSERT(gDBManager,
             "InTestingMode() called before indexedDB has been initialized!");

  return gTestingMode;
}

already_AddRefed<FileManager>
IndexedDatabaseManager::GetFileManager(PersistenceType aPersistenceType,
                                       const nsACString& aOrigin,
                                       const nsAString& aDatabaseName)
{
  AssertIsOnIOThread();

  FileManagerInfo* info;
  if (!mFileManagerInfos.Get(aOrigin, &info)) {
    return nullptr;
  }

  nsRefPtr<FileManager> fileManager =
    info->GetFileManager(aPersistenceType, aDatabaseName);

  return fileManager.forget();
}

void
IndexedDatabaseManager::AddFileManager(FileManager* aFileManager)
{
  AssertIsOnIOThread();
  NS_ASSERTION(aFileManager, "Null file manager!");

  FileManagerInfo* info;
  if (!mFileManagerInfos.Get(aFileManager->Origin(), &info)) {
    info = new FileManagerInfo();
    mFileManagerInfos.Put(aFileManager->Origin(), info);
  }

  info->AddFileManager(aFileManager);
}


PLDHashOperator
IndexedDatabaseManager::InvalidateAndRemoveFileManagers(
                                             const nsACString& aKey,
                                             nsAutoPtr<FileManagerInfo>& aValue,
                                             void* aUserArg)
{
  AssertIsOnIOThread();
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  if (!aUserArg) {
    aValue->InvalidateAllFileManagers();
    return PL_DHASH_REMOVE;
  }

  InvalidateInfo* info = static_cast<InvalidateInfo*>(aUserArg);

  if (PatternMatchesOrigin(info->pattern, aKey)) {
    aValue->InvalidateAndRemoveFileManagers(info->persistenceType);

    if (!aValue->HasFileManagers()) {
      return PL_DHASH_REMOVE;
    }
  }

  return PL_DHASH_NEXT;
}

void
IndexedDatabaseManager::InvalidateAllFileManagers()
{
  AssertIsOnIOThread();

  mFileManagerInfos.Enumerate(InvalidateAndRemoveFileManagers, nullptr);
}

void
IndexedDatabaseManager::InvalidateFileManagers(
                                  PersistenceType aPersistenceType,
                                  const OriginOrPatternString& aOriginOrPattern)
{
  AssertIsOnIOThread();
  NS_ASSERTION(!aOriginOrPattern.IsEmpty(), "Empty pattern!");

  if (aOriginOrPattern.IsOrigin()) {
    FileManagerInfo* info;
    if (!mFileManagerInfos.Get(aOriginOrPattern, &info)) {
      return;
    }

    info->InvalidateAndRemoveFileManagers(aPersistenceType);

    if (!info->HasFileManagers()) {
      mFileManagerInfos.Remove(aOriginOrPattern);
    }
  }
  else {
    InvalidateInfo info(aPersistenceType, aOriginOrPattern);
    mFileManagerInfos.Enumerate(InvalidateAndRemoveFileManagers, &info);
  }
}

void
IndexedDatabaseManager::InvalidateFileManager(PersistenceType aPersistenceType,
                                              const nsACString& aOrigin,
                                              const nsAString& aDatabaseName)
{
  AssertIsOnIOThread();

  FileManagerInfo* info;
  if (!mFileManagerInfos.Get(aOrigin, &info)) {
    return;
  }

  info->InvalidateAndRemoveFileManager(aPersistenceType, aDatabaseName);

  if (!info->HasFileManagers()) {
    mFileManagerInfos.Remove(aOrigin);
  }
}

nsresult
IndexedDatabaseManager::AsyncDeleteFile(FileManager* aFileManager,
                                        int64_t aFileId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aFileManager);

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  
  
  if (quotaManager->IsClearOriginPending(
                             aFileManager->Origin(),
                             Nullable<PersistenceType>(aFileManager->Type()))) {
    return NS_OK;
  }

  nsRefPtr<AsyncDeleteFileRunnable> runnable =
    new AsyncDeleteFileRunnable(aFileManager, aFileId);

  nsresult rv =
    quotaManager->IOThread()->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
IndexedDatabaseManager::BlockAndGetFileReferences(
                                               PersistenceType aPersistenceType,
                                               const nsACString& aOrigin,
                                               const nsAString& aDatabaseName,
                                               int64_t aFileId,
                                               int32_t* aRefCnt,
                                               int32_t* aDBRefCnt,
                                               int32_t* aSliceRefCnt,
                                               bool* aResult)
{
  if (NS_WARN_IF(!InTestingMode())) {
    return NS_ERROR_UNEXPECTED;
  }

  if (IsMainProcess()) {
    nsRefPtr<GetFileReferencesHelper> helper =
      new GetFileReferencesHelper(aPersistenceType, aOrigin, aDatabaseName,
                                  aFileId);

    nsresult rv =
      helper->DispatchAndReturnFileReferences(aRefCnt, aDBRefCnt,
                                              aSliceRefCnt, aResult);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    ContentChild* contentChild = ContentChild::GetSingleton();
    if (NS_WARN_IF(!contentChild)) {
      return NS_ERROR_FAILURE;
    }

    if (!contentChild->SendGetFileReferences(aPersistenceType,
                                             nsCString(aOrigin),
                                             nsString(aDatabaseName),
                                             aFileId,
                                             aRefCnt,
                                             aDBRefCnt,
                                             aSliceRefCnt,
                                             aResult)) {
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

NS_IMPL_ADDREF(IndexedDatabaseManager)
NS_IMPL_RELEASE_WITH_DESTROY(IndexedDatabaseManager, Destroy())
NS_IMPL_QUERY_INTERFACE(IndexedDatabaseManager, nsIObserver)

NS_IMETHODIMP
IndexedDatabaseManager::Observe(nsISupports* aSubject, const char* aTopic,
                                const char16_t* aData)
{
  NS_ASSERTION(IsMainProcess(), "Wrong process!");
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!strcmp(aTopic, DISKSPACEWATCHER_OBSERVER_TOPIC)) {
    NS_ASSERTION(aData, "No data?!");

    const nsDependentString data(aData);

    if (data.EqualsLiteral(LOW_DISK_SPACE_DATA_FULL)) {
      sLowDiskSpaceMode = true;
    }
    else if (data.EqualsLiteral(LOW_DISK_SPACE_DATA_FREE)) {
      sLowDiskSpaceMode = false;
    }
    else {
      NS_NOTREACHED("Unknown data value!");
    }

    return NS_OK;
  }

   NS_NOTREACHED("Unknown topic!");
   return NS_ERROR_UNEXPECTED;
 }

already_AddRefed<FileManager>
FileManagerInfo::GetFileManager(PersistenceType aPersistenceType,
                                const nsAString& aName) const
{
  AssertIsOnIOThread();

  const nsTArray<nsRefPtr<FileManager> >& managers =
    GetImmutableArray(aPersistenceType);

  for (uint32_t i = 0; i < managers.Length(); i++) {
    const nsRefPtr<FileManager>& fileManager = managers[i];

    if (fileManager->DatabaseName() == aName) {
      nsRefPtr<FileManager> result = fileManager;
      return result.forget();
    }
  }

  return nullptr;
}

void
FileManagerInfo::AddFileManager(FileManager* aFileManager)
{
  AssertIsOnIOThread();

  nsTArray<nsRefPtr<FileManager> >& managers = GetArray(aFileManager->Type());

  NS_ASSERTION(!managers.Contains(aFileManager), "Adding more than once?!");

  managers.AppendElement(aFileManager);
}

void
FileManagerInfo::InvalidateAllFileManagers() const
{
  AssertIsOnIOThread();

  uint32_t i;

  for (i = 0; i < mPersistentStorageFileManagers.Length(); i++) {
    mPersistentStorageFileManagers[i]->Invalidate();
  }

  for (i = 0; i < mTemporaryStorageFileManagers.Length(); i++) {
    mTemporaryStorageFileManagers[i]->Invalidate();
  }
}

void
FileManagerInfo::InvalidateAndRemoveFileManagers(
                                               PersistenceType aPersistenceType)
{
  AssertIsOnIOThread();

  nsTArray<nsRefPtr<FileManager > >& managers = GetArray(aPersistenceType);

  for (uint32_t i = 0; i < managers.Length(); i++) {
    managers[i]->Invalidate();
  }

  managers.Clear();
}

void
FileManagerInfo::InvalidateAndRemoveFileManager(
                                               PersistenceType aPersistenceType,
                                               const nsAString& aName)
{
  AssertIsOnIOThread();

  nsTArray<nsRefPtr<FileManager > >& managers = GetArray(aPersistenceType);

  for (uint32_t i = 0; i < managers.Length(); i++) {
    nsRefPtr<FileManager>& fileManager = managers[i];
    if (fileManager->DatabaseName() == aName) {
      fileManager->Invalidate();
      managers.RemoveElementAt(i);
      return;
    }
  }
}

nsTArray<nsRefPtr<FileManager> >&
FileManagerInfo::GetArray(PersistenceType aPersistenceType)
{
  switch (aPersistenceType) {
    case PERSISTENCE_TYPE_PERSISTENT:
      return mPersistentStorageFileManagers;
    case PERSISTENCE_TYPE_TEMPORARY:
      return mTemporaryStorageFileManagers;

    case PERSISTENCE_TYPE_INVALID:
    default:
      MOZ_CRASH("Bad storage type value!");
      return mPersistentStorageFileManagers;
  }
}

AsyncDeleteFileRunnable::AsyncDeleteFileRunnable(FileManager* aFileManager,
                                                 int64_t aFileId)
: mFileManager(aFileManager), mFileId(aFileId)
{
}

NS_IMPL_ISUPPORTS(AsyncDeleteFileRunnable,
                  nsIRunnable)

NS_IMETHODIMP
AsyncDeleteFileRunnable::Run()
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> directory = mFileManager->GetDirectory();
  NS_ENSURE_TRUE(directory, NS_ERROR_FAILURE);

  nsCOMPtr<nsIFile> file = mFileManager->GetFileForId(directory, mFileId);
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  nsresult rv;
  int64_t fileSize;

  if (mFileManager->EnforcingQuota()) {
    rv = file->GetFileSize(&fileSize);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  rv = file->Remove(false);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  if (mFileManager->EnforcingQuota()) {
    QuotaManager* quotaManager = QuotaManager::Get();
    NS_ASSERTION(quotaManager, "Shouldn't be null!");

    quotaManager->DecreaseUsageForOrigin(mFileManager->Type(),
                                         mFileManager->Group(),
                                         mFileManager->Origin(), fileSize);
  }

  directory = mFileManager->GetJournalDirectory();
  NS_ENSURE_TRUE(directory, NS_ERROR_FAILURE);

  file = mFileManager->GetFileForId(directory, mFileId);
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  rv = file->Remove(false);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
GetFileReferencesHelper::DispatchAndReturnFileReferences(int32_t* aMemRefCnt,
                                                         int32_t* aDBRefCnt,
                                                         int32_t* aSliceRefCnt,
                                                         bool* aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  nsresult rv =
    quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MutexAutoLock autolock(mMutex);
  while (mWaiting) {
    mCondVar.Wait();
  }

  *aMemRefCnt = mMemRefCnt;
  *aDBRefCnt = mDBRefCnt;
  *aSliceRefCnt = mSliceRefCnt;
  *aResult = mResult;

  return NS_OK;
}

NS_IMPL_ISUPPORTS(GetFileReferencesHelper,
                  nsIRunnable)

NS_IMETHODIMP
GetFileReferencesHelper::Run()
{
  AssertIsOnIOThread();

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never fail!");

  nsRefPtr<FileManager> fileManager =
    mgr->GetFileManager(mPersistenceType, mOrigin, mDatabaseName);

  if (fileManager) {
    nsRefPtr<FileInfo> fileInfo = fileManager->GetFileInfo(mFileId);

    if (fileInfo) {
      fileInfo->GetReferences(&mMemRefCnt, &mDBRefCnt, &mSliceRefCnt);

      if (mMemRefCnt != -1) {
        
        mMemRefCnt--;
      }

      mResult = true;
    }
  }

  mozilla::MutexAutoLock lock(mMutex);
  NS_ASSERTION(mWaiting, "Huh?!");

  mWaiting = false;
  mCondVar.Notify();

  return NS_OK;
}

} 
} 
} 
