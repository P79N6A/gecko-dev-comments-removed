





#include "QuotaManager.h"

#include "mozIApplicationClearPrivateDataParams.h"
#include "nsIBinaryInputStream.h"
#include "nsIBinaryOutputStream.h"
#include "nsIFile.h"
#include "nsIIdleService.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
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
#include "mozilla/dom/PContent.h"
#include "mozilla/dom/asmjscache/AsmJSCache.h"
#include "mozilla/dom/cache/QuotaClient.h"
#include "mozilla/dom/indexedDB/ActorsParent.h"
#include "mozilla/IntegerRange.h"
#include "mozilla/Mutex.h"
#include "mozilla/LazyIdleThread.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/TypeTraits.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsComponentManagerUtils.h"
#include "nsAboutProtocolUtils.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsCRTGlue.h"
#include "nsDirectoryServiceUtils.h"
#include "nsEscape.h"
#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsScriptSecurityManager.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "xpcpublic.h"

#include "OriginScope.h"
#include "QuotaObject.h"
#include "UsageInfo.h"
#include "Utilities.h"



#define DEFAULT_THREAD_TIMEOUT_MS 30000



#define DEFAULT_SHUTDOWN_TIMER_MS 30000



#define PREF_FIXED_LIMIT "dom.quotaManager.temporaryStorage.fixedLimit"
#define PREF_CHUNK_SIZE "dom.quotaManager.temporaryStorage.chunkSize"


#define PREF_TESTING_FEATURES "dom.quotaManager.testing"


#define PROFILE_BEFORE_CHANGE_OBSERVER_ID "profile-before-change"



#define METADATA_FILE_NAME ".metadata"

#define PERMISSION_DEFAUT_PERSISTENT_STORAGE "default-persistent-storage"

#define KB * 1024ULL
#define MB * 1024ULL KB
#define GB * 1024ULL MB

namespace mozilla {
namespace dom {
namespace quota {

namespace {





static_assert(
  static_cast<uint32_t>(StorageType::Persistent) ==
  static_cast<uint32_t>(PERSISTENCE_TYPE_PERSISTENT),
  "Enum values should match.");

static_assert(
  static_cast<uint32_t>(StorageType::Temporary) ==
  static_cast<uint32_t>(PERSISTENCE_TYPE_TEMPORARY),
  "Enum values should match.");

static_assert(
  static_cast<uint32_t>(StorageType::Default) ==
  static_cast<uint32_t>(PERSISTENCE_TYPE_DEFAULT),
  "Enum values should match.");

const char kChromeOrigin[] = "chrome";
const char kAboutHomeOrigin[] = "moz-safe-about:home";
const char kIndexedDBOriginPrefix[] = "indexeddb://";

#define INDEXEDDB_DIRECTORY_NAME "indexedDB"
#define STORAGE_DIRECTORY_NAME "storage"
#define PERSISTENT_DIRECTORY_NAME "persistent"
#define PERMANENT_DIRECTORY_NAME "permanent"
#define TEMPORARY_DIRECTORY_NAME "temporary"
#define DEFAULT_DIRECTORY_NAME "default"

enum AppId {
  kNoAppId = nsIScriptSecurityManager::NO_APP_ID,
  kUnknownAppId = nsIScriptSecurityManager::UNKNOWN_APP_ID
};





} 

class DirectoryLockImpl final
  : public DirectoryLock
{
  nsRefPtr<QuotaManager> mQuotaManager;

  const Nullable<PersistenceType> mPersistenceType;
  const nsCString mGroup;
  const OriginScope mOriginScope;
  const Nullable<bool> mIsApp;
  const Nullable<Client::Type> mClientType;
  nsRefPtr<OpenDirectoryListener> mOpenListener;

  nsTArray<DirectoryLockImpl*> mBlocking;
  nsTArray<DirectoryLockImpl*> mBlockedOn;

  const bool mExclusive;

  
  
  const bool mInternal;

  bool mInvalidated;

public:
  DirectoryLockImpl(QuotaManager* aQuotaManager,
                    Nullable<PersistenceType> aPersistenceType,
                    const nsACString& aGroup,
                    const OriginScope& aOriginScope,
                    Nullable<bool> aIsApp,
                    Nullable<Client::Type> aClientType,
                    bool aExclusive,
                    bool aInternal,
                    OpenDirectoryListener* aOpenListener);

  static bool
  MatchOriginScopes(const OriginScope& aOriginScope1,
                    const OriginScope& aOriginScope2);

  const Nullable<PersistenceType>&
  GetPersistenceType() const
  {
    return mPersistenceType;
  }

  const nsACString&
  GetGroup() const
  {
    return mGroup;
  }

  const OriginScope&
  GetOriginScope() const
  {
    return mOriginScope;
  }

  const Nullable<bool>&
  GetIsApp() const
  {
    return mIsApp;
  }

  const Nullable<Client::Type>&
  GetClientType() const
  {
    return mClientType;
  }

  bool
  IsInternal() const
  {
    return mInternal;
  }

  bool
  ShouldUpdateLockTable()
  {
    return !mInternal &&
           mPersistenceType.Value() != PERSISTENCE_TYPE_PERSISTENT;
  }

  
  bool
  MustWaitFor(const DirectoryLockImpl& aLock);

  void
  AddBlockingLock(DirectoryLockImpl* aLock)
  {
    MOZ_ASSERT(NS_IsMainThread());

    mBlocking.AppendElement(aLock);
  }

  const nsTArray<DirectoryLockImpl*>&
  GetBlockedOnLocks()
  {
    return mBlockedOn;
  }

  void
  AddBlockedOnLock(DirectoryLockImpl* aLock)
  {
    MOZ_ASSERT(NS_IsMainThread());

    mBlockedOn.AppendElement(aLock);
  }

  void
  MaybeUnblock(DirectoryLockImpl* aLock)
  {
    MOZ_ASSERT(NS_IsMainThread());

    mBlockedOn.RemoveElement(aLock);
    if (mBlockedOn.IsEmpty()) {
      NotifyOpenListener();
    }
  }

  void
  NotifyOpenListener();

  void
  Invalidate()
  {
    MOZ_ASSERT(NS_IsMainThread());

    mInvalidated = true;
  }

private:
  ~DirectoryLockImpl();

  NS_DECL_ISUPPORTS
};

namespace {





} 

class OriginInfo final
{
  friend class GroupInfo;
  friend class QuotaManager;
  friend class QuotaObject;

public:
  OriginInfo(GroupInfo* aGroupInfo, const nsACString& aOrigin, bool aIsApp,
             uint64_t aUsage, int64_t aAccessTime)
  : mGroupInfo(aGroupInfo), mOrigin(aOrigin), mUsage(aUsage),
    mAccessTime(aAccessTime), mIsApp(aIsApp)
  {
    MOZ_COUNT_CTOR(OriginInfo);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(OriginInfo)

  int64_t
  AccessTime() const
  {
    return mAccessTime;
  }

private:
  
  ~OriginInfo()
  {
    MOZ_COUNT_DTOR(OriginInfo);

    MOZ_ASSERT(!mQuotaObjects.Count());
  }

  void
  LockedDecreaseUsage(int64_t aSize);

  void
  LockedUpdateAccessTime(int64_t aAccessTime)
  {
    AssertCurrentThreadOwnsQuotaMutex();

    mAccessTime = aAccessTime;
  }

  nsDataHashtable<nsStringHashKey, QuotaObject*> mQuotaObjects;

  GroupInfo* mGroupInfo;
  const nsCString mOrigin;
  uint64_t mUsage;
  int64_t mAccessTime;
  const bool mIsApp;
};

class OriginInfoLRUComparator
{
public:
  bool
  Equals(const OriginInfo* a, const OriginInfo* b) const
  {
    return
      a && b ? a->AccessTime() == b->AccessTime() : !a && !b ? true : false;
  }

  bool
  LessThan(const OriginInfo* a, const OriginInfo* b) const
  {
    return a && b ? a->AccessTime() < b->AccessTime() : b ? true : false;
  }
};

class GroupInfo final
{
  friend class GroupInfoPair;
  friend class OriginInfo;
  friend class QuotaManager;
  friend class QuotaObject;

public:
  GroupInfo(GroupInfoPair* aGroupInfoPair, PersistenceType aPersistenceType,
            const nsACString& aGroup)
  : mGroupInfoPair(aGroupInfoPair), mPersistenceType(aPersistenceType),
    mGroup(aGroup), mUsage(0)
  {
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

    MOZ_COUNT_CTOR(GroupInfo);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GroupInfo)

private:
  
  ~GroupInfo()
  {
    MOZ_COUNT_DTOR(GroupInfo);
  }

  already_AddRefed<OriginInfo>
  LockedGetOriginInfo(const nsACString& aOrigin);

  void
  LockedAddOriginInfo(OriginInfo* aOriginInfo);

  void
  LockedRemoveOriginInfo(const nsACString& aOrigin);

  void
  LockedRemoveOriginInfos();

  bool
  LockedHasOriginInfos()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    return !mOriginInfos.IsEmpty();
  }

  nsTArray<nsRefPtr<OriginInfo> > mOriginInfos;

  GroupInfoPair* mGroupInfoPair;
  PersistenceType mPersistenceType;
  nsCString mGroup;
  uint64_t mUsage;
};

class GroupInfoPair
{
  friend class QuotaManager;
  friend class QuotaObject;

public:
  GroupInfoPair()
  {
    MOZ_COUNT_CTOR(GroupInfoPair);
  }

  ~GroupInfoPair()
  {
    MOZ_COUNT_DTOR(GroupInfoPair);
  }

private:
  already_AddRefed<GroupInfo>
  LockedGetGroupInfo(PersistenceType aPersistenceType)
  {
    AssertCurrentThreadOwnsQuotaMutex();
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

    nsRefPtr<GroupInfo> groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    return groupInfo.forget();
  }

  void
  LockedSetGroupInfo(PersistenceType aPersistenceType, GroupInfo* aGroupInfo)
  {
    AssertCurrentThreadOwnsQuotaMutex();
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

    nsRefPtr<GroupInfo>& groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    groupInfo = aGroupInfo;
  }

  void
  LockedClearGroupInfo(PersistenceType aPersistenceType)
  {
    AssertCurrentThreadOwnsQuotaMutex();
    MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

    nsRefPtr<GroupInfo>& groupInfo =
      GetGroupInfoForPersistenceType(aPersistenceType);
    groupInfo = nullptr;
  }

  bool
  LockedHasGroupInfos()
  {
    AssertCurrentThreadOwnsQuotaMutex();

    return mTemporaryStorageGroupInfo || mDefaultStorageGroupInfo;
  }

  nsRefPtr<GroupInfo>&
  GetGroupInfoForPersistenceType(PersistenceType aPersistenceType);

  nsRefPtr<GroupInfo> mTemporaryStorageGroupInfo;
  nsRefPtr<GroupInfo> mDefaultStorageGroupInfo;
};

namespace {

class CollectOriginsHelper final
  : public nsRunnable
{
  uint64_t mMinSizeToBeFreed;

  Mutex& mMutex;
  CondVar mCondVar;

  
  nsTArray<nsRefPtr<DirectoryLockImpl>> mLocks;
  uint64_t mSizeToBeFreed;
  bool mWaiting;

public:
  CollectOriginsHelper(mozilla::Mutex& aMutex,
                       uint64_t aMinSizeToBeFreed);

  
  
  int64_t
  BlockAndReturnOriginsForEviction(
                                 nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks);

private:
  ~CollectOriginsHelper()
  { }

  NS_IMETHOD
  Run();
};

class OriginOperationBase
  : public nsRunnable
{
protected:
  enum State {
    
    State_Initial,

    
    State_DirectoryOpenPending,

    
    State_DirectoryWorkOpen,

    
    State_UnblockingOpen,

    State_Complete
  };

  State mState;
  nsresult mResultCode;

protected:
  OriginOperationBase()
    : mState(State_Initial)
    , mResultCode(NS_OK)
  { }

  
  virtual ~OriginOperationBase()
  {
    MOZ_ASSERT(mState == State_Complete);
  }

  void
  AdvanceState()
  {
    switch (mState) {
      case State_Initial:
        mState = State_DirectoryOpenPending;
        return;
      case State_DirectoryOpenPending:
        mState = State_DirectoryWorkOpen;
        return;
      case State_DirectoryWorkOpen:
        mState = State_UnblockingOpen;
        return;
      case State_UnblockingOpen:
        mState = State_Complete;
        return;
      default:
        MOZ_CRASH("Bad state!");
    }
  }

  NS_IMETHOD
  Run();

  virtual nsresult
  Open() = 0;

  nsresult
  DirectoryOpen();

  virtual nsresult
  DoDirectoryWork(QuotaManager* aQuotaManager) = 0;

  void
  Finish(nsresult aResult);

  virtual void
  UnblockOpen() = 0;

private:
  nsresult
  DirectoryWork();
};

class NormalOriginOperationBase
  : public OriginOperationBase
  , public OpenDirectoryListener
{
  nsRefPtr<DirectoryLock> mDirectoryLock;

protected:
  Nullable<PersistenceType> mPersistenceType;
  OriginScope mOriginScope;
  const bool mExclusive;

public:
  void
  RunImmediately()
  {
    MOZ_ASSERT(mState == State_Initial);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(this->Run()));
  }

protected:
  NormalOriginOperationBase(Nullable<PersistenceType> aPersistenceType,
                            const OriginScope& aOriginScope,
                            bool aExclusive)
    : mPersistenceType(aPersistenceType)
    , mOriginScope(aOriginScope)
    , mExclusive(aExclusive)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  ~NormalOriginOperationBase()
  { }

private:
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult
  Open() override;

  virtual void
  UnblockOpen() override;

  
  virtual void
  DirectoryLockAcquired(DirectoryLock* aLock) override;

