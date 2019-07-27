





#include "QuotaManager.h"

#include "mozIApplicationClearPrivateDataParams.h"
#include "nsIBinaryInputStream.h"
#include "nsIBinaryOutputStream.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsIOfflineStorage.h"
#include "nsIPrincipal.h"
#include "nsIQuotaRequest.h"
#include "nsIRunnable.h"
#include "nsISimpleEnumerator.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsITimer.h"
#include "nsIURI.h"
#include "nsIUsageCallback.h"
#include "nsPIDOMWindow.h"

#include <algorithm>
#include "GeckoProfiler.h"
#include "mozilla/Atomics.h"
#include "mozilla/CondVar.h"
#include "mozilla/dom/asmjscache/AsmJSCache.h"
#include "mozilla/dom/FileService.h"
#include "mozilla/dom/indexedDB/Client.h"
#include "mozilla/Mutex.h"
#include "mozilla/LazyIdleThread.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsComponentManagerUtils.h"
#include "nsAboutProtocolUtils.h"
#include "nsContentUtils.h"
#include "nsCRTGlue.h"
#include "nsDirectoryServiceUtils.h"
#include "nsNetUtil.h"
#include "nsScriptSecurityManager.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "xpcpublic.h"

#include "AcquireListener.h"
#include "CheckQuotaHelper.h"
#include "OriginCollection.h"
#include "OriginOrPatternString.h"
#include "QuotaObject.h"
#include "StorageMatcher.h"
#include "UsageInfo.h"
#include "Utilities.h"



#define DEFAULT_THREAD_TIMEOUT_MS 30000



#define DEFAULT_SHUTDOWN_TIMER_MS 30000


#define PREF_STORAGE_QUOTA "dom.indexedDB.warningQuota"



#define PREF_FIXED_LIMIT "dom.quotaManager.temporaryStorage.fixedLimit"
#define PREF_CHUNK_SIZE "dom.quotaManager.temporaryStorage.chunkSize"


#define PREF_TESTING_FEATURES "dom.quotaManager.testing"


#define PROFILE_BEFORE_CHANGE_OBSERVER_ID "profile-before-change"



#define METADATA_FILE_NAME ".metadata"

#define PERMISSION_DEFAUT_PERSISTENT_STORAGE "default-persistent-storage"

#define KB * 1024ULL
#define MB * 1024ULL KB
#define GB * 1024ULL MB

USING_QUOTA_NAMESPACE
using namespace mozilla::dom;
using mozilla::dom::FileService;

static_assert(
  static_cast<uint32_t>(StorageType::Persistent) ==
  static_cast<uint32_t>(PERSISTENCE_TYPE_PERSISTENT),
  "Enum values should match.");

static_assert(
  static_cast<uint32_t>(StorageType::Temporary) ==
  static_cast<uint32_t>(PERSISTENCE_TYPE_TEMPORARY),
  "Enum values should match.");

BEGIN_QUOTA_NAMESPACE




struct SynchronizedOp
{
  SynchronizedOp(const OriginOrPatternString& aOriginOrPattern,
                 Nullable<PersistenceType> aPersistenceType,
                 const nsACString& aId);

  ~SynchronizedOp();

  
  bool
  MustWaitFor(const SynchronizedOp& aOp);

  void
  DelayRunnable(nsIRunnable* aRunnable);

  void
  DispatchDelayedRunnables();

  const OriginOrPatternString mOriginOrPattern;
  Nullable<PersistenceType> mPersistenceType;
  nsCString mId;
  nsRefPtr<AcquireListener> mListener;
  nsTArray<nsCOMPtr<nsIRunnable> > mDelayedRunnables;
  ArrayCluster<nsIOfflineStorage*> mStorages;
};

class CollectOriginsHelper MOZ_FINAL : public nsRunnable
{
public:
  CollectOriginsHelper(mozilla::Mutex& aMutex, uint64_t aMinSizeToBeFreed);

  NS_IMETHOD
  Run();

  
  
  int64_t
  BlockAndReturnOriginsForEviction(nsTArray<OriginInfo*>& aOriginInfos);

private:
  ~CollectOriginsHelper()
  { }

  uint64_t mMinSizeToBeFreed;

  mozilla::Mutex& mMutex;
  mozilla::CondVar mCondVar;

  
  nsTArray<OriginInfo*> mOriginInfos;
  uint64_t mSizeToBeFreed;
  bool mWaiting;
};









class OriginClearRunnable MOZ_FINAL : public nsRunnable,
                                      public AcquireListener
{
  enum CallbackState {
    
    Pending = 0,

    
    OpenAllowed,

    
    IO,

    
    Complete
  };

public:
  NS_DECL_ISUPPORTS_INHERITED

  OriginClearRunnable(const OriginOrPatternString& aOriginOrPattern,
                      Nullable<PersistenceType> aPersistenceType)
  : mOriginOrPattern(aOriginOrPattern),
    mPersistenceType(aPersistenceType),
    mCallbackState(Pending)
  { }

  NS_IMETHOD
  Run();

  
  virtual nsresult
  OnExclusiveAccessAcquired() MOZ_OVERRIDE;

  void
  AdvanceState()
  {
    switch (mCallbackState) {
      case Pending:
        mCallbackState = OpenAllowed;
        return;
      case OpenAllowed:
        mCallbackState = IO;
        return;
      case IO:
        mCallbackState = Complete;
        return;
      default:
        NS_NOTREACHED("Can't advance past Complete!");
    }
  }

  static void
  InvalidateOpenedStorages(nsTArray<nsCOMPtr<nsIOfflineStorage> >& aStorages,
                           void* aClosure);

  void
  DeleteFiles(QuotaManager* aQuotaManager,
              PersistenceType aPersistenceType);

private:
  ~OriginClearRunnable() {}

  OriginOrPatternString mOriginOrPattern;
  Nullable<PersistenceType> mPersistenceType;
  CallbackState mCallbackState;
};









class AsyncUsageRunnable MOZ_FINAL : public UsageInfo,
                                     public nsRunnable,
                                     public nsIQuotaRequest
{
  enum CallbackState {
    
    Pending = 0,

    
    OpenAllowed,

    
    IO,

    
    Complete,

    
    Shortcut
  };

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIQUOTAREQUEST

  AsyncUsageRunnable(uint32_t aAppId,
                     bool aInMozBrowserOnly,
                     const nsACString& aGroup,
                     const OriginOrPatternString& aOrigin,
                     nsIURI* aURI,
                     nsIUsageCallback* aCallback);

  NS_IMETHOD
  Run();

  void
  AdvanceState()
  {
    switch (mCallbackState) {
      case Pending:
        mCallbackState = OpenAllowed;
        return;
      case OpenAllowed:
        mCallbackState = IO;
        return;
      case IO:
        mCallbackState = Complete;
        return;
      default:
        NS_NOTREACHED("Can't advance past Complete!");
    }
  }

  nsresult
  TakeShortcut();

private:
  ~AsyncUsageRunnable() {}

  
  
  inline nsresult
  RunInternal();

  nsresult
  AddToUsage(QuotaManager* aQuotaManager,
             PersistenceType aPersistenceType);

  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIUsageCallback> mCallback;
  uint32_t mAppId;
  nsCString mGroup;
  OriginOrPatternString mOrigin;
  CallbackState mCallbackState;
  bool mInMozBrowserOnly;
};

class ResetOrClearRunnable MOZ_FINAL : public nsRunnable,
                                       public AcquireListener
{
  enum CallbackState {
    
    Pending = 0,

    
    OpenAllowed,

    
    IO,

    
    Complete
  };

public:
  NS_DECL_ISUPPORTS_INHERITED

  explicit ResetOrClearRunnable(bool aClear)
  : mCallbackState(Pending),
    mClear(aClear)
  { }

  NS_IMETHOD
  Run();

  
  virtual nsresult
  OnExclusiveAccessAcquired() MOZ_OVERRIDE;

  void
  AdvanceState()
  {
    switch (mCallbackState) {
      case Pending:
        mCallbackState = OpenAllowed;
        return;
      case OpenAllowed:
        mCallbackState = IO;
        return;
      case IO:
        mCallbackState = Complete;
        return;
      default:
        NS_NOTREACHED("Can't advance past Complete!");
    }
  }

  static void
  InvalidateOpenedStorages(nsTArray<nsCOMPtr<nsIOfflineStorage> >& aStorages,
                           void* aClosure);

  void
  DeleteFiles(QuotaManager* aQuotaManager,
              PersistenceType aPersistenceType);

private:
  ~ResetOrClearRunnable() {}

  CallbackState mCallbackState;
  bool mClear;
};










class FinalizeOriginEvictionRunnable MOZ_FINAL : public nsRunnable
{
  enum CallbackState {
    
    Pending = 0,

    
    OpenAllowed,

    
    IO,

    
    Complete
  };

public:
  explicit FinalizeOriginEvictionRunnable(nsTArray<nsCString>& aOrigins)
  : mCallbackState(Pending)
  {
    mOrigins.SwapElements(aOrigins);
  }

  NS_IMETHOD
  Run();

  void
  AdvanceState()
  {
    switch (mCallbackState) {
      case Pending:
        mCallbackState = OpenAllowed;
        return;
      case OpenAllowed:
        mCallbackState = IO;
        return;
      case IO:
        mCallbackState = Complete;
        return;
      default:
        MOZ_ASSERT_UNREACHABLE("Can't advance past Complete!");
    }
  }

  nsresult
  Dispatch();

  nsresult
  RunImmediately();

private:
  CallbackState mCallbackState;
  nsTArray<nsCString> mOrigins;
};

bool
IsOnIOThread()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Must have a manager here!");

  bool currentThread;
  return NS_SUCCEEDED(quotaManager->IOThread()->
                      IsOnCurrentThread(&currentThread)) && currentThread;
}

void
AssertIsOnIOThread()
{
  NS_ASSERTION(IsOnIOThread(), "Running on the wrong thread!");
}

void
AssertCurrentThreadOwnsQuotaMutex()
{
#ifdef DEBUG
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Must have a manager here!");

  quotaManager->AssertCurrentThreadOwnsQuotaMutex();
#endif
}

END_QUOTA_NAMESPACE

namespace {


static const int32_t  kDefaultQuotaMB =             50;


QuotaManager* gInstance = nullptr;
mozilla::Atomic<bool> gShutdown(false);

int32_t gStorageQuotaMB = kDefaultQuotaMB;


static const int32_t kDefaultFixedLimitKB = -1;
static const uint32_t kDefaultChunkSizeKB = 10 * 1024;
int32_t gFixedLimitKB = kDefaultFixedLimitKB;
uint32_t gChunkSizeKB = kDefaultChunkSizeKB;

bool gTestingEnabled = false;



class WaitForTransactionsToFinishRunnable MOZ_FINAL : public nsRunnable
{
public:
  explicit WaitForTransactionsToFinishRunnable(SynchronizedOp* aOp)
  : mOp(aOp), mCountdown(1)
  {
    NS_ASSERTION(mOp, "Why don't we have a runnable?");
    NS_ASSERTION(mOp->mStorages.IsEmpty(), "We're here too early!");
    NS_ASSERTION(mOp->mListener,
                 "What are we supposed to do when we're done?");
    NS_ASSERTION(mCountdown, "Wrong countdown!");
  }

  NS_IMETHOD
  Run();

  void
  AddRun()
  {
    mCountdown++;
  }

private:
  
  SynchronizedOp* mOp;
  uint32_t mCountdown;
};

class WaitForFileHandlesToFinishRunnable MOZ_FINAL : public nsRunnable
{
public:
  WaitForFileHandlesToFinishRunnable()
  : mBusy(true)
  { }

  NS_IMETHOD
  Run();

  bool
  IsBusy() const
  {
    return mBusy;
  }

private:
  bool mBusy;
};

class SaveOriginAccessTimeRunnable MOZ_FINAL : public nsRunnable
{
public:
  SaveOriginAccessTimeRunnable(const nsACString& aOrigin, int64_t aTimestamp)
  : mOrigin(aOrigin), mTimestamp(aTimestamp)
  { }

  NS_IMETHOD
  Run();

private:
  nsCString mOrigin;
  int64_t mTimestamp;
};

struct MOZ_STACK_CLASS RemoveQuotaInfo
{
  RemoveQuotaInfo(PersistenceType aPersistenceType, const nsACString& aPattern)
  : persistenceType(aPersistenceType), pattern(aPattern)
  { }

  PersistenceType persistenceType;
  nsCString pattern;
};

struct MOZ_STACK_CLASS InactiveOriginsInfo
{
  InactiveOriginsInfo(OriginCollection& aCollection,
                      nsTArray<OriginInfo*>& aOrigins)
  : collection(aCollection), origins(aOrigins)
  { }