  virtual void
  DirectoryLockFailed() override;

  
  virtual void
  SendResults() = 0;
};

class SaveOriginAccessTimeOp
  : public NormalOriginOperationBase
{
  int64_t mTimestamp;

public:
  SaveOriginAccessTimeOp(PersistenceType aPersistenceType,
                         const nsACString& aOrigin,
                         int64_t aTimestamp)
    : NormalOriginOperationBase(Nullable<PersistenceType>(aPersistenceType),
                                OriginScope::FromOrigin(aOrigin),
                                 false)
    , mTimestamp(aTimestamp)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

private:
  ~SaveOriginAccessTimeOp()
  { }

  virtual nsresult
  DoDirectoryWork(QuotaManager* aQuotaManager) override;

  virtual void
  SendResults()
  { }
};

class GetUsageOp
  : public NormalOriginOperationBase
  , public nsIQuotaRequest
{
  UsageInfo mUsageInfo;

  const nsCString mGroup;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIUsageCallback> mCallback;
  const uint32_t mAppId;
  const bool mIsApp;
  const bool mInMozBrowserOnly;

public:
  GetUsageOp(const nsACString& aGroup,
             const nsACString& aOrigin,
             bool aIsApp,
             nsIURI* aURI,
             nsIUsageCallback* aCallback,
             uint32_t aAppId,
             bool aInMozBrowserOnly);

private:
  ~GetUsageOp()
  { }

  nsresult
  AddToUsage(QuotaManager* aQuotaManager,
             PersistenceType aPersistenceType);

  virtual nsresult
  DoDirectoryWork(QuotaManager* aQuotaManager) override;

  virtual void
  SendResults();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIQUOTAREQUEST
};

class ResetOrClearOp
  : public NormalOriginOperationBase
{
  bool mClear;

public:
  explicit ResetOrClearOp(bool aClear)
    : NormalOriginOperationBase(Nullable<PersistenceType>(),
                                OriginScope::FromNull(),
                                 true)
    , mClear(aClear)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

private:
  ~ResetOrClearOp()
  { }

  void
  DeleteFiles(QuotaManager* aQuotaManager);

  virtual nsresult
  DoDirectoryWork(QuotaManager* aQuotaManager) override;

  virtual void
  SendResults()
  { }
};

class OriginClearOp
  : public NormalOriginOperationBase
{
public:
  OriginClearOp(Nullable<PersistenceType> aPersistenceType,
                const OriginScope& aOriginScope)
    : NormalOriginOperationBase(aPersistenceType,
                                aOriginScope,
                                 true)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

private:
  ~OriginClearOp()
  { }

  void
  DeleteFiles(QuotaManager* aQuotaManager,
              PersistenceType aPersistenceType);

  virtual nsresult
  DoDirectoryWork(QuotaManager* aQuotaManager) override;

  virtual void
  SendResults()
  { }
};

class FinalizeOriginEvictionOp
  : public OriginOperationBase
{
  nsTArray<nsRefPtr<DirectoryLockImpl>> mLocks;

public:
  explicit FinalizeOriginEvictionOp(
                                  nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks)
  {
    MOZ_ASSERT(!NS_IsMainThread());

    mLocks.SwapElements(aLocks);
  }

  void
  Dispatch();

  void
  RunOnIOThreadImmediately();

private:
  ~FinalizeOriginEvictionOp()
  { }

  virtual nsresult
  Open() override;

  virtual nsresult
  DoDirectoryWork(QuotaManager* aQuotaManager) override;

  virtual void
  UnblockOpen() override;
};





template <typename T, bool = mozilla::IsUnsigned<T>::value>
struct IntChecker
{
  static void
  Assert(T aInt)
  {
    static_assert(mozilla::IsIntegral<T>::value, "Not an integer!");
    MOZ_ASSERT(aInt >= 0);
  }
};

template <typename T>
struct IntChecker<T, true>
{
  static void
  Assert(T aInt)
  {
    static_assert(mozilla::IsIntegral<T>::value, "Not an integer!");
  }
};

template <typename T>
void
AssertNoOverflow(uint64_t aDest, T aArg)
{
  IntChecker<T>::Assert(aDest);
  IntChecker<T>::Assert(aArg);
  MOZ_ASSERT(UINT64_MAX - aDest >= uint64_t(aArg));
}

template <typename T, typename U>
void
AssertNoUnderflow(T aDest, U aArg)
{
  IntChecker<T>::Assert(aDest);
  IntChecker<T>::Assert(aArg);
  MOZ_ASSERT(uint64_t(aDest) >= uint64_t(aArg));
}

} 

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

void
ReportInternalError(const char* aFile, uint32_t aLine, const char* aStr)
{
  
  for (const char* p = aFile; *p; ++p) {
    if (*p == '/' && *(p + 1)) {
      aFile = p + 1;
    }
  }

  nsContentUtils::LogSimpleConsoleError(
    NS_ConvertUTF8toUTF16(nsPrintfCString(
                          "Quota %s: %s:%lu", aStr, aFile, aLine)),
    "quota");
}

namespace {

QuotaManager* gInstance = nullptr;
mozilla::Atomic<bool> gShutdown(false);


static const int32_t kDefaultFixedLimitKB = -1;
static const uint32_t kDefaultChunkSizeKB = 10 * 1024;
int32_t gFixedLimitKB = kDefaultFixedLimitKB;
uint32_t gChunkSizeKB = kDefaultChunkSizeKB;

bool gTestingEnabled = false;

class StorageDirectoryHelper final
  : public nsRunnable
{
  struct OriginProps;

  nsTArray<OriginProps> mOriginProps;

  nsCOMPtr<nsIFile> mDirectory;
  mozilla::Mutex mMutex;
  mozilla::CondVar mCondVar;
  nsresult mMainThreadResultCode;
  const bool mPersistent;
  bool mCreate;
  bool mWaiting;

public:
  StorageDirectoryHelper(nsIFile* aDirectory,
                         bool aPersistent)
    : mDirectory(aDirectory)
    , mMutex("StorageDirectoryHelper::mMutex")
    , mCondVar(mMutex, "StorageDirectoryHelper::mCondVar")
    , mMainThreadResultCode(NS_OK)
    , mPersistent(aPersistent)
    , mCreate(true)
    , mWaiting(true)
  {
    AssertIsOnIOThread();
  }

  nsresult
  CreateOrUpgradeMetadataFiles(bool aCreate);

  nsresult
  RestoreMetadataFile();

private:
  ~StorageDirectoryHelper()
  { }

  nsresult
  AddOriginDirectory(nsIFile* aDirectory);

  nsresult
  ProcessOriginDirectories(bool aMove);

  nsresult
  RunOnMainThread();

  NS_IMETHOD
  Run();
};

struct StorageDirectoryHelper::OriginProps
{
  enum Type
  {
    eChrome,
    eContent
  };

  nsCOMPtr<nsIFile> mDirectory;
  nsCString mSpec;
  uint32_t mAppId;
  int64_t mTimestamp;
  nsCString mGroup;
  nsCString mOrigin;

  Type mType;
  bool mInMozBrowser;
  bool mIsApp;

public:
  explicit OriginProps()
    : mAppId(kNoAppId)
    , mTimestamp(0)
    , mType(eContent)
    , mInMozBrowser(false)
    , mIsApp(false)
  { }
};

class MOZ_STACK_CLASS OriginParser final
{
  static bool
  IgnoreWhitespace(char16_t )
  {
    return false;
  }

  typedef nsCCharSeparatedTokenizerTemplate<IgnoreWhitespace> Tokenizer;

  enum SchemaType {
    eNone,
    eFile,
    eMozSafeAbout
  };

  enum State {
    eExpectingAppIdOrSchema,
    eExpectingInMozBrowser,
    eExpectingSchema,
    eExpectingEmptyToken1,
    eExpectingEmptyToken2,
    eExpectingEmptyToken3,
    eExpectingHost,
    eExpectingPort,
    eExpectingEmptyTokenOrDriveLetterOrPathnameComponent,
    eExpectingEmptyTokenOrPathnameComponent,
    eComplete,
    eHandledTrailingSeparator
  };

  const nsCString mOrigin;
  Tokenizer mTokenizer;

  uint32_t mAppId;
  nsCString mSchema;
  nsCString mHost;
  Nullable<uint32_t> mPort;
  nsTArray<nsCString> mPathnameComponents;
  nsCString mHandledTokens;

  SchemaType mSchemaType;
  State mState;
  bool mInMozBrowser;
  bool mMaybeDriveLetter;
  bool mError;

public:
  explicit OriginParser(const nsACString& aOrigin)
    : mOrigin(aOrigin)
    , mTokenizer(aOrigin, '+')
    , mAppId(kNoAppId)
    , mPort()
    , mSchemaType(eNone)
    , mState(eExpectingAppIdOrSchema)
    , mInMozBrowser(false)
    , mMaybeDriveLetter(false)
    , mError(false)
  { }

  static bool
  ParseOrigin(const nsACString& aOrigin,
              uint32_t* aAppId,
              bool* aInMozBrowser,
              nsCString& aSpec);

  bool
  Parse(uint32_t* aAppId,
        bool* aInMozBrowser,
        nsACString& aSpec);

private:
  void
  HandleSchema(const nsDependentCSubstring& aSchema);

  void
  HandlePathnameComponent(const nsDependentCSubstring& aSchema);

  void
  HandleToken(const nsDependentCSubstring& aToken);

  void
  HandleTrailingSeparator();
};

class OriginKey : public nsAutoCString
{
public:
  OriginKey(PersistenceType aPersistenceType,
            const nsACString& aOrigin)
  {
    PersistenceTypeToText(aPersistenceType, *this);
    Append(':');
    Append(aOrigin);
  }
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

bool
IsTreatedAsPersistent(PersistenceType aPersistenceType,
                      bool aIsApp)
{
  if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT ||
      (aPersistenceType == PERSISTENCE_TYPE_DEFAULT && aIsApp)) {
    return true;
  }