  OriginCollection& collection;
  nsTArray<OriginInfo*>& origins;
};

bool
IsMainProcess()
{
  return XRE_GetProcessType() == GeckoProcessType_Default;
}

void
SanitizeOriginString(nsCString& aOrigin)
{
  
  
  
  
  static const char kReplaceChars[] = CONTROL_CHARACTERS "/:*?\"<>|\\";

#ifdef XP_WIN
  NS_ASSERTION(!strcmp(kReplaceChars,
                       FILE_ILLEGAL_CHARACTERS FILE_PATH_SEPARATOR),
               "Illegal file characters have changed!");
#endif

  aOrigin.ReplaceChar(kReplaceChars, '+');
}

nsresult
EnsureDirectory(nsIFile* aDirectory, bool* aCreated)
{
  AssertIsOnIOThread();

  nsresult rv = aDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
  if (rv == NS_ERROR_FILE_ALREADY_EXISTS) {
    bool isDirectory;
    rv = aDirectory->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(isDirectory, NS_ERROR_UNEXPECTED);

    *aCreated = false;
  }
  else {
    NS_ENSURE_SUCCESS(rv, rv);

    *aCreated = true;
  }

  return NS_OK;
}

nsresult
CreateDirectoryUpgradeStamp(nsIFile* aDirectory)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> metadataFile;
  nsresult rv = aDirectory->Clone(getter_AddRefs(metadataFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = metadataFile->Append(NS_LITERAL_STRING(METADATA_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = metadataFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
GetDirectoryMetadataStream(nsIFile* aDirectory, bool aUpdate,
                           nsIBinaryOutputStream** aStream)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> metadataFile;
  nsresult rv = aDirectory->Clone(getter_AddRefs(metadataFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = metadataFile->Append(NS_LITERAL_STRING(METADATA_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> outputStream;
  if (aUpdate) {
    bool exists;
    rv = metadataFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!exists) {
      *aStream = nullptr;
      return NS_OK;
    }

    nsCOMPtr<nsIFileStream> stream;
    rv = NS_NewLocalFileStream(getter_AddRefs(stream), metadataFile);
    NS_ENSURE_SUCCESS(rv, rv);

    outputStream = do_QueryInterface(stream);
    NS_ENSURE_TRUE(outputStream, NS_ERROR_FAILURE);
  }
  else {
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream),
                                     metadataFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIBinaryOutputStream> binaryStream =
    do_CreateInstance("@mozilla.org/binaryoutputstream;1");
  NS_ENSURE_TRUE(binaryStream, NS_ERROR_FAILURE);

  rv = binaryStream->SetOutputStream(outputStream);
  NS_ENSURE_SUCCESS(rv, rv);

  binaryStream.forget(aStream);
  return NS_OK;
}

nsresult
CreateDirectoryMetadata(nsIFile* aDirectory, int64_t aTimestamp,
                        const nsACString& aGroup, const nsACString& aOrigin)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIBinaryOutputStream> stream;
  nsresult rv =
    GetDirectoryMetadataStream(aDirectory, false, getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(stream, "This shouldn't be null!");

  rv = stream->Write64(aTimestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->WriteStringZ(PromiseFlatCString(aGroup).get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->WriteStringZ(PromiseFlatCString(aOrigin).get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
GetDirectoryMetadata(nsIFile* aDirectory, int64_t* aTimestamp,
                     nsACString& aGroup, nsACString& aOrigin)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> metadataFile;
  nsresult rv = aDirectory->Clone(getter_AddRefs(metadataFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = metadataFile->Append(NS_LITERAL_STRING(METADATA_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> stream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(stream), metadataFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> bufferedStream;
  rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream), stream, 512);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBinaryInputStream> binaryStream =
    do_CreateInstance("@mozilla.org/binaryinputstream;1");
  NS_ENSURE_TRUE(binaryStream, NS_ERROR_FAILURE);

  rv = binaryStream->SetInputStream(bufferedStream);
  NS_ENSURE_SUCCESS(rv, rv);

  uint64_t timestamp;
  rv = binaryStream->Read64(&timestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString group;
  rv = binaryStream->ReadCString(group);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString origin;
  rv = binaryStream->ReadCString(origin);
  NS_ENSURE_SUCCESS(rv, rv);

  *aTimestamp = timestamp;
  aGroup = group;
  aOrigin = origin;
  return NS_OK;
}

nsresult
MaybeUpgradeOriginDirectory(nsIFile* aDirectory)
{
  AssertIsOnIOThread();
  NS_ASSERTION(aDirectory, "Null pointer!");

  nsCOMPtr<nsIFile> metadataFile;
  nsresult rv = aDirectory->Clone(getter_AddRefs(metadataFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = metadataFile->Append(NS_LITERAL_STRING(METADATA_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = metadataFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    
    

    nsString idbDirectoryName;
    rv = Client::TypeToText(Client::IDB, idbDirectoryName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> idbDirectory;
    rv = aDirectory->Clone(getter_AddRefs(idbDirectory));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = idbDirectory->Append(idbDirectoryName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = idbDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
    if (rv == NS_ERROR_FILE_ALREADY_EXISTS) {
      NS_WARNING("IDB directory already exists!");

      bool isDirectory;
      rv = idbDirectory->IsDirectory(&isDirectory);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(isDirectory, NS_ERROR_UNEXPECTED);
    }
    else {
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
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

      if (!leafName.Equals(idbDirectoryName)) {
        rv = file->MoveTo(idbDirectory, EmptyString());
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    rv = metadataFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}




nsresult
GetTemporaryStorageLimit(nsIFile* aDirectory, uint64_t aCurrentUsage,
                         uint64_t* aLimit)
{
  
  int64_t bytesAvailable;
  nsresult rv = aDirectory->GetDiskSpaceAvailable(&bytesAvailable);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(bytesAvailable >= 0, "Negative bytes available?!");

  uint64_t availableKB =
    static_cast<uint64_t>((bytesAvailable + aCurrentUsage) / 1024);

  
  
  
  availableKB = (availableKB / gChunkSizeKB) * gChunkSizeKB;

  
  uint64_t resultKB = availableKB * .50;

  *aLimit = resultKB * 1024;
  return NS_OK;
}

} 

QuotaManager::QuotaManager()
: mCurrentWindowIndex(BAD_TLS_INDEX),
  mQuotaMutex("QuotaManager.mQuotaMutex"),
  mTemporaryStorageLimit(0),
  mTemporaryStorageUsage(0),
  mTemporaryStorageInitialized(false),
  mStorageAreaInitialized(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance, "More than one instance!");
}

QuotaManager::~QuotaManager()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance || gInstance == this, "Different instances!");
  gInstance = nullptr;
}


QuotaManager*
QuotaManager::GetOrCreate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (IsShuttingDown()) {
    NS_ERROR("Calling GetOrCreate() after shutdown!");
    return nullptr;
  }

  if (!gInstance) {
    nsRefPtr<QuotaManager> instance(new QuotaManager());

    nsresult rv = instance->Init();
    NS_ENSURE_SUCCESS(rv, nullptr);

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    NS_ENSURE_TRUE(obs, nullptr);

    
    rv = obs->AddObserver(instance, PROFILE_BEFORE_CHANGE_OBSERVER_ID, false);
    NS_ENSURE_SUCCESS(rv, nullptr);

    
    gInstance = instance;
  }

  return gInstance;
}


QuotaManager*
QuotaManager::Get()
{
  
  return gInstance;
}


QuotaManager*
QuotaManager::FactoryCreate()
{
  
  
  QuotaManager* quotaManager = GetOrCreate();
  NS_IF_ADDREF(quotaManager);
  return quotaManager;
}


bool
QuotaManager::IsShuttingDown()
{
  return gShutdown;
}

nsresult
QuotaManager::Init()
{
  
  NS_ASSERTION(mCurrentWindowIndex == BAD_TLS_INDEX, "Huh?");

  if (PR_NewThreadPrivateIndex(&mCurrentWindowIndex, nullptr) != PR_SUCCESS) {
    NS_ERROR("PR_NewThreadPrivateIndex failed, QuotaManager disabled");
    mCurrentWindowIndex = BAD_TLS_INDEX;
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  if (IsMainProcess()) {
    nsCOMPtr<nsIFile> baseDir;
    rv = NS_GetSpecialDirectory(NS_APP_INDEXEDDB_PARENT_DIR,
                                getter_AddRefs(baseDir));
    if (NS_FAILED(rv)) {
      rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                  getter_AddRefs(baseDir));
    }
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> indexedDBDir;
    rv = baseDir->Clone(getter_AddRefs(indexedDBDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = indexedDBDir->Append(NS_LITERAL_STRING("indexedDB"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = indexedDBDir->GetPath(mIndexedDBPath);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = baseDir->Append(NS_LITERAL_STRING("storage"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> persistentStorageDir;
    rv = baseDir->Clone(getter_AddRefs(persistentStorageDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = persistentStorageDir->Append(NS_LITERAL_STRING("persistent"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = persistentStorageDir->GetPath(mPersistentStoragePath);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> temporaryStorageDir;
    rv = baseDir->Clone(getter_AddRefs(temporaryStorageDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = temporaryStorageDir->Append(NS_LITERAL_STRING("temporary"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = temporaryStorageDir->GetPath(mTemporaryStoragePath);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    mIOThread = new LazyIdleThread(DEFAULT_THREAD_TIMEOUT_MS,
                                   NS_LITERAL_CSTRING("Storage I/O"),
                                   LazyIdleThread::ManualShutdown);

    
    
    mShutdownTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    NS_ENSURE_TRUE(mShutdownTimer, NS_ERROR_FAILURE);
  }

  if (NS_FAILED(Preferences::AddIntVarCache(&gStorageQuotaMB,
                                            PREF_STORAGE_QUOTA,
                                            kDefaultQuotaMB))) {
    NS_WARNING("Unable to respond to quota pref changes!");
  }

  if (NS_FAILED(Preferences::AddIntVarCache(&gFixedLimitKB, PREF_FIXED_LIMIT,
                                            kDefaultFixedLimitKB)) ||
      NS_FAILED(Preferences::AddUintVarCache(&gChunkSizeKB,
                                             PREF_CHUNK_SIZE,
                                             kDefaultChunkSizeKB))) {
    NS_WARNING("Unable to respond to temp storage pref changes!");
  }

  if (NS_FAILED(Preferences::AddBoolVarCache(&gTestingEnabled,
                                             PREF_TESTING_FEATURES, false))) {
    NS_WARNING("Unable to respond to testing pref changes!");
  }

  static_assert(Client::IDB == 0 && Client::ASMJS == 1 && Client::TYPE_MAX == 2,
                "Fix the registration!");

  NS_ASSERTION(mClients.Capacity() == Client::TYPE_MAX,
               "Should be using an auto array with correct capacity!");

  
  mClients.AppendElement(new indexedDB::Client());
  mClients.AppendElement(asmjscache::CreateClient());

  return NS_OK;
}

void
QuotaManager::InitQuotaForOrigin(PersistenceType aPersistenceType,
                                 const nsACString& aGroup,
                                 const nsACString& aOrigin,
                                 uint64_t aLimitBytes,
                                 uint64_t aUsageBytes,
                                 int64_t aAccessTime)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aLimitBytes > 0 ||
             aPersistenceType == PERSISTENCE_TYPE_TEMPORARY);
  MOZ_ASSERT(aUsageBytes <= aLimitBytes ||
             aPersistenceType == PERSISTENCE_TYPE_TEMPORARY);

  MutexAutoLock lock(mQuotaMutex);

  GroupInfoPair* pair;
  if (!mGroupInfoPairs.Get(aGroup, &pair)) {
    pair = new GroupInfoPair();
    mGroupInfoPairs.Put(aGroup, pair);
    
  }

  nsRefPtr<GroupInfo> groupInfo = pair->LockedGetGroupInfo(aPersistenceType);
  if (!groupInfo) {
    groupInfo = new GroupInfo(aPersistenceType, aGroup);
    pair->LockedSetGroupInfo(groupInfo);
  }

  nsRefPtr<OriginInfo> originInfo =
    new OriginInfo(groupInfo, aOrigin, aLimitBytes, aUsageBytes, aAccessTime);
  groupInfo->LockedAddOriginInfo(originInfo);
}

void
QuotaManager::DecreaseUsageForOrigin(PersistenceType aPersistenceType,
                                     const nsACString& aGroup,
                                     const nsACString& aOrigin,
                                     int64_t aSize)
{
  AssertIsOnIOThread();

  MutexAutoLock lock(mQuotaMutex);

  GroupInfoPair* pair;
  if (!mGroupInfoPairs.Get(aGroup, &pair)) {
    return;
  }

  nsRefPtr<GroupInfo> groupInfo = pair->LockedGetGroupInfo(aPersistenceType);
  if (!groupInfo) {
    return;
  }

  nsRefPtr<OriginInfo> originInfo = groupInfo->LockedGetOriginInfo(aOrigin);
  if (originInfo) {
    originInfo->LockedDecreaseUsage(aSize);
  }
}

void
QuotaManager::UpdateOriginAccessTime(PersistenceType aPersistenceType,
                                     const nsACString& aGroup,
                                     const nsACString& aOrigin)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  MutexAutoLock lock(mQuotaMutex);

  GroupInfoPair* pair;
  if (!mGroupInfoPairs.Get(aGroup, &pair)) {
    return;
  }

  nsRefPtr<GroupInfo> groupInfo =
    pair->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
  if (!groupInfo) {
    return;
  }

  nsRefPtr<OriginInfo> originInfo = groupInfo->LockedGetOriginInfo(aOrigin);
  if (originInfo) {
    int64_t timestamp = PR_Now();
    originInfo->LockedUpdateAccessTime(timestamp);

    if (!groupInfo->IsForTemporaryStorage()) {
      return;
    }

    MutexAutoUnlock autoUnlock(mQuotaMutex);

    SaveOriginAccessTime(aOrigin, timestamp);
  }
}


PLDHashOperator
QuotaManager::RemoveQuotaCallback(const nsACString& aKey,
                                  nsAutoPtr<GroupInfoPair>& aValue,
                                  void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  nsRefPtr<GroupInfo> groupInfo =
    aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
  if (groupInfo) {
    groupInfo->LockedRemoveOriginInfos();
  }

  return PL_DHASH_REMOVE;
}

void
QuotaManager::RemoveQuota()
{
  MutexAutoLock lock(mQuotaMutex);

  mGroupInfoPairs.Enumerate(RemoveQuotaCallback, nullptr);

  NS_ASSERTION(mTemporaryStorageUsage == 0, "Should be zero!");
}


PLDHashOperator
QuotaManager::RemoveQuotaForPersistenceTypeCallback(
                                               const nsACString& aKey,
                                               nsAutoPtr<GroupInfoPair>& aValue,
                                               void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");
  NS_ASSERTION(aUserArg, "Null pointer!");

  PersistenceType& persistenceType = *static_cast<PersistenceType*>(aUserArg);

  if (persistenceType == PERSISTENCE_TYPE_TEMPORARY) {
    nsRefPtr<GroupInfo> groupInfo =
      aValue->LockedGetGroupInfo(persistenceType);
    if (groupInfo) {
      groupInfo->LockedRemoveOriginInfos();
    }
  }

  aValue->LockedClearGroupInfo(persistenceType);

  return aValue->LockedHasGroupInfos() ? PL_DHASH_NEXT : PL_DHASH_REMOVE;
}

void
QuotaManager::RemoveQuotaForPersistenceType(PersistenceType aPersistenceType)
{
  MutexAutoLock lock(mQuotaMutex);

  mGroupInfoPairs.Enumerate(RemoveQuotaForPersistenceTypeCallback,
                            &aPersistenceType);

  NS_ASSERTION(aPersistenceType == PERSISTENCE_TYPE_PERSISTENT ||
               mTemporaryStorageUsage == 0, "Should be zero!");
}


PLDHashOperator
QuotaManager::RemoveQuotaForPatternCallback(const nsACString& aKey,
                                            nsAutoPtr<GroupInfoPair>& aValue,
                                            void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");
  NS_ASSERTION(aUserArg, "Null pointer!");

  RemoveQuotaInfo* info = static_cast<RemoveQuotaInfo*>(aUserArg);

  nsRefPtr<GroupInfo> groupInfo =
    aValue->LockedGetGroupInfo(info->persistenceType);
  if (groupInfo) {
    groupInfo->LockedRemoveOriginInfosForPattern(info->pattern);

    if (!groupInfo->LockedHasOriginInfos()) {
      aValue->LockedClearGroupInfo(info->persistenceType);

      if (!aValue->LockedHasGroupInfos()) {
        return PL_DHASH_REMOVE;
      }
    }
  }

  return PL_DHASH_NEXT;
}

void
QuotaManager::RemoveQuotaForPattern(PersistenceType aPersistenceType,
                                    const nsACString& aPattern)
{
  NS_ASSERTION(!aPattern.IsEmpty(), "Empty pattern!");

  RemoveQuotaInfo info(aPersistenceType, aPattern);

  MutexAutoLock lock(mQuotaMutex);

  mGroupInfoPairs.Enumerate(RemoveQuotaForPatternCallback, &info);
}

already_AddRefed<QuotaObject>
QuotaManager::GetQuotaObject(PersistenceType aPersistenceType,
                             const nsACString& aGroup,
                             const nsACString& aOrigin,
                             nsIFile* aFile)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsString path;
  nsresult rv = aFile->GetPath(path);
  NS_ENSURE_SUCCESS(rv, nullptr);

  int64_t fileSize;

  bool exists;
  rv = aFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, nullptr);

  if (exists) {
    rv = aFile->GetFileSize(&fileSize);
    NS_ENSURE_SUCCESS(rv, nullptr);
  }
  else {
    fileSize = 0;
  }

  nsRefPtr<QuotaObject> result;
  {
    MutexAutoLock lock(mQuotaMutex);

    GroupInfoPair* pair;
    if (!mGroupInfoPairs.Get(aGroup, &pair)) {
      return nullptr;
    }

    nsRefPtr<GroupInfo> groupInfo = pair->LockedGetGroupInfo(aPersistenceType);

    if (!groupInfo) {
      return nullptr;
    }

    nsRefPtr<OriginInfo> originInfo = groupInfo->LockedGetOriginInfo(aOrigin);

    if (!originInfo) {
      return nullptr;
    }

    
    
    
    QuotaObject* quotaObject;
    if (!originInfo->mQuotaObjects.Get(path, &quotaObject)) {
      
      quotaObject = new QuotaObject(originInfo, path, fileSize);

      
      
      originInfo->mQuotaObjects.Put(path, quotaObject);
    }

    
    
    result = quotaObject->LockedAddRef();
  }

  
  
  return result.forget();
}

already_AddRefed<QuotaObject>
QuotaManager::GetQuotaObject(PersistenceType aPersistenceType,
                             const nsACString& aGroup,
                             const nsACString& aOrigin,
                             const nsAString& aPath)
{
  nsresult rv;
  nsCOMPtr<nsIFile> file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nullptr);

  rv = file->InitWithPath(aPath);
  NS_ENSURE_SUCCESS(rv, nullptr);

  return GetQuotaObject(aPersistenceType, aGroup, aOrigin, file);
}

bool
QuotaManager::RegisterStorage(nsIOfflineStorage* aStorage)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aStorage, "Null pointer!");

  
  if (IsShuttingDown()) {
    return false;
  }

  
  const nsACString& origin = aStorage->Origin();
  ArrayCluster<nsIOfflineStorage*>* cluster;
  if (!mLiveStorages.Get(origin, &cluster)) {
    cluster = new ArrayCluster<nsIOfflineStorage*>();
    mLiveStorages.Put(origin, cluster);

    UpdateOriginAccessTime(aStorage->Type(), aStorage->Group(), origin);
  }
  (*cluster)[aStorage->GetClient()->GetType()].AppendElement(aStorage);

  return true;
}

void
QuotaManager::UnregisterStorage(nsIOfflineStorage* aStorage)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aStorage, "Null pointer!");

  
  
  const nsACString& origin = aStorage->Origin();
  ArrayCluster<nsIOfflineStorage*>* cluster;
  if (mLiveStorages.Get(origin, &cluster) &&
      (*cluster)[aStorage->GetClient()->GetType()].RemoveElement(aStorage)) {
    if (cluster->IsEmpty()) {
      mLiveStorages.Remove(origin);

      UpdateOriginAccessTime(aStorage->Type(), aStorage->Group(), origin);
    }
    return;
  }
  NS_ERROR("Didn't know anything about this storage!");
}

void
QuotaManager::OnStorageClosed(nsIOfflineStorage* aStorage)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aStorage, "Null pointer!");

  
  
  SynchronizedOp* op =
    FindSynchronizedOp(aStorage->Origin(),
                       Nullable<PersistenceType>(aStorage->Type()),
                       aStorage->Id());
  if (op) {
    Client::Type clientType = aStorage->GetClient()->GetType();

    
    
    if (op->mStorages[clientType].RemoveElement(aStorage)) {
      
      NS_ASSERTION(op->mListener,
                   "How did we get rid of the listener before removing the "
                    "last storage?");
      if (op->mStorages[clientType].IsEmpty()) {
        
        
        
        
        if (NS_FAILED(RunSynchronizedOp(aStorage, op))) {
          NS_WARNING("Failed to run synchronized op!");
        }
      }
    }
  }
}

void
QuotaManager::AbortCloseStoragesForWindow(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWindow, "Null pointer!");

  FileService* service = FileService::Get();

  StorageMatcher<ArrayCluster<nsIOfflineStorage*> > liveStorages;
  liveStorages.Find(mLiveStorages);

  for (uint32_t i = 0; i < Client::TYPE_MAX; i++) {
    nsRefPtr<Client>& client = mClients[i];
    bool utilized = service && client->IsFileServiceUtilized();
    bool activated = client->IsTransactionServiceActivated();

    nsTArray<nsIOfflineStorage*>& array = liveStorages[i];
    for (uint32_t j = 0; j < array.Length(); j++) {
      nsIOfflineStorage*& storage = array[j];

      if (storage->IsOwned(aWindow)) {
        if (NS_FAILED(storage->Close())) {
          NS_WARNING("Failed to close storage for dying window!");
        }

        if (utilized) {
          service->AbortFileHandlesForStorage(storage);
        }

        if (activated) {
          client->AbortTransactionsForStorage(storage);
        }
      }
    }
  }
}

bool
QuotaManager::HasOpenTransactions(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWindow, "Null pointer!");

  FileService* service = FileService::Get();

  nsAutoPtr<StorageMatcher<ArrayCluster<nsIOfflineStorage*> > > liveStorages;

  for (uint32_t i = 0; i < Client::TYPE_MAX; i++) {
    nsRefPtr<Client>& client = mClients[i];
    bool utilized = service && client->IsFileServiceUtilized();
    bool activated = client->IsTransactionServiceActivated();

    if (utilized || activated) {
      if (!liveStorages) {
        liveStorages = new StorageMatcher<ArrayCluster<nsIOfflineStorage*> >();
        liveStorages->Find(mLiveStorages);
      }

      nsTArray<nsIOfflineStorage*>& storages = liveStorages->ArrayAt(i);
      for (uint32_t j = 0; j < storages.Length(); j++) {
        nsIOfflineStorage*& storage = storages[j];

        if (storage->IsOwned(aWindow) &&
            ((utilized && service->HasFileHandlesForStorage(storage)) ||
             (activated && client->HasTransactionsForStorage(storage)))) {
          return true;
        }
      }
    }
  }

  return false;
}

nsresult
QuotaManager::WaitForOpenAllowed(const OriginOrPatternString& aOriginOrPattern,
                                 Nullable<PersistenceType> aPersistenceType,
                                 const nsACString& aId, nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aOriginOrPattern.IsEmpty() || aOriginOrPattern.IsNull(),
               "Empty pattern!");
  NS_ASSERTION(aRunnable, "Null pointer!");

  nsAutoPtr<SynchronizedOp> op(new SynchronizedOp(aOriginOrPattern,
                                                  aPersistenceType, aId));

  
  bool delayed = false;
  for (uint32_t index = mSynchronizedOps.Length(); index > 0; index--) {
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
QuotaManager::AddSynchronizedOp(const OriginOrPatternString& aOriginOrPattern,
                                Nullable<PersistenceType> aPersistenceType)
{
  nsAutoPtr<SynchronizedOp> op(new SynchronizedOp(aOriginOrPattern,
                                                  aPersistenceType,
                                                  EmptyCString()));

#ifdef DEBUG
  for (uint32_t index = mSynchronizedOps.Length(); index > 0; index--) {
    nsAutoPtr<SynchronizedOp>& existingOp = mSynchronizedOps[index - 1];
    NS_ASSERTION(!op->MustWaitFor(*existingOp), "What?");
  }
#endif

  mSynchronizedOps.AppendElement(op.forget());
}

void
QuotaManager::AllowNextSynchronizedOp(
                                  const OriginOrPatternString& aOriginOrPattern,
                                  Nullable<PersistenceType> aPersistenceType,
                                  const nsACString& aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aOriginOrPattern.IsEmpty() || aOriginOrPattern.IsNull(),
               "Empty origin/pattern!");

  uint32_t count = mSynchronizedOps.Length();
  for (uint32_t index = 0; index < count; index++) {
    nsAutoPtr<SynchronizedOp>& op = mSynchronizedOps[index];
    if (op->mOriginOrPattern.IsOrigin() == aOriginOrPattern.IsOrigin() &&
        op->mOriginOrPattern == aOriginOrPattern &&
        op->mPersistenceType == aPersistenceType) {
      if (op->mId == aId) {
        NS_ASSERTION(op->mStorages.IsEmpty(), "How did this happen?");

        op->DispatchDelayedRunnables();

        mSynchronizedOps.RemoveElementAt(index);
        return;
      }

      
      
      NS_ASSERTION(!op->mId.IsEmpty() && !aId.IsEmpty(),
                   "Why didn't we match earlier?");
    }
  }

  NS_NOTREACHED("Why didn't we find a SynchronizedOp?");
}

nsresult
QuotaManager::GetDirectoryForOrigin(PersistenceType aPersistenceType,
                                    const nsACString& aASCIIOrigin,
                                    nsIFile** aDirectory) const
{
  nsresult rv;
  nsCOMPtr<nsIFile> directory =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = directory->InitWithPath(GetStoragePath(aPersistenceType));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString originSanitized(aASCIIOrigin);
  SanitizeOriginString(originSanitized);

  rv = directory->Append(NS_ConvertASCIItoUTF16(originSanitized));
  NS_ENSURE_SUCCESS(rv, rv);

  directory.forget(aDirectory);
  return NS_OK;
}

nsresult
QuotaManager::InitializeOrigin(PersistenceType aPersistenceType,
                               const nsACString& aGroup,
                               const nsACString& aOrigin,
                               bool aTrackQuota,
                               int64_t aAccessTime,
                               nsIFile* aDirectory)
{
  AssertIsOnIOThread();

  nsresult rv;

  bool temporaryStorage = aPersistenceType == PERSISTENCE_TYPE_TEMPORARY;
  if (!temporaryStorage) {
    rv = MaybeUpgradeOriginDirectory(aDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  nsAutoPtr<UsageInfo> usageInfo;
  if (aTrackQuota) {
    usageInfo = new UsageInfo();
  }

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
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

    if (leafName.EqualsLiteral(METADATA_FILE_NAME) ||
        leafName.EqualsLiteral(DSSTORE_FILE_NAME)) {
      continue;
    }

    bool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!isDirectory) {
      NS_WARNING("Unknown file found!");
      return NS_ERROR_UNEXPECTED;
    }

    Client::Type clientType;
    rv = Client::TypeFromText(leafName, clientType);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unknown directory found!");
      return NS_ERROR_UNEXPECTED;
    }

    rv = mClients[clientType]->InitOrigin(aPersistenceType, aGroup, aOrigin,
                                          usageInfo);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aTrackQuota) {
    uint64_t quotaMaxBytes;
    uint64_t totalUsageBytes = usageInfo->TotalUsage();

    if (temporaryStorage) {
      
      
      quotaMaxBytes = 0;
    }
    else {
      quotaMaxBytes = GetStorageQuotaMB() * 1024 * 1024;
      if (totalUsageBytes > quotaMaxBytes) {
        NS_WARNING("Origin is already using more storage than allowed!");
        return NS_ERROR_UNEXPECTED;
      }
    }

    InitQuotaForOrigin(aPersistenceType, aGroup, aOrigin, quotaMaxBytes,
                       totalUsageBytes, aAccessTime);
  }

  return NS_OK;
}

nsresult
QuotaManager::MaybeUpgradeIndexedDBDirectory()
{
  AssertIsOnIOThread();

  if (mStorageAreaInitialized) {
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<nsIFile> indexedDBDir =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = indexedDBDir->InitWithPath(mIndexedDBPath);
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = indexedDBDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    
    mStorageAreaInitialized = true;

    return NS_OK;
  }

  bool isDirectory;
  rv = indexedDBDir->IsDirectory(&isDirectory);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isDirectory) {
    NS_WARNING("indexedDB entry is not a directory!");
    return NS_OK;
  }

  nsCOMPtr<nsIFile> persistentStorageDir =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = persistentStorageDir->InitWithPath(mPersistentStoragePath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = persistentStorageDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    NS_WARNING("indexedDB directory shouldn't exist after the upgrade!");
    return NS_OK;
  }

  nsCOMPtr<nsIFile> storageDir;
  rv = persistentStorageDir->GetParent(getter_AddRefs(storageDir));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString persistentStorageName;
  rv = persistentStorageDir->GetLeafName(persistentStorageName);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  rv = indexedDBDir->MoveTo(storageDir, persistentStorageName);
  NS_ENSURE_SUCCESS(rv, rv);

  mStorageAreaInitialized = true;

  return NS_OK;
}

nsresult
QuotaManager::EnsureOriginIsInitialized(PersistenceType aPersistenceType,
                                        const nsACString& aGroup,
                                        const nsACString& aOrigin,
                                        bool aTrackQuota,
                                        nsIFile** aDirectory)
{
  AssertIsOnIOThread();

  nsresult rv = MaybeUpgradeIndexedDBDirectory();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> directory;
  rv = GetDirectoryForOrigin(aPersistenceType, aOrigin,
                             getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT) {
    if (mInitializedOrigins.Contains(aOrigin)) {
      NS_ADDREF(*aDirectory = directory);
      return NS_OK;
    }

    bool created;
    rv = EnsureDirectory(directory, &created);
    NS_ENSURE_SUCCESS(rv, rv);

    if (created) {
      rv = CreateDirectoryUpgradeStamp(directory);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = InitializeOrigin(aPersistenceType, aGroup, aOrigin, aTrackQuota, 0,
                          directory);
    NS_ENSURE_SUCCESS(rv, rv);

    mInitializedOrigins.AppendElement(aOrigin);

    directory.forget(aDirectory);
    return NS_OK;
  }

  NS_ASSERTION(aPersistenceType == PERSISTENCE_TYPE_TEMPORARY, "Huh?");
  NS_ASSERTION(aTrackQuota, "Huh?");

  if (!mTemporaryStorageInitialized) {
    nsCOMPtr<nsIFile> parentDirectory;
    rv = directory->GetParent(getter_AddRefs(parentDirectory));
    NS_ENSURE_SUCCESS(rv, rv);

    bool created;
    rv = EnsureDirectory(parentDirectory, &created);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = parentDirectory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
      nsCOMPtr<nsISupports> entry;
      rv = entries->GetNext(getter_AddRefs(entry));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIFile> childDirectory = do_QueryInterface(entry);
      NS_ENSURE_TRUE(childDirectory, NS_NOINTERFACE);

      bool isDirectory;
      rv = childDirectory->IsDirectory(&isDirectory);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(isDirectory, NS_ERROR_UNEXPECTED);

      int64_t timestamp;
      nsCString group;
      nsCString origin;
      rv = GetDirectoryMetadata(childDirectory, &timestamp, group, origin);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = InitializeOrigin(aPersistenceType, group, origin, aTrackQuota,
                            timestamp, childDirectory);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to initialize origin!");

        
        RemoveQuotaForPersistenceType(aPersistenceType);

        return rv;
      }
    }

    if (gFixedLimitKB >= 0) {
      mTemporaryStorageLimit = gFixedLimitKB * 1024;
    }
    else {
      rv = GetTemporaryStorageLimit(parentDirectory, mTemporaryStorageUsage,
                                    &mTemporaryStorageLimit);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mTemporaryStorageInitialized = true;

    CheckTemporaryStorageLimits();
  }

  bool created;
  rv = EnsureDirectory(directory, &created);
  NS_ENSURE_SUCCESS(rv, rv);

  if (created) {
    int64_t timestamp = PR_Now();

    rv = CreateDirectoryMetadata(directory, timestamp, aGroup, aOrigin);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = InitializeOrigin(aPersistenceType, aGroup, aOrigin, aTrackQuota,
                          timestamp, directory);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  directory.forget(aDirectory);
  return NS_OK;
}

void
QuotaManager::OriginClearCompleted(
                                  PersistenceType aPersistenceType,
                                  const OriginOrPatternString& aOriginOrPattern)
{
  AssertIsOnIOThread();

  if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT) {
    if (aOriginOrPattern.IsOrigin()) {
      mInitializedOrigins.RemoveElement(aOriginOrPattern);
    }
    else {
      for (uint32_t index = mInitializedOrigins.Length(); index > 0; index--) {
        if (PatternMatchesOrigin(aOriginOrPattern,
                                 mInitializedOrigins[index - 1])) {
          mInitializedOrigins.RemoveElementAt(index - 1);
        }
      }
    }
  }

  for (uint32_t index = 0; index < Client::TYPE_MAX; index++) {
    mClients[index]->OnOriginClearCompleted(aPersistenceType, aOriginOrPattern);
  }
}

void
QuotaManager::ResetOrClearCompleted()
{
  AssertIsOnIOThread();

  mInitializedOrigins.Clear();
  mTemporaryStorageInitialized = false;

  ReleaseIOThreadObjects();
}

already_AddRefed<mozilla::dom::quota::Client>
QuotaManager::GetClient(Client::Type aClientType)
{
  nsRefPtr<Client> client = mClients.SafeElementAt(aClientType);
  return client.forget();
}

uint64_t
QuotaManager::GetGroupLimit() const
{
  MOZ_ASSERT(mTemporaryStorageInitialized);

  
  
  
  uint64_t x = std::min<uint64_t>(mTemporaryStorageLimit * .20, 2 GB);

  
  
  return std::min<uint64_t>(mTemporaryStorageLimit,
                            std::max<uint64_t>(x, 10 MB));
}


uint32_t
QuotaManager::GetStorageQuotaMB()
{
  return uint32_t(std::max(gStorageQuotaMB, 0));
}


void
QuotaManager::GetStorageId(PersistenceType aPersistenceType,
                           const nsACString& aOrigin,
                           Client::Type aClientType,
                           const nsAString& aName,
                           nsACString& aDatabaseId)
{
  nsAutoCString str;
  str.AppendInt(aPersistenceType);
  str.Append('*');
  str.Append(aOrigin);
  str.Append('*');
  str.AppendInt(aClientType);
  str.Append('*');
  str.Append(NS_ConvertUTF16toUTF8(aName));

  aDatabaseId = str;
}


nsresult
QuotaManager::GetInfoFromURI(nsIURI* aURI,
                             uint32_t aAppId,
                             bool aInMozBrowser,
                             nsACString* aGroup,
                             nsACString* aASCIIOrigin,
                             StoragePrivilege* aPrivilege,
                             PersistenceType* aDefaultPersistenceType)
{
  NS_ASSERTION(aURI, "Null uri!");

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = secMan->GetAppCodebasePrincipal(aURI, aAppId, aInMozBrowser,
                                                getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetInfoFromPrincipal(principal, aGroup, aASCIIOrigin, aPrivilege,
                            aDefaultPersistenceType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static nsresult
TryGetInfoForAboutURI(nsIPrincipal* aPrincipal,
                      nsACString& aGroup,
                      nsACString& aASCIIOrigin,
                      StoragePrivilege* aPrivilege,
                      PersistenceType* aDefaultPersistenceType)
{
  NS_ASSERTION(aPrincipal, "Don't hand me a null principal!");

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!uri) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  bool isAbout;
  rv = uri->SchemeIs("about", &isAbout);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && isAbout, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAboutModule> module;
  rv = NS_GetAboutModule(uri, getter_AddRefs(module));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> inner = NS_GetInnermostURI(uri);
  NS_ENSURE_TRUE(inner, NS_ERROR_FAILURE);

  nsAutoString postfix;
  rv = module->GetIndexedDBOriginPostfix(uri, postfix);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString origin;
  if (DOMStringIsNull(postfix)) {
    rv = inner->GetSpec(origin);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    nsAutoCString scheme;
    rv = inner->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);

    origin = scheme + NS_LITERAL_CSTRING(":") + NS_ConvertUTF16toUTF8(postfix);
  }

  ToLowerCase(origin);
  aGroup.Assign(origin);
  aASCIIOrigin.Assign(origin);

  if (aPrivilege) {
    *aPrivilege = Content;
  }

  if (aDefaultPersistenceType) {
    *aDefaultPersistenceType = PERSISTENCE_TYPE_PERSISTENT;
  }

  return NS_OK;
}


nsresult
QuotaManager::GetInfoFromPrincipal(nsIPrincipal* aPrincipal,
                                   nsACString* aGroup,
                                   nsACString* aASCIIOrigin,
                                   StoragePrivilege* aPrivilege,
                                   PersistenceType* aDefaultPersistenceType)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aPrincipal, "Don't hand me a null principal!");

  if (aGroup && aASCIIOrigin) {
    nsresult rv = TryGetInfoForAboutURI(aPrincipal, *aGroup, *aASCIIOrigin,
                                        aPrivilege, aDefaultPersistenceType);
    if (NS_SUCCEEDED(rv)) {
      return NS_OK;
    }
  }

  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    GetInfoForChrome(aGroup, aASCIIOrigin, aPrivilege, aDefaultPersistenceType);
    return NS_OK;
  }

  bool isNullPrincipal;
  nsresult rv = aPrincipal->GetIsNullPrincipal(&isNullPrincipal);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isNullPrincipal) {
    NS_WARNING("IndexedDB not supported from this principal!");
    return NS_ERROR_FAILURE;
  }

  nsCString origin;
  rv = aPrincipal->GetOrigin(getter_Copies(origin));
  NS_ENSURE_SUCCESS(rv, rv);

  if (origin.EqualsLiteral("chrome")) {
    NS_WARNING("Non-chrome principal can't use chrome origin!");
    return NS_ERROR_FAILURE;
  }

  nsCString jarPrefix;
  if (aGroup || aASCIIOrigin) {
    rv = aPrincipal->GetJarPrefix(jarPrefix);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aGroup) {
    nsCString baseDomain;
    rv = aPrincipal->GetBaseDomain(baseDomain);
    if (NS_FAILED(rv)) {
      

      nsCOMPtr<nsIURI> uri;
      rv = aPrincipal->GetURI(getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);

      bool isIndexedDBURI = false;
      rv = uri->SchemeIs("indexedDB", &isIndexedDBURI);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isIndexedDBURI) {
        rv = NS_OK;
      }
    }
    NS_ENSURE_SUCCESS(rv, rv);

    if (baseDomain.IsEmpty()) {
      aGroup->Assign(jarPrefix + origin);
    }
    else {
      aGroup->Assign(jarPrefix + baseDomain);
    }
  }

  if (aASCIIOrigin) {
    aASCIIOrigin->Assign(jarPrefix + origin);
  }

  if (aPrivilege) {
    *aPrivilege = Content;
  }

  if (aDefaultPersistenceType) {
    *aDefaultPersistenceType = PERSISTENCE_TYPE_PERSISTENT;
  }

  return NS_OK;
}


nsresult
QuotaManager::GetInfoFromWindow(nsPIDOMWindow* aWindow,
                                nsACString* aGroup,
                                nsACString* aASCIIOrigin,
                                StoragePrivilege* aPrivilege,
                                PersistenceType* aDefaultPersistenceType)
{
  NS_ASSERTION(NS_IsMainThread(),
               "We're about to touch a window off the main thread!");
  NS_ASSERTION(aWindow, "Don't hand me a null window!");

  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(sop, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();
  NS_ENSURE_TRUE(principal, NS_ERROR_FAILURE);

  nsresult rv = GetInfoFromPrincipal(principal, aGroup, aASCIIOrigin,
                                     aPrivilege, aDefaultPersistenceType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


void
QuotaManager::GetInfoForChrome(nsACString* aGroup,
                               nsACString* aASCIIOrigin,
                               StoragePrivilege* aPrivilege,
                               PersistenceType* aDefaultPersistenceType)
{
  NS_ASSERTION(nsContentUtils::IsCallerChrome(), "Only for chrome!");

  static const char kChromeOrigin[] = "chrome";

  if (aGroup) {
    aGroup->AssignLiteral(kChromeOrigin);
  }
  if (aASCIIOrigin) {
    aASCIIOrigin->AssignLiteral(kChromeOrigin);
  }
  if (aPrivilege) {
    *aPrivilege = Chrome;
  }
  if (aDefaultPersistenceType) {
    *aDefaultPersistenceType = PERSISTENCE_TYPE_PERSISTENT;
  }
}

NS_IMPL_ISUPPORTS(QuotaManager, nsIQuotaManager, nsIObserver)

NS_IMETHODIMP
QuotaManager::GetUsageForURI(nsIURI* aURI,
                             nsIUsageCallback* aCallback,
                             uint32_t aAppId,
                             bool aInMozBrowserOnly,
                             uint8_t aOptionalArgCount,
                             nsIQuotaRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aCallback);

  
  NS_ENSURE_TRUE(IsMainProcess(), NS_ERROR_NOT_AVAILABLE);

  if (!aOptionalArgCount) {
    aAppId = nsIScriptSecurityManager::NO_APP_ID;
  }

  
  nsCString group;
  nsCString origin;
  nsresult rv = GetInfoFromURI(aURI, aAppId, aInMozBrowserOnly, &group, &origin,
                               nullptr, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  OriginOrPatternString oops = OriginOrPatternString::FromOrigin(origin);

  nsRefPtr<AsyncUsageRunnable> runnable =
    new AsyncUsageRunnable(aAppId, aInMozBrowserOnly, group, oops, aURI,
                           aCallback);

  
  rv = WaitForOpenAllowed(oops, Nullable<PersistenceType>(), EmptyCString(),
                          runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  runnable->AdvanceState();

  runnable.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
QuotaManager::Clear()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!gTestingEnabled) {
    NS_WARNING("Testing features are not enabled!");
    return NS_OK;
  }

  OriginOrPatternString oops = OriginOrPatternString::FromNull();

  nsRefPtr<ResetOrClearRunnable> runnable = new ResetOrClearRunnable(true);

  
  nsresult rv =
    WaitForOpenAllowed(oops, Nullable<PersistenceType>(), EmptyCString(),
                       runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  runnable->AdvanceState();

  
  StorageMatcher<nsAutoTArray<nsIOfflineStorage*, 20> > matches;
  matches.Find(mLiveStorages);

  for (uint32_t index = 0; index < matches.Length(); index++) {
    
    
    nsCOMPtr<nsIOfflineStorage> storage = matches[index];
    storage->Invalidate();
  }

  
  
  return NS_OK;
}

NS_IMETHODIMP
QuotaManager::ClearStoragesForURI(nsIURI* aURI,
                                  uint32_t aAppId,
                                  bool aInMozBrowserOnly,
                                  const nsACString& aPersistenceType,
                                  uint8_t aOptionalArgCount)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);

  Nullable<PersistenceType> persistenceType;
  nsresult rv =
    NullablePersistenceTypeFromText(aPersistenceType, &persistenceType);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_INVALID_ARG;
  }

  
  NS_ENSURE_TRUE(IsMainProcess(), NS_ERROR_NOT_AVAILABLE);

  if (!aOptionalArgCount) {
    aAppId = nsIScriptSecurityManager::NO_APP_ID;
  }

  
  nsCString origin;
  rv = GetInfoFromURI(aURI, aAppId, aInMozBrowserOnly, nullptr, &origin,
                      nullptr, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString pattern;
  GetOriginPatternString(aAppId, aInMozBrowserOnly, origin, pattern);

  
  
  if (IsClearOriginPending(pattern, persistenceType)) {
    return NS_OK;
  }

  OriginOrPatternString oops = OriginOrPatternString::FromPattern(pattern);

  
  nsRefPtr<OriginClearRunnable> runnable =
    new OriginClearRunnable(oops, persistenceType);

  rv = WaitForOpenAllowed(oops, persistenceType, EmptyCString(), runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  runnable->AdvanceState();

  
  StorageMatcher<nsAutoTArray<nsIOfflineStorage*, 20> > matches;
  matches.Find(mLiveStorages, pattern);

  for (uint32_t index = 0; index < matches.Length(); index++) {
    if (persistenceType.IsNull() ||
        matches[index]->Type() == persistenceType.Value()) {
      
      
      nsCOMPtr<nsIOfflineStorage> storage = matches[index];
      storage->Invalidate();
    }
  }

  
  
  return NS_OK;
}

NS_IMETHODIMP
QuotaManager::Reset()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!gTestingEnabled) {
    NS_WARNING("Testing features are not enabled!");
    return NS_OK;
  }

  OriginOrPatternString oops = OriginOrPatternString::FromNull();

  nsRefPtr<ResetOrClearRunnable> runnable = new ResetOrClearRunnable(false);

  
  nsresult rv =
    WaitForOpenAllowed(oops, Nullable<PersistenceType>(), EmptyCString(),
                       runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  runnable->AdvanceState();

  
  StorageMatcher<nsAutoTArray<nsIOfflineStorage*, 20> > matches;
  matches.Find(mLiveStorages);

  for (uint32_t index = 0; index < matches.Length(); index++) {
    
    
    nsCOMPtr<nsIOfflineStorage> storage = matches[index];
    storage->Invalidate();
  }

  
  
  return NS_OK;
}

NS_IMETHODIMP
QuotaManager::Observe(nsISupports* aSubject,
                      const char* aTopic,
                      const char16_t* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!strcmp(aTopic, PROFILE_BEFORE_CHANGE_OBSERVER_ID)) {
    
    
    if (gShutdown.exchange(true)) {
      NS_ERROR("Shutdown more than once?!");
    }

    if (IsMainProcess()) {
      FileService* service = FileService::Get();
      if (service) {
        
        
        
        
        
        

        nsTArray<uint32_t> indexes;
        for (uint32_t index = 0; index < Client::TYPE_MAX; index++) {
          if (mClients[index]->IsFileServiceUtilized()) {
            indexes.AppendElement(index);
          }
        }

        StorageMatcher<nsTArray<nsCOMPtr<nsIOfflineStorage>>> liveStorages;
        liveStorages.Find(mLiveStorages, &indexes);

        if (!liveStorages.IsEmpty()) {
          nsRefPtr<WaitForFileHandlesToFinishRunnable> runnable =
            new WaitForFileHandlesToFinishRunnable();

          service->WaitForStoragesToComplete(liveStorages, runnable);

          nsIThread* thread = NS_GetCurrentThread();
          while (runnable->IsBusy()) {
            if (!NS_ProcessNextEvent(thread)) {
              NS_ERROR("Failed to process next event!");
              break;
            }
          }
        }
      }

      
      if (NS_FAILED(mShutdownTimer->Init(this, DEFAULT_SHUTDOWN_TIMER_MS,
                                         nsITimer::TYPE_ONE_SHOT))) {
        NS_WARNING("Failed to initialize shutdown timer!");
      }

      
      
      for (uint32_t index = 0; index < Client::TYPE_MAX; index++) {
        mClients[index]->ShutdownTransactionService();
      }

      
      if (NS_FAILED(mShutdownTimer->Cancel())) {
        NS_WARNING("Failed to cancel shutdown timer!");
      }

      
      nsCOMPtr<nsIRunnable> runnable =
        NS_NewRunnableMethod(this, &QuotaManager::ReleaseIOThreadObjects);
      if (!runnable) {
        NS_WARNING("Failed to create runnable!");
      }

      if (NS_FAILED(mIOThread->Dispatch(runnable, NS_DISPATCH_NORMAL))) {
        NS_WARNING("Failed to dispatch runnable!");
      }

      
      if (NS_FAILED(mIOThread->Shutdown())) {
        NS_WARNING("Failed to shutdown IO thread!");
      }
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC)) {
    NS_ASSERTION(IsMainProcess(), "Should only happen in the main process!");

    NS_WARNING("Some storage operations are taking longer than expected "
               "during shutdown and will be aborted!");

    
    StorageMatcher<nsAutoTArray<nsIOfflineStorage*, 50> > liveStorages;
    liveStorages.Find(mLiveStorages);

    
    if (!liveStorages.IsEmpty()) {
      uint32_t count = liveStorages.Length();
      for (uint32_t index = 0; index < count; index++) {
        liveStorages[index]->Invalidate();
      }
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, TOPIC_WEB_APP_CLEAR_DATA)) {
    nsCOMPtr<mozIApplicationClearPrivateDataParams> params =
      do_QueryInterface(aSubject);
    NS_ENSURE_TRUE(params, NS_ERROR_UNEXPECTED);

    uint32_t appId;
    nsresult rv = params->GetAppId(&appId);
    NS_ENSURE_SUCCESS(rv, rv);

    bool browserOnly;
    rv = params->GetBrowserOnly(&browserOnly);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ClearStoragesForApp(appId, browserOnly);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_NOTREACHED("Unknown topic!");
  return NS_ERROR_UNEXPECTED;
}

void
QuotaManager::SetCurrentWindowInternal(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(mCurrentWindowIndex != BAD_TLS_INDEX,
               "Should have a valid TLS storage index!");

  if (aWindow) {
    NS_ASSERTION(!PR_GetThreadPrivate(mCurrentWindowIndex),
                 "Somebody forgot to clear the current window!");
    PR_SetThreadPrivate(mCurrentWindowIndex, aWindow);
  }
  else {
    
    
    PR_SetThreadPrivate(mCurrentWindowIndex, nullptr);
  }
}

void
QuotaManager::CancelPromptsForWindowInternal(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<CheckQuotaHelper> helper;

  MutexAutoLock autoLock(mQuotaMutex);

  if (mCheckQuotaHelpers.Get(aWindow, getter_AddRefs(helper))) {
    helper->Cancel();
  }
}

bool
QuotaManager::LockedQuotaIsLifted()
{
  mQuotaMutex.AssertCurrentThreadOwns();

  NS_ASSERTION(mCurrentWindowIndex != BAD_TLS_INDEX,
               "Should have a valid TLS storage index!");

  nsPIDOMWindow* window =
    static_cast<nsPIDOMWindow*>(PR_GetThreadPrivate(mCurrentWindowIndex));

  
  
  NS_ASSERTION(window, "Why don't we have a Window here?");

  bool createdHelper = false;

  nsRefPtr<CheckQuotaHelper> helper;
  if (!mCheckQuotaHelpers.Get(window, getter_AddRefs(helper))) {
    helper = new CheckQuotaHelper(window, mQuotaMutex);
    createdHelper = true;

    mCheckQuotaHelpers.Put(window, helper);

    
    
    
    
    {
      MutexAutoUnlock autoUnlock(mQuotaMutex);

      nsresult rv = NS_DispatchToMainThread(helper);
      NS_ENSURE_SUCCESS(rv, false);
    }

    
    
    
  }

  bool result = helper->PromptAndReturnQuotaIsDisabled();

  
  
  if (createdHelper) {
    mCheckQuotaHelpers.Remove(window);
  }

  return result;
}

uint64_t
QuotaManager::LockedCollectOriginsForEviction(
                                            uint64_t aMinSizeToBeFreed,
                                            nsTArray<OriginInfo*>& aOriginInfos)
{
  mQuotaMutex.AssertCurrentThreadOwns();

  nsRefPtr<CollectOriginsHelper> helper =
    new CollectOriginsHelper(mQuotaMutex, aMinSizeToBeFreed);

  
  
  {
    MutexAutoUnlock autoUnlock(mQuotaMutex);

    if (NS_FAILED(NS_DispatchToMainThread(helper))) {
      NS_WARNING("Failed to dispatch to the main thread!");
    }
  }

  return helper->BlockAndReturnOriginsForEviction(aOriginInfos);
}

void
QuotaManager::LockedRemoveQuotaForOrigin(PersistenceType aPersistenceType,
                                         const nsACString& aGroup,
                                         const nsACString& aOrigin)
{
  mQuotaMutex.AssertCurrentThreadOwns();

  GroupInfoPair* pair;
  mGroupInfoPairs.Get(aGroup, &pair);

  if (!pair) {
    return;
  }

  nsRefPtr<GroupInfo> groupInfo = pair->LockedGetGroupInfo(aPersistenceType);
  if (groupInfo) {
    groupInfo->LockedRemoveOriginInfo(aOrigin);

    if (!groupInfo->LockedHasOriginInfos()) {
      pair->LockedClearGroupInfo(aPersistenceType);

      if (!pair->LockedHasGroupInfos()) {
        mGroupInfoPairs.Remove(aGroup);
      }
    }
  }
}

nsresult
QuotaManager::AcquireExclusiveAccess(const nsACString& aPattern,
                                     Nullable<PersistenceType> aPersistenceType,
                                     nsIOfflineStorage* aStorage,
                                     AcquireListener* aListener,
                                     WaitingOnStoragesCallback aCallback,
                                     void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aListener, "Need a listener!");

  
  SynchronizedOp* op =
    FindSynchronizedOp(aPattern, aPersistenceType,
                       aStorage ? aStorage->Id() : EmptyCString());

  NS_ASSERTION(op, "We didn't find a SynchronizedOp?");
  NS_ASSERTION(!op->mListener, "SynchronizedOp already has a listener?!?");

  nsTArray<nsCOMPtr<nsIOfflineStorage> > liveStorages;

  if (aStorage) {
    
    
    

    Client::Type clientType = aStorage->GetClient()->GetType();

    StorageMatcher<nsAutoTArray<nsIOfflineStorage*, 20> > matches;
    matches.Find(mLiveStorages, aPattern, clientType);

    if (!matches.IsEmpty()) {
      
      
      for (uint32_t index = 0; index < matches.Length(); index++) {
        nsIOfflineStorage*& storage = matches[index];
        if (!storage->IsClosed() &&
            storage != aStorage &&
            storage->Id() == aStorage->Id() &&
            (aPersistenceType.IsNull() ||
             aPersistenceType.Value() == storage->Type())) {
          liveStorages.AppendElement(storage);
        }
      }
    }

    if (!liveStorages.IsEmpty()) {
      NS_ASSERTION(op->mStorages[clientType].IsEmpty(),
                   "How do we already have storages here?");
      op->mStorages[clientType].AppendElements(liveStorages);
    }
  }
  else {
    StorageMatcher<ArrayCluster<nsIOfflineStorage*> > matches;
    if (aPattern.IsVoid()) {
      matches.Find(mLiveStorages);
    }
    else {
      matches.Find(mLiveStorages, aPattern);
    }

    NS_ASSERTION(op->mStorages.IsEmpty(),
               "How do we already have storages here?");

    
    
    if (!matches.IsEmpty()) {
      for (uint32_t i = 0; i < Client::TYPE_MAX; i++) {
        nsTArray<nsIOfflineStorage*>& storages = matches.ArrayAt(i);
        for (uint32_t j = 0; j < storages.Length(); j++) {
          nsIOfflineStorage* storage = storages[j];
          if (aPersistenceType.IsNull() ||
              aPersistenceType.Value() == storage->Type()) {
            liveStorages.AppendElement(storage);
            op->mStorages[i].AppendElement(storage);
          }
        }
      }
    }
  }

  op->mListener = aListener;

  if (!liveStorages.IsEmpty()) {
    
    aCallback(liveStorages, aClosure);

    NS_ASSERTION(liveStorages.IsEmpty(),
                 "Should have done something with the array!");

    if (aStorage) {
      
      return NS_OK;
    }
  }

  
  
  nsresult rv = RunSynchronizedOp(aStorage, op);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
QuotaManager::RunSynchronizedOp(nsIOfflineStorage* aStorage,
                                SynchronizedOp* aOp)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aOp, "Null pointer!");
  NS_ASSERTION(aOp->mListener, "No listener on this op!");
  NS_ASSERTION(!aStorage ||
               aOp->mStorages[aStorage->GetClient()->GetType()].IsEmpty(),
               "This op isn't ready to run!");

  ArrayCluster<nsIOfflineStorage*> storages;

  uint32_t startIndex;
  uint32_t endIndex;

  if (aStorage) {
    Client::Type clientType = aStorage->GetClient()->GetType();

    storages[clientType].AppendElement(aStorage);

    startIndex = clientType;
    endIndex = clientType + 1;
  }
  else {
    aOp->mStorages.SwapElements(storages);

    startIndex = 0;
    endIndex = Client::TYPE_MAX;
  }

  nsRefPtr<WaitForTransactionsToFinishRunnable> runnable =
    new WaitForTransactionsToFinishRunnable(aOp);

  
  FileService* service = FileService::Get();

  if (service) {
    
    nsTArray<nsCOMPtr<nsIOfflineStorage>> array;

    for (uint32_t index = startIndex; index < endIndex; index++)  {
      if (!storages[index].IsEmpty() &&
          mClients[index]->IsFileServiceUtilized()) {
        array.AppendElements(storages[index]);
      }
    }

    if (!array.IsEmpty()) {
      runnable->AddRun();

      service->WaitForStoragesToComplete(array, runnable);
    }
  }

  
  
  for (uint32_t index = startIndex; index < endIndex; index++)  {
    nsRefPtr<Client>& client = mClients[index];
    if (!storages[index].IsEmpty() && client->IsTransactionServiceActivated()) {
      runnable->AddRun();

      client->WaitForStoragesToComplete(storages[index], runnable);
    }
  }

  nsresult rv = runnable->Run();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

SynchronizedOp*
QuotaManager::FindSynchronizedOp(const nsACString& aPattern,
                                 Nullable<PersistenceType> aPersistenceType,
                                 const nsACString& aId)
{
  for (uint32_t index = 0; index < mSynchronizedOps.Length(); index++) {
    const nsAutoPtr<SynchronizedOp>& currentOp = mSynchronizedOps[index];
    if (PatternMatchesOrigin(aPattern, currentOp->mOriginOrPattern) &&
        (currentOp->mPersistenceType.IsNull() ||
         currentOp->mPersistenceType == aPersistenceType) &&
        (currentOp->mId.IsEmpty() || currentOp->mId == aId)) {
      return currentOp;
    }
  }

  return nullptr;
}

nsresult
QuotaManager::ClearStoragesForApp(uint32_t aAppId, bool aBrowserOnly)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID,
               "Bad appId!");

  
  NS_ENSURE_TRUE(IsMainProcess(), NS_ERROR_NOT_AVAILABLE);

  nsAutoCString pattern;
  GetOriginPatternStringMaybeIgnoreBrowser(aAppId, aBrowserOnly, pattern);

  
  Nullable<PersistenceType> persistenceType;

  
  
  if (IsClearOriginPending(pattern, persistenceType)) {
    return NS_OK;
  }

  OriginOrPatternString oops = OriginOrPatternString::FromPattern(pattern);

  
  nsRefPtr<OriginClearRunnable> runnable =
    new OriginClearRunnable(oops, persistenceType);

  nsresult rv =
    WaitForOpenAllowed(oops, persistenceType, EmptyCString(), runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  runnable->AdvanceState();

  
  StorageMatcher<nsAutoTArray<nsIOfflineStorage*, 20> > matches;
  matches.Find(mLiveStorages, pattern);

  for (uint32_t index = 0; index < matches.Length(); index++) {
    
    
    nsCOMPtr<nsIOfflineStorage> storage = matches[index];
    storage->Invalidate();
  }

  return NS_OK;
}


PLDHashOperator
QuotaManager::GetOriginsExceedingGroupLimit(const nsACString& aKey,
                                            GroupInfoPair* aValue,
                                            void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  nsRefPtr<GroupInfo> groupInfo =
    aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
  if (groupInfo) {
    QuotaManager* quotaManager = QuotaManager::Get();
    NS_ASSERTION(quotaManager, "Shouldn't be null!");

    if (groupInfo->mUsage > quotaManager->GetGroupLimit()) {
      nsTArray<OriginInfo*>* doomedOriginInfos =
        static_cast<nsTArray<OriginInfo*>*>(aUserArg);

      nsTArray<nsRefPtr<OriginInfo> >& originInfos = groupInfo->mOriginInfos;
      originInfos.Sort(OriginInfoLRUComparator());

      uint64_t usage = groupInfo->mUsage;
      for (uint32_t i = 0; i < originInfos.Length(); i++) {
        OriginInfo* originInfo = originInfos[i];

        doomedOriginInfos->AppendElement(originInfo);
        usage -= originInfo->mUsage;

        if (usage <= quotaManager->GetGroupLimit()) {
          break;
        }
      }
    }
  }

  return PL_DHASH_NEXT;
}


PLDHashOperator
QuotaManager::GetAllTemporaryStorageOrigins(const nsACString& aKey,
                                            GroupInfoPair* aValue,
                                            void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  nsRefPtr<GroupInfo> groupInfo =
    aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
  if (groupInfo) {
    nsTArray<OriginInfo*>* originInfos =
      static_cast<nsTArray<OriginInfo*>*>(aUserArg);

    originInfos->AppendElements(groupInfo->mOriginInfos);
  }

  return PL_DHASH_NEXT;
}

void
QuotaManager::CheckTemporaryStorageLimits()
{
  AssertIsOnIOThread();

  nsTArray<OriginInfo*> doomedOriginInfos;
  {
    MutexAutoLock lock(mQuotaMutex);

    mGroupInfoPairs.EnumerateRead(GetOriginsExceedingGroupLimit,
                                  &doomedOriginInfos);

    uint64_t usage = 0;
    for (uint32_t index = 0; index < doomedOriginInfos.Length(); index++) {
      usage += doomedOriginInfos[index]->mUsage;
    }

    if (mTemporaryStorageUsage - usage > mTemporaryStorageLimit) {
      nsTArray<OriginInfo*> originInfos;

      mGroupInfoPairs.EnumerateRead(GetAllTemporaryStorageOrigins,
                                    &originInfos);

      for (uint32_t index = originInfos.Length(); index > 0; index--) {
        if (doomedOriginInfos.Contains(originInfos[index - 1])) {
          originInfos.RemoveElementAt(index - 1);
        }
      }

      originInfos.Sort(OriginInfoLRUComparator());

      for (uint32_t i = 0; i < originInfos.Length(); i++) {
        if (mTemporaryStorageUsage - usage <= mTemporaryStorageLimit) {
          originInfos.TruncateLength(i);
          break;
        }

        usage += originInfos[i]->mUsage;
      }

      doomedOriginInfos.AppendElements(originInfos);
    }
  }

  for (uint32_t index = 0; index < doomedOriginInfos.Length(); index++) {
    DeleteTemporaryFilesForOrigin(doomedOriginInfos[index]->mOrigin);
  }

  nsTArray<nsCString> doomedOrigins;
  {
    MutexAutoLock lock(mQuotaMutex);

    for (uint32_t index = 0; index < doomedOriginInfos.Length(); index++) {
      OriginInfo* doomedOriginInfo = doomedOriginInfos[index];

      nsCString group = doomedOriginInfo->mGroupInfo->mGroup;
      nsCString origin = doomedOriginInfo->mOrigin;
      LockedRemoveQuotaForOrigin(PERSISTENCE_TYPE_TEMPORARY, group, origin);

#ifdef DEBUG
      doomedOriginInfos[index] = nullptr;
#endif

      doomedOrigins.AppendElement(origin);
    }
  }

  for (uint32_t index = 0; index < doomedOrigins.Length(); index++) {
    OriginClearCompleted(
                       PERSISTENCE_TYPE_TEMPORARY,
                       OriginOrPatternString::FromOrigin(doomedOrigins[index]));
  }
}


PLDHashOperator
QuotaManager::AddTemporaryStorageOrigins(
                                       const nsACString& aKey,
                                       ArrayCluster<nsIOfflineStorage*>* aValue,
                                       void* aUserArg)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");
  NS_ASSERTION(aUserArg, "Null pointer!");

  OriginCollection& collection = *static_cast<OriginCollection*>(aUserArg);

  if (collection.ContainsOrigin(aKey)) {
    return PL_DHASH_NEXT;
  }

  for (uint32_t i = 0; i < Client::TYPE_MAX; i++) {
    nsTArray<nsIOfflineStorage*>& array = (*aValue)[i];
    for (uint32_t j = 0; j < array.Length(); j++) {
      nsIOfflineStorage*& storage = array[j];
      if (storage->Type() == PERSISTENCE_TYPE_TEMPORARY) {
        collection.AddOrigin(aKey);
        return PL_DHASH_NEXT;
      }
    }
  }

  return PL_DHASH_NEXT;
}


PLDHashOperator
QuotaManager::GetInactiveTemporaryStorageOrigins(const nsACString& aKey,
                                                 GroupInfoPair* aValue,
                                                 void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");
  NS_ASSERTION(aUserArg, "Null pointer!");

  nsRefPtr<GroupInfo> groupInfo =
    aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
  if (groupInfo) {
    InactiveOriginsInfo* info = static_cast<InactiveOriginsInfo*>(aUserArg);

    nsTArray<nsRefPtr<OriginInfo> >& originInfos = groupInfo->mOriginInfos;

    for (uint32_t i = 0; i < originInfos.Length(); i++) {
      OriginInfo* originInfo = originInfos[i];

      if (!info->collection.ContainsOrigin(originInfo->mOrigin)) {
        NS_ASSERTION(!originInfo->mQuotaObjects.Count(),
                     "Inactive origin shouldn't have open files!");
        info->origins.AppendElement(originInfo);
      }
    }
  }

  return PL_DHASH_NEXT;
}

uint64_t
QuotaManager::CollectOriginsForEviction(uint64_t aMinSizeToBeFreed,
                                        nsTArray<OriginInfo*>& aOriginInfos)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  
  OriginCollection originCollection;

  
  
  uint32_t index;
  for (index = 0; index < mSynchronizedOps.Length(); index++) {
    nsAutoPtr<SynchronizedOp>& op = mSynchronizedOps[index];
    if (op->mPersistenceType.IsNull() ||
        op->mPersistenceType.Value() == PERSISTENCE_TYPE_TEMPORARY) {
      if (op->mOriginOrPattern.IsPattern() &&
          !originCollection.ContainsPattern(op->mOriginOrPattern)) {
        originCollection.AddPattern(op->mOriginOrPattern);
      }
    }
  }

  for (index = 0; index < mSynchronizedOps.Length(); index++) {
    nsAutoPtr<SynchronizedOp>& op = mSynchronizedOps[index];
    if (op->mPersistenceType.IsNull() ||
        op->mPersistenceType.Value() == PERSISTENCE_TYPE_TEMPORARY) {
      if (op->mOriginOrPattern.IsOrigin() &&
          !originCollection.ContainsOrigin(op->mOriginOrPattern)) {
        originCollection.AddOrigin(op->mOriginOrPattern);
      }
    }
  }

  
  mLiveStorages.EnumerateRead(AddTemporaryStorageOrigins, &originCollection);

  
  nsTArray<OriginInfo*> inactiveOrigins;
  {
    InactiveOriginsInfo info(originCollection, inactiveOrigins);
    MutexAutoLock lock(mQuotaMutex);
    mGroupInfoPairs.EnumerateRead(GetInactiveTemporaryStorageOrigins, &info);
  }

  
  

  
  inactiveOrigins.Sort(OriginInfoLRUComparator());

  
  
  uint64_t sizeToBeFreed = 0;
  for(index = 0; index < inactiveOrigins.Length(); index++) {
    if (sizeToBeFreed >= aMinSizeToBeFreed) {
      inactiveOrigins.TruncateLength(index);
      break;
    }

    sizeToBeFreed += inactiveOrigins[index]->mUsage;
  }

  if (sizeToBeFreed >= aMinSizeToBeFreed) {
    
    

    for(index = 0; index < inactiveOrigins.Length(); index++) {
      OriginOrPatternString oops =
        OriginOrPatternString::FromOrigin(inactiveOrigins[index]->mOrigin);

      AddSynchronizedOp(oops,
                        Nullable<PersistenceType>(PERSISTENCE_TYPE_TEMPORARY));
    }

    inactiveOrigins.SwapElements(aOriginInfos);
    return sizeToBeFreed;
  }

  return 0;
}

void
QuotaManager::DeleteTemporaryFilesForOrigin(const nsACString& aOrigin)
{
  nsCOMPtr<nsIFile> directory;
  nsresult rv = GetDirectoryForOrigin(PERSISTENCE_TYPE_TEMPORARY, aOrigin,
                                      getter_AddRefs(directory));
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = directory->Remove(true);
  if (rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST &&
      rv != NS_ERROR_FILE_NOT_FOUND && NS_FAILED(rv)) {
    
    
    NS_ERROR("Failed to remove directory!");
  }
}

void
QuotaManager::FinalizeOriginEviction(nsTArray<nsCString>& aOrigins)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<FinalizeOriginEvictionRunnable> runnable =
    new FinalizeOriginEvictionRunnable(aOrigins);

  nsresult rv = IsOnIOThread() ? runnable->RunImmediately()
                               : runnable->Dispatch();
  NS_ENSURE_SUCCESS_VOID(rv);
}

void
QuotaManager::SaveOriginAccessTime(const nsACString& aOrigin,
                                   int64_t aTimestamp)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (QuotaManager::IsShuttingDown()) {
    return;
  }

  nsRefPtr<SaveOriginAccessTimeRunnable> runnable =
    new SaveOriginAccessTimeRunnable(aOrigin, aTimestamp);

  if (NS_FAILED(mIOThread->Dispatch(runnable, NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch runnable!");
  }
}

void
QuotaManager::GetOriginPatternString(uint32_t aAppId,
                                     MozBrowserPatternFlag aBrowserFlag,
                                     const nsACString& aOrigin,
                                     nsAutoCString& _retval)
{
  NS_ASSERTION(aAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID,
               "Bad appId!");
  NS_ASSERTION(aOrigin.IsEmpty() || aBrowserFlag != IgnoreMozBrowser,
               "Bad args!");

  if (aOrigin.IsEmpty()) {
    _retval.Truncate();

    _retval.AppendInt(aAppId);
    _retval.Append('+');

    if (aBrowserFlag != IgnoreMozBrowser) {
      if (aBrowserFlag == MozBrowser) {
        _retval.Append('t');
      }
      else {
        _retval.Append('f');
      }
      _retval.Append('+');
    }

    return;
  }

#ifdef DEBUG
  if (aAppId != nsIScriptSecurityManager::NO_APP_ID ||
      aBrowserFlag == MozBrowser) {
    nsAutoCString pattern;
    GetOriginPatternString(aAppId, aBrowserFlag, EmptyCString(), pattern);
    NS_ASSERTION(PatternMatchesOrigin(pattern, aOrigin),
                 "Origin doesn't match parameters!");
  }
#endif

  _retval = aOrigin;
}

SynchronizedOp::SynchronizedOp(const OriginOrPatternString& aOriginOrPattern,
                               Nullable<PersistenceType> aPersistenceType,
                               const nsACString& aId)
: mOriginOrPattern(aOriginOrPattern), mPersistenceType(aPersistenceType),
  mId(aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  MOZ_COUNT_CTOR(SynchronizedOp);
}

SynchronizedOp::~SynchronizedOp()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  MOZ_COUNT_DTOR(SynchronizedOp);
}

bool
SynchronizedOp::MustWaitFor(const SynchronizedOp& aExistingOp)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (aExistingOp.mOriginOrPattern.IsNull() || mOriginOrPattern.IsNull()) {
    return true;
  }

  bool match;

  if (aExistingOp.mOriginOrPattern.IsOrigin()) {
    if (mOriginOrPattern.IsOrigin()) {
      match = aExistingOp.mOriginOrPattern.Equals(mOriginOrPattern);
    }
    else {
      match = PatternMatchesOrigin(mOriginOrPattern, aExistingOp.mOriginOrPattern);
    }
  }
  else if (mOriginOrPattern.IsOrigin()) {
    match = PatternMatchesOrigin(aExistingOp.mOriginOrPattern, mOriginOrPattern);
  }
  else {
    match = PatternMatchesOrigin(mOriginOrPattern, aExistingOp.mOriginOrPattern) ||
            PatternMatchesOrigin(aExistingOp.mOriginOrPattern, mOriginOrPattern);
  }

  
  if (!match) {
    return false;
  }

  
  
  if (!aExistingOp.mPersistenceType.IsNull() && !mPersistenceType.IsNull() &&
      aExistingOp.mPersistenceType.Value() != mPersistenceType.Value()) {
    return false;
  }

  
  if (aExistingOp.mId == mId) {
    return true;
  }

  
  
  if (aExistingOp.mId.IsEmpty() || mId.IsEmpty()) {
    return true;
  }

  
  
  return false;
}

void
SynchronizedOp::DelayRunnable(nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mDelayedRunnables.IsEmpty() || mId.IsEmpty(),
               "Only ClearOrigin operations can delay multiple runnables!");

  mDelayedRunnables.AppendElement(aRunnable);
}

void
SynchronizedOp::DispatchDelayedRunnables()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mListener, "Any listener should be gone by now!");

  uint32_t count = mDelayedRunnables.Length();
  for (uint32_t index = 0; index < count; index++) {
    NS_DispatchToCurrentThread(mDelayedRunnables[index]);
  }

  mDelayedRunnables.Clear();
}

CollectOriginsHelper::CollectOriginsHelper(mozilla::Mutex& aMutex,
                                           uint64_t aMinSizeToBeFreed)
: mMinSizeToBeFreed(aMinSizeToBeFreed),
  mMutex(aMutex),
  mCondVar(aMutex, "CollectOriginsHelper::mCondVar"),
  mSizeToBeFreed(0),
  mWaiting(true)
{
  MOZ_ASSERT(!NS_IsMainThread(), "Wrong thread!");
  mMutex.AssertCurrentThreadOwns();
}

int64_t
CollectOriginsHelper::BlockAndReturnOriginsForEviction(
                                            nsTArray<OriginInfo*>& aOriginInfos)
{
  MOZ_ASSERT(!NS_IsMainThread(), "Wrong thread!");
  mMutex.AssertCurrentThreadOwns();

  while (mWaiting) {
    mCondVar.Wait();
  }

  mOriginInfos.SwapElements(aOriginInfos);
  return mSizeToBeFreed;
}

NS_IMETHODIMP
CollectOriginsHelper::Run()
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  
  
  nsTArray<OriginInfo*> originInfos;
  uint64_t sizeToBeFreed =
    quotaManager->CollectOriginsForEviction(mMinSizeToBeFreed, originInfos);

  MutexAutoLock lock(mMutex);

  NS_ASSERTION(mWaiting, "Huh?!");

  mOriginInfos.SwapElements(originInfos);
  mSizeToBeFreed = sizeToBeFreed;
  mWaiting = false;
  mCondVar.Notify();

  return NS_OK;
}

nsresult
OriginClearRunnable::OnExclusiveAccessAcquired()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  nsresult rv = quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


void
OriginClearRunnable::InvalidateOpenedStorages(
                              nsTArray<nsCOMPtr<nsIOfflineStorage> >& aStorages,
                              void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsTArray<nsCOMPtr<nsIOfflineStorage> > storages;
  storages.SwapElements(aStorages);

  for (uint32_t index = 0; index < storages.Length(); index++) {
    storages[index]->Invalidate();
  }
}

void
OriginClearRunnable::DeleteFiles(QuotaManager* aQuotaManager,
                                 PersistenceType aPersistenceType)
{
  AssertIsOnIOThread();
  NS_ASSERTION(aQuotaManager, "Don't pass me null!");

  nsresult rv;

  nsCOMPtr<nsIFile> directory =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = directory->InitWithPath(aQuotaManager->GetStoragePath(aPersistenceType));
  NS_ENSURE_SUCCESS_VOID(rv);

  nsCOMPtr<nsISimpleEnumerator> entries;
  if (NS_FAILED(directory->GetDirectoryEntries(getter_AddRefs(entries))) ||
      !entries) {
    return;
  }

  nsCString originSanitized(mOriginOrPattern);
  SanitizeOriginString(originSanitized);

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    NS_ENSURE_SUCCESS_VOID(rv);

    nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
    NS_ASSERTION(file, "Don't know what this is!");

    bool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS_VOID(rv);

    if (!isDirectory) {
      NS_WARNING("Something in the IndexedDB directory that doesn't belong!");
      continue;
    }

    nsString leafName;
    rv = file->GetLeafName(leafName);
    NS_ENSURE_SUCCESS_VOID(rv);

    
    if (!PatternMatchesOrigin(originSanitized,
                              NS_ConvertUTF16toUTF8(leafName))) {
      continue;
    }

    if (NS_FAILED(file->Remove(true))) {
      
      
      NS_ERROR("Failed to remove directory!");
    }
  }

  aQuotaManager->RemoveQuotaForPattern(aPersistenceType, mOriginOrPattern);

  aQuotaManager->OriginClearCompleted(aPersistenceType, mOriginOrPattern);
}

NS_IMPL_ISUPPORTS_INHERITED0(OriginClearRunnable, nsRunnable)

NS_IMETHODIMP
OriginClearRunnable::Run()
{
  PROFILER_LABEL("OriginClearRunnable", "Run",
    js::ProfileEntry::Category::OTHER);

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  switch (mCallbackState) {
    case Pending: {
      NS_NOTREACHED("Should never get here without being dispatched!");
      return NS_ERROR_UNEXPECTED;
    }

    case OpenAllowed: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      AdvanceState();

      
      
      nsresult rv =
        quotaManager->AcquireExclusiveAccess(mOriginOrPattern, mPersistenceType,
                                             this, InvalidateOpenedStorages,
                                             nullptr);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }

    case IO: {
      AssertIsOnIOThread();

      AdvanceState();

      if (mPersistenceType.IsNull()) {
        DeleteFiles(quotaManager, PERSISTENCE_TYPE_PERSISTENT);
        DeleteFiles(quotaManager, PERSISTENCE_TYPE_TEMPORARY);
      } else {
        DeleteFiles(quotaManager, mPersistenceType.Value());
      }

      
      if (NS_FAILED(NS_DispatchToMainThread(this))) {
        NS_WARNING("Failed to dispatch to main thread!");
        return NS_ERROR_FAILURE;
      }

      return NS_OK;
    }

    case Complete: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      
      quotaManager->AllowNextSynchronizedOp(mOriginOrPattern, mPersistenceType,
                                            EmptyCString());

      return NS_OK;
    }