  return false;
}

bool
IsTreatedAsTemporary(PersistenceType aPersistenceType,
                     bool aIsApp)
{
  return !IsTreatedAsPersistent(aPersistenceType, aIsApp);
}

nsresult
CloneStoragePath(nsIFile* aBaseDir,
                 const nsAString& aStorageName,
                 nsAString& aStoragePath)
{
  nsresult rv;

  nsCOMPtr<nsIFile> storageDir;
  rv = aBaseDir->Clone(getter_AddRefs(storageDir));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = storageDir->Append(aStorageName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = storageDir->GetPath(aStoragePath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
GetLastModifiedTime(nsIFile* aFile, int64_t* aTimestamp)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aFile);
  MOZ_ASSERT(aTimestamp);

  class MOZ_STACK_CLASS Helper final
  {
  public:
    static nsresult
    GetLastModifiedTime(nsIFile* aFile, int64_t* aTimestamp)
    {
      AssertIsOnIOThread();
      MOZ_ASSERT(aFile);
      MOZ_ASSERT(aTimestamp);

      bool isDirectory;
      nsresult rv = aFile->IsDirectory(&isDirectory);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      if (!isDirectory) {
        nsString leafName;
        rv = aFile->GetLeafName(leafName);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        if (leafName.EqualsLiteral(METADATA_FILE_NAME) ||
            leafName.EqualsLiteral(DSSTORE_FILE_NAME)) {
          return NS_OK;
        }

        int64_t timestamp;
        rv = aFile->GetLastModifiedTime(&timestamp);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        
        MOZ_ASSERT((INT64_MAX / PR_USEC_PER_MSEC) > timestamp);
        timestamp *= int64_t(PR_USEC_PER_MSEC);

        if (timestamp > *aTimestamp) {
          *aTimestamp = timestamp;
        }
        return NS_OK;
      }

      nsCOMPtr<nsISimpleEnumerator> entries;
      rv = aFile->GetDirectoryEntries(getter_AddRefs(entries));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      bool hasMore;
      while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
        nsCOMPtr<nsISupports> entry;
        rv = entries->GetNext(getter_AddRefs(entry));
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
        MOZ_ASSERT(file);

        rv = GetLastModifiedTime(file, aTimestamp);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      return NS_OK;
    }
  };

  int64_t timestamp = INT64_MIN;
  nsresult rv = Helper::GetLastModifiedTime(aFile, &timestamp);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  *aTimestamp = timestamp;
  return NS_OK;
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

enum FileFlag {
  kTruncateFileFlag,
  kUpdateFileFlag,
  kAppendFileFlag
};

nsresult
GetDirectoryMetadataOutputStream(nsIFile* aDirectory, FileFlag aFileFlag,
                                 nsIBinaryOutputStream** aStream)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> metadataFile;
  nsresult rv = aDirectory->Clone(getter_AddRefs(metadataFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = metadataFile->Append(NS_LITERAL_STRING(METADATA_FILE_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIOutputStream> outputStream;
  switch (aFileFlag) {
    case kTruncateFileFlag: {
      rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream),
                                       metadataFile);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      break;
    }

    case kUpdateFileFlag: {
      bool exists;
      rv = metadataFile->Exists(&exists);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      if (!exists) {
        *aStream = nullptr;
        return NS_OK;
      }

      nsCOMPtr<nsIFileStream> stream;
      rv = NS_NewLocalFileStream(getter_AddRefs(stream), metadataFile);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      outputStream = do_QueryInterface(stream);
      if (NS_WARN_IF(!outputStream)) {
        return NS_ERROR_FAILURE;
      }

      break;
    }

    case kAppendFileFlag: {
      rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream),
                                       metadataFile,
                                       PR_WRONLY | PR_CREATE_FILE | PR_APPEND);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      break;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }

  nsCOMPtr<nsIBinaryOutputStream> binaryStream =
    do_CreateInstance("@mozilla.org/binaryoutputstream;1");
  if (NS_WARN_IF(!binaryStream)) {
    return NS_ERROR_FAILURE;
  }

  rv = binaryStream->SetOutputStream(outputStream);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  binaryStream.forget(aStream);
  return NS_OK;
}

nsresult
CreateDirectoryMetadata(nsIFile* aDirectory, int64_t aTimestamp,
                        const nsACString& aGroup, const nsACString& aOrigin,
                        bool aIsApp)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIBinaryOutputStream> stream;
  nsresult rv =
    GetDirectoryMetadataOutputStream(aDirectory, kTruncateFileFlag,
                                     getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(stream, "This shouldn't be null!");

  rv = stream->Write64(aTimestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->WriteStringZ(PromiseFlatCString(aGroup).get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->WriteStringZ(PromiseFlatCString(aOrigin).get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->WriteBoolean(aIsApp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
RestoreDirectoryMetadata(nsIFile* aDirectory, bool aPersistent)
{
  nsRefPtr<StorageDirectoryHelper> helper =
    new StorageDirectoryHelper(aDirectory, aPersistent);

  nsresult rv = helper->RestoreMetadataFile();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
GetDirectoryMetadataInputStream(nsIFile* aDirectory,
                                nsIBinaryInputStream** aStream)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aStream);

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

  binaryStream.forget(aStream);
  return NS_OK;
}

nsresult
GetDirectoryMetadataWithRestore(nsIFile* aDirectory,
                                bool aPersistent,
                                int64_t* aTimestamp,
                                nsACString& aGroup,
                                nsACString& aOrigin,
                                bool* aIsApp)
{
  nsresult rv = QuotaManager::GetDirectoryMetadata(aDirectory,
                                                   aTimestamp,
                                                   aGroup,
                                                   aOrigin,
                                                   aIsApp);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    rv = RestoreDirectoryMetadata(aDirectory, aPersistent);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = QuotaManager::GetDirectoryMetadata(aDirectory,
                                            aTimestamp,
                                            aGroup,
                                            aOrigin,
                                            aIsApp);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

nsresult
GetDirectoryMetadata(nsIFile* aDirectory, int64_t* aTimestamp)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aTimestamp);

  nsCOMPtr<nsIBinaryInputStream> binaryStream;
  nsresult rv =
    GetDirectoryMetadataInputStream(aDirectory, getter_AddRefs(binaryStream));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  uint64_t timestamp;
  rv = binaryStream->Read64(&timestamp);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  *aTimestamp = timestamp;
  return NS_OK;
}

nsresult
GetDirectoryMetadataWithRestore(nsIFile* aDirectory,
                                bool aPersistent,
                                int64_t* aTimestamp)
{
  nsresult rv = GetDirectoryMetadata(aDirectory, aTimestamp);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    rv = RestoreDirectoryMetadata(aDirectory, aPersistent);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = GetDirectoryMetadata(aDirectory, aTimestamp);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

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





DirectoryLockImpl::DirectoryLockImpl(QuotaManager* aQuotaManager,
                                     Nullable<PersistenceType> aPersistenceType,
                                     const nsACString& aGroup,
                                     const OriginScope& aOriginScope,
                                     Nullable<bool> aIsApp,
                                     Nullable<Client::Type> aClientType,
                                     bool aExclusive,
                                     bool aInternal,
                                     OpenDirectoryListener* aOpenListener)
  : mQuotaManager(aQuotaManager)
  , mPersistenceType(aPersistenceType)
  , mGroup(aGroup)
  , mOriginScope(aOriginScope)
  , mIsApp(aIsApp)
  , mClientType(aClientType)
  , mOpenListener(aOpenListener)
  , mExclusive(aExclusive)
  , mInternal(aInternal)
  , mInvalidated(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aQuotaManager);
  MOZ_ASSERT_IF(!aInternal, !aPersistenceType.IsNull());
  MOZ_ASSERT_IF(!aInternal,
                aPersistenceType.Value() != PERSISTENCE_TYPE_INVALID);
  MOZ_ASSERT_IF(!aInternal, !aGroup.IsEmpty());
  MOZ_ASSERT_IF(aInternal, !aOriginScope.IsEmpty() || aOriginScope.IsNull());
  MOZ_ASSERT_IF(!aInternal, aOriginScope.IsOrigin());
  MOZ_ASSERT_IF(!aInternal, !aOriginScope.IsEmpty());
  MOZ_ASSERT_IF(!aInternal, !aIsApp.IsNull());
  MOZ_ASSERT_IF(!aInternal, !aClientType.IsNull());
  MOZ_ASSERT_IF(!aInternal, aClientType.Value() != Client::TYPE_MAX);
  MOZ_ASSERT_IF(!aInternal, aOpenListener);
}

DirectoryLockImpl::~DirectoryLockImpl()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mQuotaManager);

  for (DirectoryLockImpl* blockingLock : mBlocking) {
    blockingLock->MaybeUnblock(this);
  }

  mBlocking.Clear();

  mQuotaManager->UnregisterDirectoryLock(this);
}


bool
DirectoryLockImpl::MatchOriginScopes(const OriginScope& aOriginScope1,
                                     const OriginScope& aOriginScope2)
{
  MOZ_ASSERT(NS_IsMainThread());

  bool match;

  if (aOriginScope2.IsNull() || aOriginScope1.IsNull()) {
    match = true;
  } else if (aOriginScope2.IsOrigin()) {
    if (aOriginScope1.IsOrigin()) {
      match = aOriginScope2.Equals(aOriginScope1);
    } else {
      match = PatternMatchesOrigin(aOriginScope1, aOriginScope2);
    }
  } else if (aOriginScope1.IsOrigin()) {
    match = PatternMatchesOrigin(aOriginScope2, aOriginScope1);
  } else {
    match = PatternMatchesOrigin(aOriginScope1, aOriginScope2) ||
            PatternMatchesOrigin(aOriginScope2, aOriginScope1);
  }

  return match;
}

bool
DirectoryLockImpl::MustWaitFor(const DirectoryLockImpl& aExistingLock)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (!aExistingLock.mExclusive && !mExclusive) {
    return false;
  }

  
  if (!aExistingLock.mPersistenceType.IsNull() && !mPersistenceType.IsNull() &&
      aExistingLock.mPersistenceType.Value() != mPersistenceType.Value()) {
    return false;
  }

  
  bool match = MatchOriginScopes(mOriginScope, aExistingLock.mOriginScope);
  if (!match) {
    return false;
  }

  
  if (!aExistingLock.mClientType.IsNull() && !mClientType.IsNull() &&
      aExistingLock.mClientType.Value() != mClientType.Value()) {
    return false;
  }

  
  
  return true;
}

void
DirectoryLockImpl::NotifyOpenListener()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mQuotaManager);
  MOZ_ASSERT(mOpenListener);

  if (mInvalidated) {
    mOpenListener->DirectoryLockFailed();
  } else {
    mOpenListener->DirectoryLockAcquired(this);
  }

  mOpenListener = nullptr;

  mQuotaManager->RemovePendingDirectoryLock(this);
}

NS_IMPL_ISUPPORTS0(DirectoryLockImpl);





void
QuotaObject::AddRef()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  if (!quotaManager) {
    NS_ERROR("Null quota manager, this shouldn't happen, possible leak!");

    ++mRefCnt;

    return;
  }

  MutexAutoLock lock(quotaManager->mQuotaMutex);

  ++mRefCnt;
}

void
QuotaObject::Release()
{
  QuotaManager* quotaManager = QuotaManager::Get();
  if (!quotaManager) {
    NS_ERROR("Null quota manager, this shouldn't happen, possible leak!");

    nsrefcnt count = --mRefCnt;
    if (count == 0) {
      mRefCnt = 1;
      delete this;
    }

    return;
  }

  {
    MutexAutoLock lock(quotaManager->mQuotaMutex);

    --mRefCnt;

    if (mRefCnt > 0) {
      return;
    }

    if (mOriginInfo) {
      mOriginInfo->mQuotaObjects.Remove(mPath);
    }
  }

  delete this;
}

bool
QuotaObject::MaybeUpdateSize(int64_t aSize, bool aTruncate)
{
  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  MutexAutoLock lock(quotaManager->mQuotaMutex);

  if (mSize == aSize) {
    return true;
  }

  if (!mOriginInfo) {
    mSize = aSize;
    return true;
  }

  GroupInfo* groupInfo = mOriginInfo->mGroupInfo;
  MOZ_ASSERT(groupInfo);

  if (mSize > aSize) {
    if (aTruncate) {
      const int64_t delta = mSize - aSize;

      AssertNoUnderflow(quotaManager->mTemporaryStorageUsage, delta);
      quotaManager->mTemporaryStorageUsage -= delta;

      AssertNoUnderflow(groupInfo->mUsage, delta);
      groupInfo->mUsage -= delta;

      AssertNoUnderflow(mOriginInfo->mUsage, delta);
      mOriginInfo->mUsage -= delta;

      mSize = aSize;
    }
    return true;
  }

  MOZ_ASSERT(mSize < aSize);

  nsRefPtr<GroupInfo> complementaryGroupInfo =
    groupInfo->mGroupInfoPair->LockedGetGroupInfo(
      ComplementaryPersistenceType(groupInfo->mPersistenceType));

  uint64_t delta = aSize - mSize;

  AssertNoOverflow(mOriginInfo->mUsage, delta);
  uint64_t newUsage = mOriginInfo->mUsage + delta;

  
  

  AssertNoOverflow(groupInfo->mUsage, delta);
  uint64_t newGroupUsage = groupInfo->mUsage + delta;

  uint64_t groupUsage = groupInfo->mUsage;
  if (complementaryGroupInfo) {
    AssertNoOverflow(groupUsage, complementaryGroupInfo->mUsage);
    groupUsage += complementaryGroupInfo->mUsage;
  }

  
  
  AssertNoOverflow(groupUsage, delta);
  if (groupUsage + delta > quotaManager->GetGroupLimit()) {
    return false;
  }

  AssertNoOverflow(quotaManager->mTemporaryStorageUsage, delta);
  uint64_t newTemporaryStorageUsage = quotaManager->mTemporaryStorageUsage +
                                      delta;

  if (newTemporaryStorageUsage > quotaManager->mTemporaryStorageLimit) {
    

    nsAutoTArray<nsRefPtr<DirectoryLockImpl>, 10> locks;

    uint64_t sizeToBeFreed =
      quotaManager->LockedCollectOriginsForEviction(delta, locks);

    if (!sizeToBeFreed) {
      return false;
    }

    NS_ASSERTION(sizeToBeFreed >= delta, "Huh?");

    {
      MutexAutoUnlock autoUnlock(quotaManager->mQuotaMutex);

      for (nsRefPtr<DirectoryLockImpl>& lock : locks) {
        MOZ_ASSERT(!lock->GetPersistenceType().IsNull());
        MOZ_ASSERT(lock->GetOriginScope().IsOrigin());
        MOZ_ASSERT(!lock->GetOriginScope().IsEmpty());

        quotaManager->DeleteFilesForOrigin(lock->GetPersistenceType().Value(),
                                           lock->GetOriginScope());
      }
    }

    

    NS_ASSERTION(mOriginInfo, "How come?!");

    for (DirectoryLockImpl* lock : locks) {
      MOZ_ASSERT(!lock->GetPersistenceType().IsNull());
      MOZ_ASSERT(!lock->GetGroup().IsEmpty());
      MOZ_ASSERT(lock->GetOriginScope().IsOrigin());
      MOZ_ASSERT(!lock->GetOriginScope().IsEmpty());
      MOZ_ASSERT(lock->GetOriginScope() != mOriginInfo->mOrigin,
                 "Deleted itself!");

      quotaManager->LockedRemoveQuotaForOrigin(
                                             lock->GetPersistenceType().Value(),
                                             lock->GetGroup(),
                                             lock->GetOriginScope());
    }

    
    

    AssertNoUnderflow(aSize, mSize);
    delta = aSize - mSize;

    AssertNoOverflow(mOriginInfo->mUsage, delta);
    newUsage = mOriginInfo->mUsage + delta;

    AssertNoOverflow(groupInfo->mUsage, delta);
    newGroupUsage = groupInfo->mUsage + delta;

    groupUsage = groupInfo->mUsage;
    if (complementaryGroupInfo) {
      AssertNoOverflow(groupUsage, complementaryGroupInfo->mUsage);
      groupUsage += complementaryGroupInfo->mUsage;
    }

    AssertNoOverflow(groupUsage, delta);
    if (groupUsage + delta > quotaManager->GetGroupLimit()) {
      
      

      
      MutexAutoUnlock autoUnlock(quotaManager->mQuotaMutex);

      quotaManager->FinalizeOriginEviction(locks);

      return false;
    }

    AssertNoOverflow(quotaManager->mTemporaryStorageUsage, delta);
    newTemporaryStorageUsage = quotaManager->mTemporaryStorageUsage + delta;

    NS_ASSERTION(newTemporaryStorageUsage <=
                 quotaManager->mTemporaryStorageLimit, "How come?!");

    
    
    mOriginInfo->mUsage = newUsage;
    groupInfo->mUsage = newGroupUsage;
    quotaManager->mTemporaryStorageUsage = newTemporaryStorageUsage;;

    
    
    MOZ_ASSERT(mSize < aSize);
    mSize = aSize;

    
    
    MutexAutoUnlock autoUnlock(quotaManager->mQuotaMutex);

    quotaManager->FinalizeOriginEviction(locks);

    return true;
  }

  mOriginInfo->mUsage = newUsage;
  groupInfo->mUsage = newGroupUsage;
  quotaManager->mTemporaryStorageUsage = newTemporaryStorageUsage;

  mSize = aSize;

  return true;
}





QuotaManager::QuotaManager()
: mQuotaMutex("QuotaManager.mQuotaMutex"),
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

auto
QuotaManager::CreateDirectoryLock(Nullable<PersistenceType> aPersistenceType,
                                  const nsACString& aGroup,
                                  const OriginScope& aOriginScope,
                                  Nullable<bool> aIsApp,
                                  Nullable<Client::Type> aClientType,
                                  bool aExclusive,
                                  bool aInternal,
                                  OpenDirectoryListener* aOpenListener)
  -> already_AddRefed<DirectoryLockImpl>
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT_IF(!aInternal, !aPersistenceType.IsNull());
  MOZ_ASSERT_IF(!aInternal,
                aPersistenceType.Value() != PERSISTENCE_TYPE_INVALID);
  MOZ_ASSERT_IF(!aInternal, !aGroup.IsEmpty());
  MOZ_ASSERT_IF(aInternal, !aOriginScope.IsEmpty() || aOriginScope.IsNull());
  MOZ_ASSERT_IF(!aInternal, aOriginScope.IsOrigin());
  MOZ_ASSERT_IF(!aInternal, !aOriginScope.IsEmpty());
  MOZ_ASSERT_IF(!aInternal, !aIsApp.IsNull());
  MOZ_ASSERT_IF(!aInternal, !aClientType.IsNull());
  MOZ_ASSERT_IF(!aInternal, aClientType.Value() != Client::TYPE_MAX);
  MOZ_ASSERT_IF(!aInternal, aOpenListener);

  nsRefPtr<DirectoryLockImpl> lock = new DirectoryLockImpl(this,
                                                           aPersistenceType,
                                                           aGroup,
                                                           aOriginScope,
                                                           aIsApp,
                                                           aClientType,
                                                           aExclusive,
                                                           aInternal,
                                                           aOpenListener);

  mPendingDirectoryLocks.AppendElement(lock);

  
  bool blocked = false;
  for (uint32_t index = mDirectoryLocks.Length(); index > 0; index--) {
    DirectoryLockImpl* existingLock = mDirectoryLocks[index - 1];
    if (lock->MustWaitFor(*existingLock)) {
      existingLock->AddBlockingLock(lock);
      lock->AddBlockedOnLock(existingLock);
      blocked = true;
    }
  }

  RegisterDirectoryLock(lock);

  
  if (!blocked) {
    lock->NotifyOpenListener();
  }

  return lock.forget();
}

auto
QuotaManager::CreateDirectoryLockForEviction(PersistenceType aPersistenceType,
                                             const nsACString& aGroup,
                                             const nsACString& aOrigin,
                                             bool aIsApp)
  -> already_AddRefed<DirectoryLockImpl>
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_INVALID);
  MOZ_ASSERT(!aOrigin.IsEmpty());

  nsRefPtr<DirectoryLockImpl> lock =
    new DirectoryLockImpl(this,
                          Nullable<PersistenceType>(aPersistenceType),
                          aGroup,
                          OriginScope::FromOrigin(aOrigin),
                          Nullable<bool>(aIsApp),
                          Nullable<Client::Type>(),
                           true,
                           true,
                          nullptr);

#ifdef DEBUG
  for (uint32_t index = mDirectoryLocks.Length(); index > 0; index--) {
    DirectoryLockImpl* existingLock = mDirectoryLocks[index - 1];
    MOZ_ASSERT(!lock->MustWaitFor(*existingLock));
  }
#endif

  RegisterDirectoryLock(lock);

  return lock.forget();
}

void
QuotaManager::RegisterDirectoryLock(DirectoryLockImpl* aLock)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aLock);

  mDirectoryLocks.AppendElement(aLock);

  if (aLock->ShouldUpdateLockTable()) {
    const Nullable<PersistenceType>& persistenceType =
      aLock->GetPersistenceType();
    const OriginScope& originScope = aLock->GetOriginScope();

    MOZ_ASSERT(!persistenceType.IsNull());
    MOZ_ASSERT(!aLock->GetGroup().IsEmpty());
    MOZ_ASSERT(originScope.IsOrigin());
    MOZ_ASSERT(!originScope.IsEmpty());

    DirectoryLockTable& directoryLockTable =
      GetDirectoryLockTable(persistenceType.Value());

    nsTArray<DirectoryLockImpl*>* array;
    if (!directoryLockTable.Get(originScope, &array)) {
      array = new nsTArray<DirectoryLockImpl*>();
      directoryLockTable.Put(originScope, array);

      if (!IsShuttingDown()) {
        UpdateOriginAccessTime(persistenceType.Value(),
                               aLock->GetGroup(),
                               originScope);
      }
    }
    array->AppendElement(aLock);
  }
}

void
QuotaManager::UnregisterDirectoryLock(DirectoryLockImpl* aLock)
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ALWAYS_TRUE(mDirectoryLocks.RemoveElement(aLock));

  if (aLock->ShouldUpdateLockTable()) {
    const Nullable<PersistenceType>& persistenceType =
      aLock->GetPersistenceType();
    const OriginScope& originScope = aLock->GetOriginScope();

    MOZ_ASSERT(!persistenceType.IsNull());
    MOZ_ASSERT(!aLock->GetGroup().IsEmpty());
    MOZ_ASSERT(originScope.IsOrigin());
    MOZ_ASSERT(!originScope.IsEmpty());

    DirectoryLockTable& directoryLockTable =
      GetDirectoryLockTable(persistenceType.Value());

    nsTArray<DirectoryLockImpl*>* array;
    MOZ_ALWAYS_TRUE(directoryLockTable.Get(originScope, &array));

    MOZ_ALWAYS_TRUE(array->RemoveElement(aLock));
    if (array->IsEmpty()) {
      directoryLockTable.Remove(originScope);

      if (!IsShuttingDown()) {
        UpdateOriginAccessTime(persistenceType.Value(),
                               aLock->GetGroup(),
                               originScope);
      }
    }
  }
}

void
QuotaManager::RemovePendingDirectoryLock(DirectoryLockImpl* aLock)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aLock);

  MOZ_ALWAYS_TRUE(mPendingDirectoryLocks.RemoveElement(aLock));
}

uint64_t
QuotaManager::CollectOriginsForEviction(
                                  uint64_t aMinSizeToBeFreed,
                                  nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aLocks.IsEmpty());

  class MOZ_STACK_CLASS Closure final
  {
    nsTArray<DirectoryLockImpl*>& mTemporaryStorageLocks;
    nsTArray<DirectoryLockImpl*>& mDefaultStorageLocks;
    nsTArray<OriginInfo*>& mInactiveOriginInfos;

  public:
    Closure(nsTArray<DirectoryLockImpl*>& aTemporaryStorageLocks,
            nsTArray<DirectoryLockImpl*>& aDefaultStorageLocks,
            nsTArray<OriginInfo*>& aInactiveOriginInfos)
      : mTemporaryStorageLocks(aTemporaryStorageLocks)
      , mDefaultStorageLocks(aDefaultStorageLocks)
      , mInactiveOriginInfos(aInactiveOriginInfos)
    { }

    static PLDHashOperator
    GetInactiveTemporaryStorageOrigins(const nsACString& aKey,
                                       GroupInfoPair* aValue,
                                       void* aUserArg)
    {
      MOZ_ASSERT(!aKey.IsEmpty());
      MOZ_ASSERT(aValue);
      MOZ_ASSERT(aUserArg);

      auto* closure = static_cast<Closure*>(aUserArg);

      nsRefPtr<GroupInfo> groupInfo =
        aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
      if (groupInfo) {
        GetInactiveOriginInfos(groupInfo->mOriginInfos,
                               closure->mTemporaryStorageLocks,
                               closure->mInactiveOriginInfos);
      }

      groupInfo = aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_DEFAULT);
      if (groupInfo) {
        GetInactiveOriginInfos(groupInfo->mOriginInfos,
                               closure->mDefaultStorageLocks,
                               closure->mInactiveOriginInfos);
      }

      return PL_DHASH_NEXT;
    }

  private:
    static void
    GetInactiveOriginInfos(nsTArray<nsRefPtr<OriginInfo>>& aOriginInfos,
                           nsTArray<DirectoryLockImpl*>& aLocks,
                           nsTArray<OriginInfo*>& aInactiveOriginInfos)
    {
      for (OriginInfo* originInfo : aOriginInfos) {
        MOZ_ASSERT(IsTreatedAsTemporary(originInfo->mGroupInfo->mPersistenceType,
                                        originInfo->mIsApp));

        OriginScope originScope = OriginScope::FromOrigin(originInfo->mOrigin);

        bool match = false;
        for (uint32_t j = aLocks.Length(); j > 0; j--) {
          DirectoryLockImpl* lock = aLocks[j - 1];
          if (DirectoryLockImpl::MatchOriginScopes(originScope,
                                                   lock->GetOriginScope())) {
            match = true;
            break;
          }
        }

        if (!match) {
          MOZ_ASSERT(!originInfo->mQuotaObjects.Count(),
                     "Inactive origin shouldn't have open files!");
          aInactiveOriginInfos.InsertElementSorted(originInfo,
                                                   OriginInfoLRUComparator());
        }
      }
    }
  };

  
  
  nsTArray<DirectoryLockImpl*> temporaryStorageLocks;
  nsTArray<DirectoryLockImpl*> defaultStorageLocks;
  for (DirectoryLockImpl* lock : mDirectoryLocks) {
    const Nullable<PersistenceType>& persistenceType =
      lock->GetPersistenceType();

    if (persistenceType.IsNull()) {
      temporaryStorageLocks.AppendElement(lock);
      defaultStorageLocks.AppendElement(lock);
    } else if (persistenceType.Value() == PERSISTENCE_TYPE_TEMPORARY) {
      temporaryStorageLocks.AppendElement(lock);
    } else if (persistenceType.Value() == PERSISTENCE_TYPE_DEFAULT) {
      defaultStorageLocks.AppendElement(lock);
    } else {
      MOZ_ASSERT(persistenceType.Value() == PERSISTENCE_TYPE_PERSISTENT);

      
    }
  }

  nsTArray<OriginInfo*> inactiveOrigins;

  Closure closure(temporaryStorageLocks, defaultStorageLocks, inactiveOrigins);

  
  
  MutexAutoLock lock(mQuotaMutex);

  mGroupInfoPairs.EnumerateRead(Closure::GetInactiveTemporaryStorageOrigins,
                                &closure);

#ifdef DEBUG
  
  for (uint32_t index = inactiveOrigins.Length(); index > 1; index--) {
    MOZ_ASSERT(inactiveOrigins[index - 1]->mAccessTime >=
               inactiveOrigins[index - 2]->mAccessTime);
  }
#endif

  
  
  uint64_t sizeToBeFreed = 0;
  for(uint32_t count = inactiveOrigins.Length(), index = 0;
      index < count;
      index++) {
    if (sizeToBeFreed >= aMinSizeToBeFreed) {
      inactiveOrigins.TruncateLength(index);
      break;
    }

    sizeToBeFreed += inactiveOrigins[index]->mUsage;
  }

  if (sizeToBeFreed >= aMinSizeToBeFreed) {
    
    

    for (OriginInfo* originInfo : inactiveOrigins) {
      nsRefPtr<DirectoryLockImpl> lock =
        CreateDirectoryLockForEviction(originInfo->mGroupInfo->mPersistenceType,
                                       originInfo->mGroupInfo->mGroup,
                                       originInfo->mOrigin,
                                       originInfo->mIsApp);
      aLocks.AppendElement(lock.forget());
    }

    return sizeToBeFreed;
  }

  return 0;
}

nsresult
QuotaManager::Init()
{
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

    rv = CloneStoragePath(baseDir,
                          NS_LITERAL_STRING(INDEXEDDB_DIRECTORY_NAME),
                          mIndexedDBPath);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = baseDir->Append(NS_LITERAL_STRING(STORAGE_DIRECTORY_NAME));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = baseDir->GetPath(mStoragePath);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CloneStoragePath(baseDir,
                          NS_LITERAL_STRING(PERMANENT_DIRECTORY_NAME),
                          mPermanentStoragePath);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CloneStoragePath(baseDir,
                          NS_LITERAL_STRING(TEMPORARY_DIRECTORY_NAME),
                          mTemporaryStoragePath);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CloneStoragePath(baseDir,
                          NS_LITERAL_STRING(DEFAULT_DIRECTORY_NAME),
                          mDefaultStoragePath);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    mIOThread = new LazyIdleThread(DEFAULT_THREAD_TIMEOUT_MS,
                                   NS_LITERAL_CSTRING("Storage I/O"),
                                   LazyIdleThread::ManualShutdown);

    
    
    mShutdownTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    NS_ENSURE_TRUE(mShutdownTimer, NS_ERROR_FAILURE);
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

  static_assert(Client::IDB == 0 && Client::ASMJS == 1 && Client::DOMCACHE == 2 &&
                Client::TYPE_MAX == 3, "Fix the registration!");

  NS_ASSERTION(mClients.Capacity() == Client::TYPE_MAX,
               "Should be using an auto array with correct capacity!");

  
  mClients.AppendElement(indexedDB::CreateQuotaClient());
  mClients.AppendElement(asmjscache::CreateClient());
  mClients.AppendElement(cache::CreateQuotaClient());

  return NS_OK;
}

void
QuotaManager::InitQuotaForOrigin(PersistenceType aPersistenceType,
                                 const nsACString& aGroup,
                                 const nsACString& aOrigin,
                                 bool aIsApp,
                                 uint64_t aUsageBytes,
                                 int64_t aAccessTime)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(IsTreatedAsTemporary(aPersistenceType, aIsApp));

  MutexAutoLock lock(mQuotaMutex);

  GroupInfoPair* pair;
  if (!mGroupInfoPairs.Get(aGroup, &pair)) {
    pair = new GroupInfoPair();
    mGroupInfoPairs.Put(aGroup, pair);
    
  }

  nsRefPtr<GroupInfo> groupInfo = pair->LockedGetGroupInfo(aPersistenceType);
  if (!groupInfo) {
    groupInfo = new GroupInfo(pair, aPersistenceType, aGroup);
    pair->LockedSetGroupInfo(aPersistenceType, groupInfo);
  }

  nsRefPtr<OriginInfo> originInfo =
    new OriginInfo(groupInfo, aOrigin, aIsApp, aUsageBytes, aAccessTime);
  groupInfo->LockedAddOriginInfo(originInfo);
}

void
QuotaManager::DecreaseUsageForOrigin(PersistenceType aPersistenceType,
                                     const nsACString& aGroup,
                                     const nsACString& aOrigin,
                                     int64_t aSize)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

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
  MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

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
    int64_t timestamp = PR_Now();
    originInfo->LockedUpdateAccessTime(timestamp);

    MutexAutoUnlock autoUnlock(mQuotaMutex);

    nsRefPtr<SaveOriginAccessTimeOp> op =
      new SaveOriginAccessTimeOp(aPersistenceType, aOrigin, timestamp);

    op->RunImmediately();
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

  groupInfo = aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_DEFAULT);
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