    default:
      NS_ERROR("Unknown state value!");
      return NS_ERROR_UNEXPECTED;
  }

  NS_NOTREACHED("Should never get here!");
  return NS_ERROR_UNEXPECTED;
}

AsyncUsageRunnable::AsyncUsageRunnable(uint32_t aAppId,
                                       bool aInMozBrowserOnly,
                                       const nsACString& aGroup,
                                       const OriginOrPatternString& aOrigin,
                                       nsIURI* aURI,
                                       nsIUsageCallback* aCallback)
: mURI(aURI),
  mCallback(aCallback),
  mAppId(aAppId),
  mGroup(aGroup),
  mOrigin(aOrigin),
  mCallbackState(Pending),
  mInMozBrowserOnly(aInMozBrowserOnly)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aURI, "Null pointer!");
  NS_ASSERTION(!aGroup.IsEmpty(), "Empty group!");
  NS_ASSERTION(aOrigin.IsOrigin(), "Expect origin only here!");
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");
  NS_ASSERTION(aCallback, "Null pointer!");
}

nsresult
AsyncUsageRunnable::TakeShortcut()
{
  NS_ASSERTION(mCallbackState == Pending, "Huh?");

  nsresult rv = NS_DispatchToCurrentThread(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mCallbackState = Shortcut;
  return NS_OK;
}

nsresult
AsyncUsageRunnable::RunInternal()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  nsresult rv;

  switch (mCallbackState) {
    case Pending: {
      NS_NOTREACHED("Should never get here without being dispatched!");
      return NS_ERROR_UNEXPECTED;
    }

    case OpenAllowed: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      AdvanceState();

      rv = quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to dispatch to the IO thread!");
      }

      return NS_OK;
    }

    case IO: {
      AssertIsOnIOThread();

      AdvanceState();

      
      rv = AddToUsage(quotaManager, PERSISTENCE_TYPE_PERSISTENT);
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = AddToUsage(quotaManager, PERSISTENCE_TYPE_TEMPORARY);
      NS_ENSURE_SUCCESS(rv, rv);

      
      return NS_OK;
    }

    case Complete: 
    case Shortcut: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      
      if (!mCanceled) {
        mCallback->OnUsageResult(mURI, TotalUsage(), FileUsage(), mAppId,
                                 mInMozBrowserOnly);
      }

      
      mURI = nullptr;
      mCallback = nullptr;

      
      if (mCallbackState == Complete) {
        quotaManager->AllowNextSynchronizedOp(mOrigin,
                                              Nullable<PersistenceType>(),
                                              EmptyCString());
      }

      return NS_OK;
    }

    default:
      NS_ERROR("Unknown state value!");
      return NS_ERROR_UNEXPECTED;
  }

  NS_NOTREACHED("Should never get here!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
AsyncUsageRunnable::AddToUsage(QuotaManager* aQuotaManager,
                               PersistenceType aPersistenceType)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> directory;
  nsresult rv = aQuotaManager->GetDirectoryForOrigin(aPersistenceType, mOrigin,
                                                     getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = directory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (exists && !mCanceled) {
    bool initialized;

    if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT) {
      initialized = aQuotaManager->mInitializedOrigins.Contains(mOrigin);

      if (!initialized) {
        rv = MaybeUpgradeOriginDirectory(directory);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      NS_ASSERTION(aPersistenceType == PERSISTENCE_TYPE_TEMPORARY, "Huh?");
      initialized = aQuotaManager->mTemporaryStorageInitialized;
    }

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) &&
           hasMore && !mCanceled) {
      nsCOMPtr<nsISupports> entry;
      rv = entries->GetNext(getter_AddRefs(entry));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
      NS_ENSURE_TRUE(file, NS_NOINTERFACE);

      nsString leafName;
      rv = file->GetLeafName(leafName);
      NS_ENSURE_SUCCESS(rv, rv);

      if (leafName.EqualsLiteral(METADATA_FILE_NAME) ||
          leafName.EqualsLiteral(DSSTORE_FILE_NAME)) {
        continue;
      }

      if (!initialized) {
        bool isDirectory;
        rv = file->IsDirectory(&isDirectory);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!isDirectory) {
          NS_WARNING("Unknown file found!");
          return NS_ERROR_UNEXPECTED;
        }
      }

      Client::Type clientType;
      rv = Client::TypeFromText(leafName, clientType);
      if (NS_FAILED(rv)) {
        NS_WARNING("Unknown directory found!");
        if (!initialized) {
          return NS_ERROR_UNEXPECTED;
        }
        continue;
      }

      nsRefPtr<Client>& client = aQuotaManager->mClients[clientType];

      if (initialized) {
        rv = client->GetUsageForOrigin(aPersistenceType, mGroup, mOrigin, this);
      }
      else {
        rv = client->InitOrigin(aPersistenceType, mGroup, mOrigin, this);
      }
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED(AsyncUsageRunnable, nsRunnable, nsIQuotaRequest)

NS_IMETHODIMP
AsyncUsageRunnable::Run()
{
  PROFILER_LABEL("Quota", "AsyncUsageRunnable::Run",
    js::ProfileEntry::Category::OTHER);

  nsresult rv = RunInternal();

  if (!NS_IsMainThread()) {
    if (NS_FAILED(rv)) {
      ResetUsage();
    }

    if (NS_FAILED(NS_DispatchToMainThread(this))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
AsyncUsageRunnable::Cancel()
{
  if (mCanceled.exchange(true)) {
    NS_WARNING("Canceled more than once?!");
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

nsresult
ResetOrClearRunnable::OnExclusiveAccessAcquired()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  nsresult rv = quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


void
ResetOrClearRunnable::InvalidateOpenedStorages(
                              nsTArray<nsCOMPtr<nsIOfflineStorage> >& aStorages,
                              void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsTArray<nsCOMPtr<nsIOfflineStorage> > storages;
  storages.SwapElements(aStorages);

  for (uint32_t index = 0; index < storages.Length(); index++) {
    storages[index]->Invalidate();
  }
}

void
ResetOrClearRunnable::DeleteFiles(QuotaManager* aQuotaManager,
                                  PersistenceType aPersistenceType)
{
  AssertIsOnIOThread();
  NS_ASSERTION(aQuotaManager, "Don't pass me null!");

  nsresult rv;

  nsCOMPtr<nsIFile> directory =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = directory->InitWithPath(aQuotaManager->GetStoragePath(aPersistenceType));
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = directory->Remove(true);
  if (rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST &&
      rv != NS_ERROR_FILE_NOT_FOUND && NS_FAILED(rv)) {
    
    
    NS_ERROR("Failed to remove directory!");
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(ResetOrClearRunnable, nsRunnable)

NS_IMETHODIMP
ResetOrClearRunnable::Run()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  switch (mCallbackState) {
    case Pending: {
      NS_NOTREACHED("Should never get here without being dispatched!");
      return NS_ERROR_UNEXPECTED;
    }

    case OpenAllowed: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      AdvanceState();

      
      
      nsresult rv =
        quotaManager->AcquireExclusiveAccess(NullCString(),
                                             Nullable<PersistenceType>(), this,
                                             InvalidateOpenedStorages, nullptr);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }

    case IO: {
      AssertIsOnIOThread();

      AdvanceState();

      if (mClear) {
        DeleteFiles(quotaManager, PERSISTENCE_TYPE_PERSISTENT);
        DeleteFiles(quotaManager, PERSISTENCE_TYPE_TEMPORARY);
      }

      quotaManager->RemoveQuota();
      quotaManager->ResetOrClearCompleted();

      
      if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
        NS_WARNING("Failed to dispatch to main thread!");
        return NS_ERROR_FAILURE;
      }

      return NS_OK;
    }

    case Complete: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      
      quotaManager->AllowNextSynchronizedOp(OriginOrPatternString::FromNull(),
                                            Nullable<PersistenceType>(),
                                            EmptyCString());

      return NS_OK;
    }

    default:
      NS_ERROR("Unknown state value!");
      return NS_ERROR_UNEXPECTED;
  }

  NS_NOTREACHED("Should never get here!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
FinalizeOriginEvictionRunnable::Run()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  nsresult rv;

  switch (mCallbackState) {
    case Pending: {
      NS_NOTREACHED("Should never get here without being dispatched!");
      return NS_ERROR_UNEXPECTED;
    }

    case OpenAllowed: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      AdvanceState();

      rv = quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to dispatch to the IO thread!");
      }

      return NS_OK;
    }

    case IO: {
      AssertIsOnIOThread();

      AdvanceState();

      for (uint32_t index = 0; index < mOrigins.Length(); index++) {
        quotaManager->OriginClearCompleted(
                            PERSISTENCE_TYPE_TEMPORARY,
                            OriginOrPatternString::FromOrigin(mOrigins[index]));
      }

      if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
        NS_WARNING("Failed to dispatch to main thread!");
        return NS_ERROR_FAILURE;
      }

      return NS_OK;
    }

    case Complete: {
      NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

      for (uint32_t index = 0; index < mOrigins.Length(); index++) {
        quotaManager->AllowNextSynchronizedOp(
                          OriginOrPatternString::FromOrigin(mOrigins[index]),
                          Nullable<PersistenceType>(PERSISTENCE_TYPE_TEMPORARY),
                          EmptyCString());
      }

      return NS_OK;
    }

    default:
      NS_ERROR("Unknown state value!");
      return NS_ERROR_UNEXPECTED;
  }

  NS_NOTREACHED("Should never get here!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
FinalizeOriginEvictionRunnable::Dispatch()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mCallbackState == Pending, "Huh?");

  mCallbackState = OpenAllowed;
  return NS_DispatchToMainThread(this);
}

nsresult
FinalizeOriginEvictionRunnable::RunImmediately()
{
  AssertIsOnIOThread();
  NS_ASSERTION(mCallbackState == Pending, "Huh?");

  mCallbackState = IO;
  return this->Run();
}

NS_IMETHODIMP
WaitForTransactionsToFinishRunnable::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mOp, "Null op!");
  NS_ASSERTION(mOp->mListener, "Nothing to run!");
  NS_ASSERTION(mCountdown, "Wrong countdown!");

  if (--mCountdown) {
    return NS_OK;
  }

  
  nsRefPtr<AcquireListener> listener;
  listener.swap(mOp->mListener);

  mOp = nullptr;

  nsresult rv = listener->OnExclusiveAccessAcquired();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  return NS_OK;
}

NS_IMETHODIMP
WaitForFileHandlesToFinishRunnable::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  mBusy = false;

  return NS_OK;
}

NS_IMETHODIMP
SaveOriginAccessTimeRunnable::Run()
{
  AssertIsOnIOThread();

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "This should never fail!");

  nsCOMPtr<nsIFile> directory;
  nsresult rv =
    quotaManager->GetDirectoryForOrigin(PERSISTENCE_TYPE_TEMPORARY, mOrigin,
                                        getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBinaryOutputStream> stream;
  rv = GetDirectoryMetadataStream(directory, true, getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (stream) {
    rv = stream->Write64(mTimestamp);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