already_AddRefed<QuotaObject>
QuotaManager::GetQuotaObject(PersistenceType aPersistenceType,
                             const nsACString& aGroup,
                             const nsACString& aOrigin,
                             nsIFile* aFile)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT) {
    return nullptr;
  }

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

  
  nsAutoCString tempStorage1;
  const nsCSubstring& group = NS_EscapeURL(aGroup, esc_Query, tempStorage1);

  nsAutoCString tempStorage2;
  const nsCSubstring& origin = NS_EscapeURL(aOrigin, esc_Query, tempStorage2);

  nsRefPtr<QuotaObject> result;
  {
    MutexAutoLock lock(mQuotaMutex);

    GroupInfoPair* pair;
    if (!mGroupInfoPairs.Get(group, &pair)) {
      return nullptr;
    }

    nsRefPtr<GroupInfo> groupInfo =
      pair->LockedGetGroupInfo(aPersistenceType);

    if (!groupInfo) {
      return nullptr;
    }

    nsRefPtr<OriginInfo> originInfo = groupInfo->LockedGetOriginInfo(origin);

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

void
QuotaManager::AbortOperationsForProcess(ContentParentId aContentParentId)
{
  MOZ_ASSERT(NS_IsMainThread());

  for (nsRefPtr<Client>& client : mClients) {
    client->AbortOperationsForProcess(aContentParentId);
  }
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
QuotaManager::InitializeRepository(PersistenceType aPersistenceType)
{
  MOZ_ASSERT(aPersistenceType == PERSISTENCE_TYPE_TEMPORARY ||
             aPersistenceType == PERSISTENCE_TYPE_DEFAULT);

  nsresult rv;

  nsCOMPtr<nsIFile> directory =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = directory->InitWithPath(GetStoragePath(aPersistenceType));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool created;
  rv = EnsureDirectory(directory, &created);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsIFile> childDirectory = do_QueryInterface(entry);
    MOZ_ASSERT(childDirectory);

    bool isDirectory;
    rv = childDirectory->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!isDirectory) {
      nsString leafName;
      rv = childDirectory->GetLeafName(leafName);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      if (leafName.EqualsLiteral(METADATA_FILE_NAME) ||
          leafName.EqualsLiteral(DSSTORE_FILE_NAME)) {
        continue;
      }

      QM_WARNING("Something (%s) in the repository that doesn't belong!",
                 NS_ConvertUTF16toUTF8(leafName).get());
      return NS_ERROR_UNEXPECTED;
    }

    int64_t timestamp;
    nsCString group;
    nsCString origin;
    bool isApp;
    rv = GetDirectoryMetadataWithRestore(childDirectory,
                                          false,
                                         &timestamp,
                                         group,
                                         origin,
                                         &isApp);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (IsTreatedAsPersistent(aPersistenceType, isApp)) {
      continue;
    }

    rv = InitializeOrigin(aPersistenceType, group, origin, isApp, timestamp,
                          childDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

namespace {





bool
MaybeRemoveCorruptDirectory(const nsAString& aLeafName, nsIFile* aDir)
{
#ifdef NIGHTLY_BUILD
  MOZ_ASSERT(aDir);

  if (aLeafName != NS_LITERAL_STRING("morgue")) {
    return false;
  }

  NS_WARNING("QuotaManager removing corrupt morgue directory!");

  nsresult rv = aDir->Remove(true );
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  return true;
#else
  return false;
#endif 
}

} 

nsresult
QuotaManager::InitializeOrigin(PersistenceType aPersistenceType,
                               const nsACString& aGroup,
                               const nsACString& aOrigin,
                               bool aIsApp,
                               int64_t aAccessTime,
                               nsIFile* aDirectory)
{
  AssertIsOnIOThread();

  nsresult rv;

  if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT) {
    rv = MaybeUpgradeOriginDirectory(aDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  bool trackQuota = IsQuotaEnforced(aPersistenceType, aOrigin, aIsApp);

  
  
  nsAutoPtr<UsageInfo> usageInfo;
  if (trackQuota) {
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

    if (MaybeRemoveCorruptDirectory(leafName, file)) {
      continue;
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

  if (trackQuota) {
    InitQuotaForOrigin(aPersistenceType, aGroup, aOrigin, aIsApp,
                       usageInfo->TotalUsage(), aAccessTime);
  }

  return NS_OK;
}

nsresult
QuotaManager::MaybeUpgradeIndexedDBDirectory()
{
  AssertIsOnIOThread();

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

  rv = persistentStorageDir->InitWithPath(mStoragePath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = persistentStorageDir->Append(NS_LITERAL_STRING(PERSISTENT_DIRECTORY_NAME));
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

  
  
  
  
  
  
  rv = indexedDBDir->MoveTo(storageDir, NS_LITERAL_STRING(PERSISTENT_DIRECTORY_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
QuotaManager::MaybeUpgradePersistentStorageDirectory()
{
  AssertIsOnIOThread();

  nsresult rv;

  nsCOMPtr<nsIFile> persistentStorageDir =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = persistentStorageDir->InitWithPath(mStoragePath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = persistentStorageDir->Append(NS_LITERAL_STRING(PERSISTENT_DIRECTORY_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool exists;
  rv = persistentStorageDir->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    
    return NS_OK;
  }

  bool isDirectory;
  rv = persistentStorageDir->IsDirectory(&isDirectory);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!isDirectory) {
    NS_WARNING("persistent entry is not a directory!");
    return NS_OK;
  }

  nsCOMPtr<nsIFile> defaultStorageDir =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = defaultStorageDir->InitWithPath(mDefaultStoragePath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = defaultStorageDir->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (exists) {
    NS_WARNING("storage/persistent shouldn't exist after the upgrade!");
    return NS_OK;
  }

  
  nsRefPtr<StorageDirectoryHelper> helper =
    new StorageDirectoryHelper(persistentStorageDir,  true);

  rv = helper->CreateOrUpgradeMetadataFiles( true);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  nsCOMPtr<nsIFile> temporaryStorageDir =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = temporaryStorageDir->InitWithPath(mTemporaryStoragePath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = temporaryStorageDir->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (exists) {
    rv = temporaryStorageDir->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (!isDirectory) {
      NS_WARNING("temporary entry is not a directory!");
      return NS_OK;
    }

    helper = new StorageDirectoryHelper(temporaryStorageDir,
                                         false);

    rv = helper->CreateOrUpgradeMetadataFiles( false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  rv = persistentStorageDir->RenameTo(nullptr, NS_LITERAL_STRING(DEFAULT_DIRECTORY_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
QuotaManager::MaybeUpgradeStorageArea()
{
  AssertIsOnIOThread();

  if (mStorageAreaInitialized) {
    return NS_OK;
  }

  nsresult rv = MaybeUpgradeIndexedDBDirectory();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = MaybeUpgradePersistentStorageDirectory();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mStorageAreaInitialized = true;

  return NS_OK;
}

void
QuotaManager::OpenDirectory(PersistenceType aPersistenceType,
                            const nsACString& aGroup,
                            const nsACString& aOrigin,
                            bool aIsApp,
                            Client::Type aClientType,
                            bool aExclusive,
                            OpenDirectoryListener* aOpenListener)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<DirectoryLockImpl> lock =
    CreateDirectoryLock(Nullable<PersistenceType>(aPersistenceType),
                        aGroup,
                        OriginScope::FromOrigin(aOrigin),
                        Nullable<bool>(aIsApp),
                        Nullable<Client::Type>(aClientType),
                        aExclusive,
                        false,
                        aOpenListener);
  MOZ_ASSERT(lock);
}

void
QuotaManager::OpenDirectoryInternal(Nullable<PersistenceType> aPersistenceType,
                                    const OriginScope& aOriginScope,
                                    bool aExclusive,
                                    OpenDirectoryListener* aOpenListener)
{
  MOZ_ASSERT(NS_IsMainThread());

  class MOZ_STACK_CLASS Helper final
  {
  public:
    static PLDHashOperator
    Enumerate(nsCStringHashKey* aKey, void* aClosure)
    {
      auto* client = static_cast<Client*>(aClosure);
      MOZ_ASSERT(client);

      client->AbortOperations(aKey->GetKey());

      return PL_DHASH_NEXT;
    }
  };

  nsRefPtr<DirectoryLockImpl> lock =
    CreateDirectoryLock(aPersistenceType,
                        EmptyCString(),
                        aOriginScope,
                        Nullable<bool>(),
                        Nullable<Client::Type>(),
                        aExclusive,
                        true,
                        aOpenListener);
  MOZ_ASSERT(lock);

  if (!aExclusive) {
    return;
  }

  
  
  nsAutoTArray<nsAutoPtr<nsTHashtable<nsCStringHashKey>>,
               Client::TYPE_MAX> origins;
  origins.SetLength(Client::TYPE_MAX);

  const nsTArray<DirectoryLockImpl*>& blockedOnLocks =
    lock->GetBlockedOnLocks();

  for (DirectoryLockImpl* blockedOnLock : blockedOnLocks) {
    blockedOnLock->Invalidate();

    if (!blockedOnLock->IsInternal()) {
      MOZ_ASSERT(!blockedOnLock->GetClientType().IsNull());
      Client::Type clientType = blockedOnLock->GetClientType().Value();
      MOZ_ASSERT(clientType < Client::TYPE_MAX);

      const OriginScope& originScope = blockedOnLock->GetOriginScope();
      MOZ_ASSERT(originScope.IsOrigin());
      MOZ_ASSERT(!originScope.IsEmpty());

      nsAutoPtr<nsTHashtable<nsCStringHashKey>>& origin = origins[clientType];
      if (!origin) {
        origin = new nsTHashtable<nsCStringHashKey>();
      }
      origin->PutEntry(originScope);
    }
  }

  for (uint32_t index : MakeRange(uint32_t(Client::TYPE_MAX))) {
    if (origins[index]) {
      origins[index]->EnumerateEntries(Helper::Enumerate, mClients[index]);
    }
  }
}

nsresult
QuotaManager::EnsureOriginIsInitialized(PersistenceType aPersistenceType,
                                        const nsACString& aGroup,
                                        const nsACString& aOrigin,
                                        bool aIsApp,
                                        nsIFile** aDirectory)
{
  AssertIsOnIOThread();

  nsresult rv = MaybeUpgradeStorageArea();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> directory;
  rv = GetDirectoryForOrigin(aPersistenceType, aOrigin,
                             getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  if (IsTreatedAsPersistent(aPersistenceType, aIsApp)) {
    if (mInitializedOrigins.Contains(OriginKey(aPersistenceType, aOrigin))) {
      directory.forget(aDirectory);
      return NS_OK;
    }
  } else if (!mTemporaryStorageInitialized) {
    rv = InitializeRepository(aPersistenceType);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      
      RemoveQuota();

      return rv;
    }

    rv = InitializeRepository(ComplementaryPersistenceType(aPersistenceType));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      
      RemoveQuota();

      return rv;
    }

    if (gFixedLimitKB >= 0) {
      mTemporaryStorageLimit = gFixedLimitKB * 1024;
    }
    else {
      nsCOMPtr<nsIFile> storageDir =
        do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = storageDir->InitWithPath(GetStoragePath());
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      rv = GetTemporaryStorageLimit(storageDir, mTemporaryStorageUsage,
                                    &mTemporaryStorageLimit);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mTemporaryStorageInitialized = true;

    CheckTemporaryStorageLimits();
  }

  int64_t timestamp;

  bool created;
  rv = EnsureDirectory(directory, &created);
  NS_ENSURE_SUCCESS(rv, rv);

  if (IsTreatedAsPersistent(aPersistenceType, aIsApp)) {
    if (created) {
      timestamp = PR_Now();

      rv = CreateDirectoryMetadata(directory, timestamp, aGroup, aOrigin,
                                   aIsApp);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      bool persistent = aPersistenceType == PERSISTENCE_TYPE_PERSISTENT;
      rv = GetDirectoryMetadataWithRestore(directory,
                                           persistent,
                                           &timestamp);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(timestamp <= PR_Now());
    }

    rv = InitializeOrigin(aPersistenceType, aGroup, aOrigin, aIsApp, timestamp,
                          directory);
    NS_ENSURE_SUCCESS(rv, rv);

    mInitializedOrigins.AppendElement(OriginKey(aPersistenceType, aOrigin));
  } else if (created) {
    timestamp = PR_Now();

    rv = CreateDirectoryMetadata(directory, timestamp, aGroup, aOrigin,
                                 aIsApp);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = InitializeOrigin(aPersistenceType, aGroup, aOrigin, aIsApp, timestamp,
                          directory);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  directory.forget(aDirectory);
  return NS_OK;
}

void
QuotaManager::OriginClearCompleted(PersistenceType aPersistenceType,
                                   const nsACString& aOrigin,
                                   bool aIsApp)
{
  AssertIsOnIOThread();

  if (IsTreatedAsPersistent(aPersistenceType, aIsApp)) {
    mInitializedOrigins.RemoveElement(OriginKey(aPersistenceType, aOrigin));
  }

  for (uint32_t index = 0; index < Client::TYPE_MAX; index++) {
    mClients[index]->OnOriginClearCompleted(aPersistenceType, aOrigin);
  }
}

void
QuotaManager::ResetOrClearCompleted()
{
  AssertIsOnIOThread();

  mInitializedOrigins.Clear();
  mTemporaryStorageInitialized = false;
  mStorageAreaInitialized = false;

  ReleaseIOThreadObjects();
}

Client*
QuotaManager::GetClient(Client::Type aClientType)
{
  MOZ_ASSERT(aClientType >= Client::IDB);
  MOZ_ASSERT(aClientType < Client::TYPE_MAX);

  return mClients.ElementAt(aClientType);
}

uint64_t
QuotaManager::GetGroupLimit() const
{
  MOZ_ASSERT(mTemporaryStorageInitialized);

  
  
  
  uint64_t x = std::min<uint64_t>(mTemporaryStorageLimit * .20, 2 GB);

  
  
  return std::min<uint64_t>(mTemporaryStorageLimit,
                            std::max<uint64_t>(x, 10 MB));
}


void
QuotaManager::GetStorageId(PersistenceType aPersistenceType,
                           const nsACString& aOrigin,
                           Client::Type aClientType,
                           nsACString& aDatabaseId)
{
  nsAutoCString str;
  str.AppendInt(aPersistenceType);
  str.Append('*');
  str.Append(aOrigin);
  str.Append('*');
  str.AppendInt(aClientType);

  aDatabaseId = str;
}


nsresult
QuotaManager::GetInfoFromURI(nsIURI* aURI,
                             uint32_t aAppId,
                             bool aInMozBrowser,
                             nsACString* aGroup,
                             nsACString* aOrigin,
                             bool* aIsApp)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aURI);

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = secMan->GetAppCodebasePrincipal(aURI, aAppId, aInMozBrowser,
                                                getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetInfoFromPrincipal(principal, aGroup, aOrigin, aIsApp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static nsresult
TryGetInfoForAboutURI(nsIPrincipal* aPrincipal,
                      nsACString& aGroup,
                      nsACString& aASCIIOrigin,
                      bool* aIsApp)
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
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isAbout) {
    return NS_ERROR_FAILURE;
  }

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

  if (aIsApp) {
    *aIsApp = false;
  }

  return NS_OK;
}


nsresult
QuotaManager::GetInfoFromPrincipal(nsIPrincipal* aPrincipal,
                                   nsACString* aGroup,
                                   nsACString* aOrigin,
                                   bool* aIsApp)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aPrincipal);

  if (aGroup && aOrigin) {
    nsresult rv =
      TryGetInfoForAboutURI(aPrincipal, *aGroup, *aOrigin, aIsApp);
    if (NS_SUCCEEDED(rv)) {
      return NS_OK;
    }
  }

  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    GetInfoForChrome(aGroup, aOrigin, aIsApp);
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
  rv = aPrincipal->GetOriginNoSuffix(origin);
  NS_ENSURE_SUCCESS(rv, rv);

  if (origin.EqualsLiteral(kChromeOrigin)) {
    NS_WARNING("Non-chrome principal can't use chrome origin!");
    return NS_ERROR_FAILURE;
  }

  nsCString jarPrefix;
  if (aGroup || aOrigin) {
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

  if (aOrigin) {
    aOrigin->Assign(jarPrefix + origin);
  }

  if (aIsApp) {
    *aIsApp = aPrincipal->GetAppStatus() !=
                nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  }

  return NS_OK;
}


nsresult
QuotaManager::GetInfoFromWindow(nsPIDOMWindow* aWindow,
                                nsACString* aGroup,
                                nsACString* aOrigin,
                                bool* aIsApp)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(sop, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();
  NS_ENSURE_TRUE(principal, NS_ERROR_FAILURE);

  nsresult rv = GetInfoFromPrincipal(principal, aGroup, aOrigin, aIsApp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


void
QuotaManager::GetInfoForChrome(nsACString* aGroup,
                               nsACString* aOrigin,
                               bool* aIsApp)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(nsContentUtils::IsCallerChrome());

  if (aGroup) {
    ChromeOrigin(*aGroup);
  }
  if (aOrigin) {
    ChromeOrigin(*aOrigin);
  }
  if (aIsApp) {
    *aIsApp = false;
  }
}


bool
QuotaManager::IsOriginWhitelistedForPersistentStorage(const nsACString& aOrigin)
{
  
  
  if (aOrigin.EqualsLiteral(kChromeOrigin) ||
      aOrigin.EqualsLiteral(kAboutHomeOrigin) ||
      StringBeginsWith(aOrigin, nsDependentCString(kIndexedDBOriginPrefix))) {
    return true;
  }

  return false;
}


bool
QuotaManager::IsFirstPromptRequired(PersistenceType aPersistenceType,
                                    const nsACString& aOrigin,
                                    bool aIsApp)
{
  if (IsTreatedAsTemporary(aPersistenceType, aIsApp)) {
    return false;
  }

  return !IsOriginWhitelistedForPersistentStorage(aOrigin);
}


bool
QuotaManager::IsQuotaEnforced(PersistenceType aPersistenceType,
                              const nsACString& aOrigin,
                              bool aIsApp)
{
  return IsTreatedAsTemporary(aPersistenceType, aIsApp);
}


void
QuotaManager::ChromeOrigin(nsACString& aOrigin)
{
  aOrigin.AssignLiteral(kChromeOrigin);
}


nsresult
QuotaManager::GetDirectoryMetadata(nsIFile* aDirectory,
                                   int64_t* aTimestamp,
                                   nsACString& aGroup,
                                   nsACString& aOrigin,
                                   bool* aIsApp)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aDirectory);
  MOZ_ASSERT(aTimestamp);

  nsCOMPtr<nsIBinaryInputStream> binaryStream;
  nsresult rv =
    GetDirectoryMetadataInputStream(aDirectory, getter_AddRefs(binaryStream));
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

  bool isApp;
  if (aIsApp) {
    rv = binaryStream->ReadBoolean(&isApp);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  *aTimestamp = timestamp;
  aGroup = group;
  aOrigin = origin;
  if (aIsApp) {
    *aIsApp = isApp;
  }
  return NS_OK;
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
  bool isApp;
  nsresult rv = GetInfoFromURI(aURI, aAppId, aInMozBrowserOnly, &group, &origin,
                               &isApp);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<GetUsageOp> op =
    new GetUsageOp(group, origin, isApp, aURI, aCallback, aAppId,
                   aInMozBrowserOnly);

  op->RunImmediately();

  op.forget(_retval);
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

  nsRefPtr<ResetOrClearOp> op = new ResetOrClearOp( true);

  op->RunImmediately();

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
                      nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString pattern;
  GetOriginPatternString(aAppId, aInMozBrowserOnly, origin, pattern);

  nsRefPtr<OriginClearOp> op =
    new OriginClearOp(persistenceType, OriginScope::FromPattern(pattern));

  op->RunImmediately();

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

  nsRefPtr<ResetOrClearOp> op = new ResetOrClearOp( false);

  op->RunImmediately();

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
      
      if (NS_FAILED(mShutdownTimer->Init(this, DEFAULT_SHUTDOWN_TIMER_MS,
                                         nsITimer::TYPE_ONE_SHOT))) {
        NS_WARNING("Failed to initialize shutdown timer!");
      }

      
      
      for (uint32_t index = 0; index < Client::TYPE_MAX; index++) {
        mClients[index]->ShutdownWorkThreads();
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

      for (nsRefPtr<DirectoryLockImpl>& lock : mPendingDirectoryLocks) {
        lock->Invalidate();
      }
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC)) {
    NS_ASSERTION(IsMainProcess(), "Should only happen in the main process!");

    NS_WARNING("Some storage operations are taking longer than expected "
               "during shutdown and will be aborted!");

    
    for (nsRefPtr<Client>& client : mClients) {
      client->AbortOperations(NullCString());
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

  if (!strcmp(aTopic, OBSERVER_TOPIC_IDLE_DAILY)) {
    for (auto& client : mClients) {
      client->PerformIdleMaintenance();
    }
    return NS_OK;
  }

  NS_NOTREACHED("Unknown topic!");
  return NS_ERROR_UNEXPECTED;
}

uint64_t
QuotaManager::LockedCollectOriginsForEviction(
                                  uint64_t aMinSizeToBeFreed,
                                  nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks)
{
  mQuotaMutex.AssertCurrentThreadOwns();

  nsRefPtr<CollectOriginsHelper> helper =
    new CollectOriginsHelper(mQuotaMutex, aMinSizeToBeFreed);

  
  
  
  
  {
    MutexAutoUnlock autoUnlock(mQuotaMutex);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(helper)));
  }

  return helper->BlockAndReturnOriginsForEviction(aLocks);
}

void
QuotaManager::LockedRemoveQuotaForOrigin(PersistenceType aPersistenceType,
                                         const nsACString& aGroup,
                                         const nsACString& aOrigin)
{
  mQuotaMutex.AssertCurrentThreadOwns();
  MOZ_ASSERT(aPersistenceType != PERSISTENCE_TYPE_PERSISTENT);

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
QuotaManager::ClearStoragesForApp(uint32_t aAppId, bool aBrowserOnly)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aAppId != kUnknownAppId, "Bad appId!");

  
  NS_ENSURE_TRUE(IsMainProcess(), NS_ERROR_NOT_AVAILABLE);

  nsAutoCString pattern;
  GetOriginPatternStringMaybeIgnoreBrowser(aAppId, aBrowserOnly, pattern);

  nsRefPtr<OriginClearOp> op =
    new OriginClearOp(Nullable<PersistenceType>(),
                      OriginScope::FromPattern(pattern));

  op->RunImmediately();

  return NS_OK;
}


PLDHashOperator
QuotaManager::GetOriginsExceedingGroupLimit(const nsACString& aKey,
                                            GroupInfoPair* aValue,
                                            void* aUserArg)
{
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  uint64_t groupUsage = 0;

  nsRefPtr<GroupInfo> temporaryGroupInfo =
    aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
  if (temporaryGroupInfo) {
    groupUsage += temporaryGroupInfo->mUsage;
  }

  nsRefPtr<GroupInfo> defaultGroupInfo =
    aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_DEFAULT);
  if (defaultGroupInfo) {
    groupUsage += defaultGroupInfo->mUsage;
  }

  if (groupUsage > 0) {
    QuotaManager* quotaManager = QuotaManager::Get();
    NS_ASSERTION(quotaManager, "Shouldn't be null!");

    if (groupUsage > quotaManager->GetGroupLimit()) {
      nsTArray<OriginInfo*>* doomedOriginInfos =
        static_cast<nsTArray<OriginInfo*>*>(aUserArg);

      nsTArray<OriginInfo*> originInfos;
      if (temporaryGroupInfo) {
        originInfos.AppendElements(temporaryGroupInfo->mOriginInfos);
      }
      if (defaultGroupInfo) {
        originInfos.AppendElements(defaultGroupInfo->mOriginInfos);
      }
      originInfos.Sort(OriginInfoLRUComparator());

      for (uint32_t i = 0; i < originInfos.Length(); i++) {
        OriginInfo* originInfo = originInfos[i];

        doomedOriginInfos->AppendElement(originInfo);
        groupUsage -= originInfo->mUsage;

        if (groupUsage <= quotaManager->GetGroupLimit()) {
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

  nsTArray<OriginInfo*>* originInfos =
    static_cast<nsTArray<OriginInfo*>*>(aUserArg);

  nsRefPtr<GroupInfo> groupInfo =
    aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_TEMPORARY);
  if (groupInfo) {
    originInfos->AppendElements(groupInfo->mOriginInfos);
  }

  groupInfo = aValue->LockedGetGroupInfo(PERSISTENCE_TYPE_DEFAULT);
  if (groupInfo) {
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
    OriginInfo* doomedOriginInfo = doomedOriginInfos[index];

    DeleteFilesForOrigin(doomedOriginInfo->mGroupInfo->mPersistenceType,
                         doomedOriginInfo->mOrigin);
  }

  nsTArray<OriginParams> doomedOrigins;
  {
    MutexAutoLock lock(mQuotaMutex);

    for (uint32_t index = 0; index < doomedOriginInfos.Length(); index++) {
      OriginInfo* doomedOriginInfo = doomedOriginInfos[index];

      PersistenceType persistenceType =
        doomedOriginInfo->mGroupInfo->mPersistenceType;
      nsCString group = doomedOriginInfo->mGroupInfo->mGroup;
      nsCString origin = doomedOriginInfo->mOrigin;
      bool isApp = doomedOriginInfo->mIsApp;
      LockedRemoveQuotaForOrigin(persistenceType, group, origin);

#ifdef DEBUG
      doomedOriginInfos[index] = nullptr;
#endif

      doomedOrigins.AppendElement(OriginParams(persistenceType, origin, isApp));
    }
  }

  for (const OriginParams& doomedOrigin : doomedOrigins) {
    OriginClearCompleted(doomedOrigin.mPersistenceType,
                         doomedOrigin.mOrigin,
                         doomedOrigin.mIsApp);
  }
}

void
QuotaManager::DeleteFilesForOrigin(PersistenceType aPersistenceType,
                                   const nsACString& aOrigin)
{
  nsCOMPtr<nsIFile> directory;
  nsresult rv = GetDirectoryForOrigin(aPersistenceType, aOrigin,
                                      getter_AddRefs(directory));
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = directory->Remove(true);
  if (rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST &&
      rv != NS_ERROR_FILE_NOT_FOUND && NS_FAILED(rv)) {
    
    
    NS_ERROR("Failed to remove directory!");
  }
}

void
QuotaManager::FinalizeOriginEviction(
                                  nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<FinalizeOriginEvictionOp> op =
    new FinalizeOriginEvictionOp(aLocks);

  if (IsOnIOThread()) {
    op->RunOnIOThreadImmediately();
  } else {
    op->Dispatch();
  }
}

void
QuotaManager::GetOriginPatternString(uint32_t aAppId,
                                     MozBrowserPatternFlag aBrowserFlag,
                                     const nsACString& aOrigin,
                                     nsAutoCString& _retval)
{
  NS_ASSERTION(aAppId != kUnknownAppId, "Bad appId!");
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

auto
QuotaManager::GetDirectoryLockTable(PersistenceType aPersistenceType)
  -> DirectoryLockTable&
{
  switch (aPersistenceType) {
    case PERSISTENCE_TYPE_TEMPORARY:
      return mTemporaryDirectoryLockTable;
    case PERSISTENCE_TYPE_DEFAULT:
      return mDefaultDirectoryLockTable;

    case PERSISTENCE_TYPE_PERSISTENT:
    case PERSISTENCE_TYPE_INVALID:
    default:
      MOZ_CRASH("Bad persistence type value!");
  }
}





void
OriginInfo::LockedDecreaseUsage(int64_t aSize)
{
  AssertCurrentThreadOwnsQuotaMutex();

  AssertNoUnderflow(mUsage, aSize);
  mUsage -= aSize;

  AssertNoUnderflow(mGroupInfo->mUsage, aSize);
  mGroupInfo->mUsage -= aSize;

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  AssertNoUnderflow(quotaManager->mTemporaryStorageUsage, aSize);
  quotaManager->mTemporaryStorageUsage -= aSize;
}

already_AddRefed<OriginInfo>
GroupInfo::LockedGetOriginInfo(const nsACString& aOrigin)
{
  AssertCurrentThreadOwnsQuotaMutex();

  for (nsRefPtr<OriginInfo>& originInfo : mOriginInfos) {
    if (originInfo->mOrigin == aOrigin) {
      nsRefPtr<OriginInfo> result = originInfo;
      return result.forget();
    }
  }

  return nullptr;
}

void
GroupInfo::LockedAddOriginInfo(OriginInfo* aOriginInfo)
{
  AssertCurrentThreadOwnsQuotaMutex();

  NS_ASSERTION(!mOriginInfos.Contains(aOriginInfo),
               "Replacing an existing entry!");
  mOriginInfos.AppendElement(aOriginInfo);

  AssertNoOverflow(mUsage, aOriginInfo->mUsage);
  mUsage += aOriginInfo->mUsage;

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  AssertNoOverflow(quotaManager->mTemporaryStorageUsage, aOriginInfo->mUsage);
  quotaManager->mTemporaryStorageUsage += aOriginInfo->mUsage;
}

void
GroupInfo::LockedRemoveOriginInfo(const nsACString& aOrigin)
{
  AssertCurrentThreadOwnsQuotaMutex();

  for (uint32_t index = 0; index < mOriginInfos.Length(); index++) {
    if (mOriginInfos[index]->mOrigin == aOrigin) {
      AssertNoUnderflow(mUsage, mOriginInfos[index]->mUsage);
      mUsage -= mOriginInfos[index]->mUsage;

      QuotaManager* quotaManager = QuotaManager::Get();
      MOZ_ASSERT(quotaManager);

      AssertNoUnderflow(quotaManager->mTemporaryStorageUsage,
                        mOriginInfos[index]->mUsage);
      quotaManager->mTemporaryStorageUsage -= mOriginInfos[index]->mUsage;

      mOriginInfos.RemoveElementAt(index);

      return;
    }
  }
}

void
GroupInfo::LockedRemoveOriginInfos()
{
  AssertCurrentThreadOwnsQuotaMutex();

  QuotaManager* quotaManager = QuotaManager::Get();
  MOZ_ASSERT(quotaManager);

  for (uint32_t index = mOriginInfos.Length(); index > 0; index--) {
    OriginInfo* originInfo = mOriginInfos[index - 1];

    AssertNoUnderflow(mUsage, originInfo->mUsage);
    mUsage -= originInfo->mUsage;

    AssertNoUnderflow(quotaManager->mTemporaryStorageUsage, originInfo->mUsage);
    quotaManager->mTemporaryStorageUsage -= originInfo->mUsage;

    mOriginInfos.RemoveElementAt(index - 1);
  }
}

nsRefPtr<GroupInfo>&
GroupInfoPair::GetGroupInfoForPersistenceType(PersistenceType aPersistenceType)
{
  switch (aPersistenceType) {
    case PERSISTENCE_TYPE_TEMPORARY:
      return mTemporaryStorageGroupInfo;
    case PERSISTENCE_TYPE_DEFAULT:
      return mDefaultStorageGroupInfo;

    case PERSISTENCE_TYPE_PERSISTENT:
    case PERSISTENCE_TYPE_INVALID:
    default:
      MOZ_CRASH("Bad persistence type value!");
  }
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
                                  nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks)
{
  MOZ_ASSERT(!NS_IsMainThread(), "Wrong thread!");
  mMutex.AssertCurrentThreadOwns();

  while (mWaiting) {
    mCondVar.Wait();
  }

  mLocks.SwapElements(aLocks);
  return mSizeToBeFreed;
}

NS_IMETHODIMP
CollectOriginsHelper::Run()
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  
  
  nsTArray<nsRefPtr<DirectoryLockImpl>> locks;
  uint64_t sizeToBeFreed =
    quotaManager->CollectOriginsForEviction(mMinSizeToBeFreed, locks);

  MutexAutoLock lock(mMutex);

  NS_ASSERTION(mWaiting, "Huh?!");

  mLocks.SwapElements(locks);
  mSizeToBeFreed = sizeToBeFreed;
  mWaiting = false;
  mCondVar.Notify();

  return NS_OK;
}

NS_IMETHODIMP
OriginOperationBase::Run()
{
  nsresult rv;

  switch (mState) {
    case State_Initial: {
      rv = Open();
      break;
    }

    case State_DirectoryOpenPending: {
      rv = DirectoryOpen();
      break;
    }

    case State_DirectoryWorkOpen: {
      rv = DirectoryWork();
      break;
    }

    case State_UnblockingOpen: {
      UnblockOpen();
      return NS_OK;
    }

    default:
      MOZ_CRASH("Bad state!");
  }

  if (NS_WARN_IF(NS_FAILED(rv)) && mState != State_UnblockingOpen) {
    Finish(rv);
  }

  return NS_OK;
}

nsresult
OriginOperationBase::DirectoryOpen()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DirectoryOpenPending);

  QuotaManager* quotaManager = QuotaManager::Get();
  if (NS_WARN_IF(!quotaManager)) {
    return NS_ERROR_FAILURE;
  }

  
  AdvanceState();

  nsresult rv = quotaManager->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

void
OriginOperationBase::Finish(nsresult aResult)
{
  if (NS_SUCCEEDED(mResultCode)) {
    mResultCode = aResult;
  }

  
  
  mState = State_UnblockingOpen;

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));
}

nsresult
OriginOperationBase::DirectoryWork()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == State_DirectoryWorkOpen);

  QuotaManager* quotaManager = QuotaManager::Get();
  if (NS_WARN_IF(!quotaManager)) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = DoDirectoryWork(quotaManager);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  AdvanceState();

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(NormalOriginOperationBase, nsRunnable)

nsresult
NormalOriginOperationBase::Open()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_Initial);

  if (QuotaManager::IsShuttingDown()) {
    return NS_ERROR_FAILURE;
  }

  QuotaManager* quotaManager = QuotaManager::Get();
  if (NS_WARN_IF(!quotaManager)) {
    return NS_ERROR_FAILURE;
  }

  AdvanceState();

  quotaManager->OpenDirectoryInternal(mPersistenceType,
                                      mOriginScope,
                                      mExclusive,
                                      this);

  return NS_OK;
}

void
NormalOriginOperationBase::UnblockOpen()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_UnblockingOpen);

  SendResults();

  mDirectoryLock = nullptr;

  AdvanceState();
}

void
NormalOriginOperationBase::DirectoryLockAcquired(DirectoryLock* aLock)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aLock);
  MOZ_ASSERT(mState == State_DirectoryOpenPending);
  MOZ_ASSERT(!mDirectoryLock);

  mDirectoryLock = aLock;

  nsresult rv = DirectoryOpen();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    Finish(rv);
    return;
  }
}

void
NormalOriginOperationBase::DirectoryLockFailed()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_DirectoryOpenPending);
  MOZ_ASSERT(!mDirectoryLock);

  Finish(NS_ERROR_FAILURE);
}

nsresult
SaveOriginAccessTimeOp::DoDirectoryWork(QuotaManager* aQuotaManager)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(!mPersistenceType.IsNull());
  MOZ_ASSERT(mOriginScope.IsOrigin());

  PROFILER_LABEL("Quota", "SaveOriginAccessTimeOp::DoDirectoryWork",
                 js::ProfileEntry::Category::OTHER);

  nsCOMPtr<nsIFile> directory;
  nsresult rv =
    aQuotaManager->GetDirectoryForOrigin(mPersistenceType.Value(),
                                         mOriginScope,
                                         getter_AddRefs(directory));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIBinaryOutputStream> stream;
  rv = GetDirectoryMetadataOutputStream(directory, kUpdateFileFlag,
                                        getter_AddRefs(stream));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  if (stream) {
    rv = stream->Write64(mTimestamp);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

GetUsageOp::GetUsageOp(const nsACString& aGroup,
                       const nsACString& aOrigin,
                       bool aIsApp,
                       nsIURI* aURI,
                       nsIUsageCallback* aCallback,
                       uint32_t aAppId,
                       bool aInMozBrowserOnly)
  : NormalOriginOperationBase(Nullable<PersistenceType>(),
                              OriginScope::FromOrigin(aOrigin),
                               false)
  , mGroup(aGroup)
  , mURI(aURI)
  , mCallback(aCallback)
  , mAppId(aAppId)
  , mIsApp(aIsApp)
  , mInMozBrowserOnly(aInMozBrowserOnly)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aGroup.IsEmpty());
  MOZ_ASSERT(!aOrigin.IsEmpty());
  MOZ_ASSERT(aURI);
  MOZ_ASSERT(aCallback);
}

nsresult
GetUsageOp::AddToUsage(QuotaManager* aQuotaManager,
                       PersistenceType aPersistenceType)
{
  AssertIsOnIOThread();

  nsCOMPtr<nsIFile> directory;
  nsresult rv = aQuotaManager->GetDirectoryForOrigin(aPersistenceType,
                                                     mOriginScope,
                                                     getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = directory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (exists && !mUsageInfo.Canceled()) {
    bool initialized;

    if (IsTreatedAsPersistent(aPersistenceType, mIsApp)) {
      nsCString originKey = OriginKey(aPersistenceType, mOriginScope);
      initialized = aQuotaManager->IsOriginInitialized(originKey);
    } else {
      initialized = aQuotaManager->IsTemporaryStorageInitialized();
    }

    if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT && !initialized) {
      rv = MaybeUpgradeOriginDirectory(directory);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) &&
           hasMore && !mUsageInfo.Canceled()) {
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

      if (MaybeRemoveCorruptDirectory(leafName, file)) {
        continue;
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

      Client* client = aQuotaManager->GetClient(clientType);
      MOZ_ASSERT(client);

      if (initialized) {
        rv = client->GetUsageForOrigin(aPersistenceType, mGroup, mOriginScope,
                                       &mUsageInfo);
      }
      else {
        rv = client->InitOrigin(aPersistenceType, mGroup, mOriginScope,
                                &mUsageInfo);
      }
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
GetUsageOp::DoDirectoryWork(QuotaManager* aQuotaManager)
{
  AssertIsOnIOThread();

  PROFILER_LABEL("Quota", "GetUsageOp::DoDirectoryWork",
                 js::ProfileEntry::Category::OTHER);

  
  nsresult rv;
  for (const PersistenceType type : kAllPersistenceTypes) {
    rv = AddToUsage(aQuotaManager, type);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

void
GetUsageOp::SendResults()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (!mUsageInfo.Canceled()) {
    
    if (NS_FAILED(mResultCode)) {
      mUsageInfo.ResetUsage();
    }

    mCallback->OnUsageResult(mURI, mUsageInfo.TotalUsage(), mUsageInfo.FileUsage(), mAppId,
                             mInMozBrowserOnly);
  }

  
  mURI = nullptr;
  mCallback = nullptr;
}

NS_IMPL_ISUPPORTS_INHERITED(GetUsageOp,
                            NormalOriginOperationBase,
                            nsIQuotaRequest)

NS_IMETHODIMP
GetUsageOp::Cancel()
{
  return mUsageInfo.Cancel();
}

void
ResetOrClearOp::DeleteFiles(QuotaManager* aQuotaManager)
{
  AssertIsOnIOThread();
  NS_ASSERTION(aQuotaManager, "Don't pass me null!");

  nsresult rv;

  nsCOMPtr<nsIFile> directory =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = directory->InitWithPath(aQuotaManager->GetStoragePath());
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = directory->Remove(true);
  if (rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST &&
      rv != NS_ERROR_FILE_NOT_FOUND && NS_FAILED(rv)) {
    
    
    NS_ERROR("Failed to remove directory!");
  }
}

nsresult
ResetOrClearOp::DoDirectoryWork(QuotaManager* aQuotaManager)
{
  AssertIsOnIOThread();

  PROFILER_LABEL("Quota", "ResetOrClearOp::DoDirectoryWork",
                 js::ProfileEntry::Category::OTHER);

  if (mClear) {
    DeleteFiles(aQuotaManager);
  }

  aQuotaManager->RemoveQuota();

  aQuotaManager->ResetOrClearCompleted();

  return NS_OK;
}

void
OriginClearOp::DeleteFiles(QuotaManager* aQuotaManager,
                           PersistenceType aPersistenceType)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(aQuotaManager);

  nsresult rv;

  nsCOMPtr<nsIFile> directory =
    do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  rv = directory->InitWithPath(aQuotaManager->GetStoragePath(aPersistenceType));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  nsCOMPtr<nsISimpleEnumerator> entries;
  if (NS_WARN_IF(NS_FAILED(
        directory->GetDirectoryEntries(getter_AddRefs(entries)))) || !entries) {
    return;
  }

  nsCString originSanitized(mOriginScope);
  SanitizeOriginString(originSanitized);

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
    MOZ_ASSERT(file);

    bool isDirectory;
    rv = file->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    nsString leafName;
    rv = file->GetLeafName(leafName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    if (!isDirectory) {
      if (!leafName.EqualsLiteral(DSSTORE_FILE_NAME)) {
        QM_WARNING("Something (%s) in the repository that doesn't belong!",
                   NS_ConvertUTF16toUTF8(leafName).get());
      }
      continue;
    }

    
    if (!PatternMatchesOrigin(originSanitized,
                              NS_ConvertUTF16toUTF8(leafName))) {
      continue;
    }

    bool persistent = aPersistenceType == PERSISTENCE_TYPE_PERSISTENT;

    int64_t timestamp;
    nsCString group;
    nsCString origin;
    bool isApp;
    rv = GetDirectoryMetadataWithRestore(file,
                                         persistent,
                                         &timestamp,
                                         group,
                                         origin,
                                         &isApp);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    for (uint32_t index = 0; index < 10; index++) {
      
      if (NS_SUCCEEDED((rv = file->Remove(true)))) {
        break;
      }

      NS_WARNING("Failed to remove directory, retrying after a short delay.");

      PR_Sleep(PR_MillisecondsToInterval(200));
    }

    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to remove directory, giving up!");
    }

    if (aPersistenceType != PERSISTENCE_TYPE_PERSISTENT) {
      aQuotaManager->RemoveQuotaForOrigin(aPersistenceType, group, origin);
    }

    aQuotaManager->OriginClearCompleted(aPersistenceType, origin, isApp);
  }

}

nsresult
OriginClearOp::DoDirectoryWork(QuotaManager* aQuotaManager)
{
  AssertIsOnIOThread();

  PROFILER_LABEL("Quota", "OriginClearOp::DoDirectoryWork",
                 js::ProfileEntry::Category::OTHER);

  if (mPersistenceType.IsNull()) {
    for (const PersistenceType type : kAllPersistenceTypes) {
      DeleteFiles(aQuotaManager, type);
    }
  } else {
    DeleteFiles(aQuotaManager, mPersistenceType.Value());
  }

  return NS_OK;
}

void
FinalizeOriginEvictionOp::Dispatch()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(mState == State_Initial);

  mState = State_DirectoryOpenPending;

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));
}

void
FinalizeOriginEvictionOp::RunOnIOThreadImmediately()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == State_Initial);

  mState = State_DirectoryWorkOpen;

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(this->Run()));
}

nsresult
FinalizeOriginEvictionOp::Open()
{
  MOZ_CRASH("Shouldn't get here!");
}

nsresult
FinalizeOriginEvictionOp::DoDirectoryWork(QuotaManager* aQuotaManager)
{
  AssertIsOnIOThread();

  PROFILER_LABEL("Quota", "FinalizeOriginEvictionOp::DoDirectoryWork",
                 js::ProfileEntry::Category::OTHER);

  for (nsRefPtr<DirectoryLockImpl>& lock : mLocks) {
    aQuotaManager->OriginClearCompleted(lock->GetPersistenceType().Value(),
                                        lock->GetOriginScope(),
                                        lock->GetIsApp().Value());
  }

  return NS_OK;
}

void
FinalizeOriginEvictionOp::UnblockOpen()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == State_UnblockingOpen);

  mLocks.Clear();

  AdvanceState();
}

nsresult
StorageDirectoryHelper::CreateOrUpgradeMetadataFiles(bool aCreate)
{
  AssertIsOnIOThread();
  MOZ_ASSERT_IF(mPersistent, aCreate);

  mCreate = aCreate;

  nsCOMPtr<nsISimpleEnumerator> entries;
  nsresult rv = mDirectory->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool hasMore;
  while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
    nsCOMPtr<nsISupports> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsIFile> originDir = do_QueryInterface(entry);
    MOZ_ASSERT(originDir);

    nsString leafName;
    rv = originDir->GetLeafName(leafName);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    bool isDirectory;
    rv = originDir->IsDirectory(&isDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (isDirectory) {
      if (leafName.EqualsLiteral("moz-safe-about+++home")) {
        
        

        QM_WARNING("Deleting accidental moz-safe-about+++home directory!");

        rv = originDir->Remove( true);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        continue;
      }
    } else {
      if (!leafName.EqualsLiteral(DSSTORE_FILE_NAME)) {
        QM_WARNING("Something (%s) in the storage directory that doesn't belong!",
                   NS_ConvertUTF16toUTF8(leafName).get());

      }
      continue;
    }

    rv = AddOriginDirectory(originDir);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  if (mOriginProps.IsEmpty()) {
    return NS_OK;
  }

  rv = ProcessOriginDirectories( true);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
StorageDirectoryHelper::RestoreMetadataFile()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mCreate);

  nsresult rv = AddOriginDirectory(mDirectory);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = ProcessOriginDirectories( false);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
StorageDirectoryHelper::AddOriginDirectory(nsIFile* aDirectory)
{
  MOZ_ASSERT(aDirectory);

  nsresult rv;

  if (mPersistent) {
    rv = MaybeUpgradeOriginDirectory(aDirectory);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  nsString leafName;
  rv = aDirectory->GetLeafName(leafName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (leafName.EqualsLiteral(kChromeOrigin)) {
    OriginProps* originProps = mOriginProps.AppendElement();
    originProps->mDirectory = aDirectory;
    originProps->mSpec = kChromeOrigin;
    originProps->mType = OriginProps::eChrome;
  } else {
    nsCString spec;
    uint32_t appId;
    bool inMozBrowser;
    if (NS_WARN_IF(!OriginParser::ParseOrigin(NS_ConvertUTF16toUTF8(leafName),
                                              &appId, &inMozBrowser, spec))) {
      return NS_ERROR_FAILURE;
    }

    OriginProps* originProps = mOriginProps.AppendElement();
    originProps->mDirectory = aDirectory;
    originProps->mSpec = spec;
    originProps->mAppId = appId;
    originProps->mType = OriginProps::eContent;
    originProps->mInMozBrowser = inMozBrowser;

    if (mCreate) {
      int64_t timestamp = INT64_MIN;
      rv = GetLastModifiedTime(aDirectory, &timestamp);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      originProps->mTimestamp = timestamp;
    }
  }

  return NS_OK;
}

nsresult
StorageDirectoryHelper::ProcessOriginDirectories(bool aMove)
{
  AssertIsOnIOThread();
  MOZ_ASSERT(!mOriginProps.IsEmpty());

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));

  {
    mozilla::MutexAutoLock autolock(mMutex);
    while (mWaiting) {
      mCondVar.Wait();
    }
  }

  if (NS_WARN_IF(NS_FAILED(mMainThreadResultCode))) {
    return mMainThreadResultCode;
  }

  
  
  if (NS_WARN_IF(QuotaManager::IsShuttingDown())) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;

  nsCOMPtr<nsIFile> permanentStorageDir;

  for (uint32_t count = mOriginProps.Length(), index = 0;
       index < count;
       index++) {
    OriginProps& originProps = mOriginProps[index];

    if (mCreate) {
      rv = CreateDirectoryMetadata(originProps.mDirectory,
                                   originProps.mTimestamp,
                                   originProps.mGroup,
                                   originProps.mOrigin,
                                   originProps.mIsApp);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      
      if (mPersistent &&
          aMove &&
          QuotaManager::IsOriginWhitelistedForPersistentStorage(
                                                           originProps.mSpec)) {
        if (!permanentStorageDir) {
          permanentStorageDir =
            do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }

          QuotaManager* quotaManager = QuotaManager::Get();
          MOZ_ASSERT(quotaManager);

          const nsString& permanentStoragePath =
            quotaManager->GetStoragePath(PERSISTENCE_TYPE_PERSISTENT);

          rv = permanentStorageDir->InitWithPath(permanentStoragePath);
          if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
          }
        }

        rv = originProps.mDirectory->MoveTo(permanentStorageDir, EmptyString());
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }
      }
    } else {
      nsCOMPtr<nsIBinaryOutputStream> stream;
      rv = GetDirectoryMetadataOutputStream(originProps.mDirectory,
                                            kAppendFileFlag,
                                            getter_AddRefs(stream));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      MOZ_ASSERT(stream);

      rv = stream->WriteBoolean(originProps.mIsApp);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  return NS_OK;
}

nsresult
StorageDirectoryHelper::RunOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mOriginProps.IsEmpty());

  nsresult rv;

  nsCOMPtr<nsIScriptSecurityManager> secMan =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (uint32_t count = mOriginProps.Length(), index = 0;
       index < count;
       index++) {
    OriginProps& originProps = mOriginProps[index];

    switch (originProps.mType) {
      case OriginProps::eChrome: {
        QuotaManager::GetInfoForChrome(&originProps.mGroup,
                                       &originProps.mOrigin,
                                       &originProps.mIsApp);
        break;
      }

      case OriginProps::eContent: {
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), originProps.mSpec);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        nsCOMPtr<nsIPrincipal> principal;
        if (originProps.mAppId == kUnknownAppId) {
          rv = secMan->GetSimpleCodebasePrincipal(uri,
                                                  getter_AddRefs(principal));
        } else {
          rv = secMan->GetAppCodebasePrincipal(uri,
                                               originProps.mAppId,
                                               originProps.mInMozBrowser,
                                               getter_AddRefs(principal));
        }
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        if (mCreate) {
          rv = QuotaManager::GetInfoFromPrincipal(principal,
                                                  &originProps.mGroup,
                                                  &originProps.mOrigin,
                                                  &originProps.mIsApp);
        } else {
          rv = QuotaManager::GetInfoFromPrincipal(principal,
                                                  nullptr,
                                                  nullptr,
                                                  &originProps.mIsApp);
        }
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        break;
      }

      default:
        MOZ_CRASH("Bad type!");
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
StorageDirectoryHelper::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv = RunOnMainThread();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mMainThreadResultCode = rv;
  }

  MutexAutoLock lock(mMutex);
  MOZ_ASSERT(mWaiting);

  mWaiting = false;
  mCondVar.Notify();

  return NS_OK;
}


bool
OriginParser::ParseOrigin(const nsACString& aOrigin,
                          uint32_t* aAppId,
                          bool* aInMozBrowser,
                          nsCString& aSpec)
{
  MOZ_ASSERT(!aOrigin.IsEmpty());
  MOZ_ASSERT(aAppId);
  MOZ_ASSERT(aInMozBrowser);

  OriginParser parser(aOrigin);

  if (!parser.Parse(aAppId, aInMozBrowser, aSpec)) {
    return false;
  }

  return true;
}

bool
OriginParser::Parse(uint32_t* aAppId,
                    bool* aInMozBrowser,
                    nsACString& aSpec)
{
  MOZ_ASSERT(aAppId);
  MOZ_ASSERT(aInMozBrowser);

  while (mTokenizer.hasMoreTokens()) {
    const nsDependentCSubstring& token = mTokenizer.nextToken();

    HandleToken(token);

    if (mError) {
      break;
    }

    if (!mHandledTokens.IsEmpty()) {
      mHandledTokens.Append(NS_LITERAL_CSTRING(", "));
    }
    mHandledTokens.Append('\'');
    mHandledTokens.Append(token);
    mHandledTokens.Append('\'');
  }

  if (!mError && mTokenizer.separatorAfterCurrentToken()) {
    HandleTrailingSeparator();
  }

  if (mError) {
    QM_WARNING("Origin '%s' failed to parse, handled tokens: %s", mOrigin.get(),
               mHandledTokens.get());

    return false;
  }

  MOZ_ASSERT(mState == eComplete || mState == eHandledTrailingSeparator);

  *aAppId = mAppId;
  *aInMozBrowser = mInMozBrowser;

  nsAutoCString spec(mSchema);

  if (mSchemaType == eFile) {
    spec.AppendLiteral("://");

    for (uint32_t count = mPathnameComponents.Length(), index = 0;
         index < count;
         index++) {
      spec.Append('/');
      spec.Append(mPathnameComponents[index]);
    }

    aSpec = spec;

    return true;
  }

  if (mSchemaType == eMozSafeAbout) {
    spec.Append(':');
  } else {
    spec.AppendLiteral("://");
  }

  spec.Append(mHost);

  if (!mPort.IsNull()) {
    spec.Append(':');
    spec.AppendInt(mPort.Value());
  }

  aSpec = spec;

  return true;
}

void
OriginParser::HandleSchema(const nsDependentCSubstring& aToken)
{
  MOZ_ASSERT(!aToken.IsEmpty());
  MOZ_ASSERT(mState == eExpectingAppIdOrSchema || mState == eExpectingSchema);

  bool isMozSafeAbout = false;
  bool isFile = false;
  if (aToken.EqualsLiteral("http") ||
      aToken.EqualsLiteral("https") ||
      (isMozSafeAbout = aToken.EqualsLiteral("moz-safe-about")) ||
      aToken.EqualsLiteral("indexeddb") ||
      (isFile = aToken.EqualsLiteral("file")) ||
      aToken.EqualsLiteral("app")) {
    mSchema = aToken;

    if (isMozSafeAbout) {
      mSchemaType = eMozSafeAbout;
      mState = eExpectingHost;
    } else {
      if (isFile) {
        mSchemaType = eFile;
      }
      mState = eExpectingEmptyToken1;
    }

    return;
  }

  QM_WARNING("'%s' is not a valid schema!", nsCString(aToken).get());

  mError = true;
}

void
OriginParser::HandlePathnameComponent(const nsDependentCSubstring& aToken)
{
  MOZ_ASSERT(!aToken.IsEmpty());
  MOZ_ASSERT(mState == eExpectingEmptyTokenOrDriveLetterOrPathnameComponent ||
             mState == eExpectingEmptyTokenOrPathnameComponent);
  MOZ_ASSERT(mSchemaType == eFile);

  mPathnameComponents.AppendElement(aToken);

  mState = mTokenizer.hasMoreTokens() ? eExpectingEmptyTokenOrPathnameComponent
                                      : eComplete;
}

void
OriginParser::HandleToken(const nsDependentCSubstring& aToken)
{
  switch (mState) {
    case eExpectingAppIdOrSchema: {
      if (aToken.IsEmpty()) {
        QM_WARNING("Expected an app id or schema (not an empty string)!");

        mError = true;
        return;
      }

      if (NS_IsAsciiDigit(aToken.First())) {
        
        nsCString token(aToken);

        nsresult rv;
        uint32_t appId = token.ToInteger(&rv);
        if (NS_SUCCEEDED(rv)) {
          mAppId = appId;
          mState = eExpectingInMozBrowser;
          return;
        }
      }

      HandleSchema(aToken);

      return;
    }

    case eExpectingInMozBrowser: {
      if (aToken.Length() != 1) {
        QM_WARNING("'%d' is not a valid length for the inMozBrowser flag!",
                   aToken.Length());

        mError = true;
        return;
      }

      if (aToken.First() == 't') {
        mInMozBrowser = true;
      } else if (aToken.First() == 'f') {
        mInMozBrowser = false;
      } else {
        QM_WARNING("'%s' is not a valid value for the inMozBrowser flag!",
                   nsCString(aToken).get());

        mError = true;
        return;
      }

      mState = eExpectingSchema;

      return;
    }

    case eExpectingSchema: {
      if (aToken.IsEmpty()) {
        QM_WARNING("Expected a schema (not an empty string)!");

        mError = true;
        return;
      }

      HandleSchema(aToken);

      return;
    }

    case eExpectingEmptyToken1: {
      if (!aToken.IsEmpty()) {
        QM_WARNING("Expected the first empty token!");

        mError = true;
        return;
      }

      mState = eExpectingEmptyToken2;

      return;
    }

    case eExpectingEmptyToken2: {
      if (!aToken.IsEmpty()) {
        QM_WARNING("Expected the second empty token!");

        mError = true;
        return;
      }

      if (mSchemaType == eFile) {
        mState = eExpectingEmptyToken3;
      } else {
        mState = eExpectingHost;
      }

      return;
    }

    case eExpectingEmptyToken3: {
      MOZ_ASSERT(mSchemaType == eFile);

      if (!aToken.IsEmpty()) {
        QM_WARNING("Expected the third empty token!");

        mError = true;
        return;
      }

      mState = mTokenizer.hasMoreTokens()
                 ? eExpectingEmptyTokenOrDriveLetterOrPathnameComponent
                 : eComplete;

      return;
    }

    case eExpectingHost: {
      if (aToken.IsEmpty()) {
        QM_WARNING("Expected a host (not an empty string)!");

        mError = true;
        return;
      }

      mHost = aToken;

      mState = mTokenizer.hasMoreTokens() ? eExpectingPort : eComplete;

      return;
    }

    case eExpectingPort: {
      MOZ_ASSERT(mSchemaType == eNone);

      if (aToken.IsEmpty()) {
        QM_WARNING("Expected a port (not an empty string)!");

        mError = true;
        return;
      }

      
      nsCString token(aToken);

      nsresult rv;
      uint32_t port = token.ToInteger(&rv);
      if (NS_SUCCEEDED(rv)) {
        mPort.SetValue() = port;
      } else {
        QM_WARNING("'%s' is not a valid port number!", token.get());

        mError = true;
        return;
      }

      mState = eComplete;

      return;
    }

    case eExpectingEmptyTokenOrDriveLetterOrPathnameComponent: {
      MOZ_ASSERT(mSchemaType == eFile);

      if (aToken.IsEmpty()) {
        mPathnameComponents.AppendElement(EmptyCString());

        mState =
          mTokenizer.hasMoreTokens() ? eExpectingEmptyTokenOrPathnameComponent
                                     : eComplete;

        return;
      }

      if (aToken.Length() == 1 && NS_IsAsciiAlpha(aToken.First())) {
        mMaybeDriveLetter = true;

        mPathnameComponents.AppendElement(aToken);

        mState =
          mTokenizer.hasMoreTokens() ? eExpectingEmptyTokenOrPathnameComponent
                                     : eComplete;

        return;
      }

      HandlePathnameComponent(aToken);

      return;
    }

    case eExpectingEmptyTokenOrPathnameComponent: {
      MOZ_ASSERT(mSchemaType == eFile);

      if (aToken.IsEmpty()) {
        if (mMaybeDriveLetter) {
          MOZ_ASSERT(mPathnameComponents.Length() == 1);

          nsCString& pathnameComponent = mPathnameComponents[0];
          pathnameComponent.Append(':');

          mMaybeDriveLetter = false;
        } else {
          mPathnameComponents.AppendElement(EmptyCString());
        }

        mState =
          mTokenizer.hasMoreTokens() ? eExpectingEmptyTokenOrPathnameComponent
                                     : eComplete;

        return;
      }

      HandlePathnameComponent(aToken);

      return;
    }

    default:
      MOZ_CRASH("Should never get here!");
  }
}

void
OriginParser::HandleTrailingSeparator()
{
  MOZ_ASSERT(mState == eComplete);
  MOZ_ASSERT(mSchemaType == eFile);

  mPathnameComponents.AppendElement(EmptyCString());

  mState = eHandledTrailingSeparator;
}

} 
} 
} 
