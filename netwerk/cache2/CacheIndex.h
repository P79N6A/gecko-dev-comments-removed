



#ifndef CacheIndex__h__
#define CacheIndex__h__

#include "CacheLog.h"
#include "CacheFileIOManager.h"
#include "nsIRunnable.h"
#include "CacheHashUtils.h"
#include "nsICacheStorageService.h"
#include "nsICacheEntry.h"
#include "nsILoadContextInfo.h"
#include "nsTHashtable.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "mozilla/SHA1.h"
#include "mozilla/Mutex.h"
#include "mozilla/Endian.h"
#include "mozilla/TimeStamp.h"

class nsIFile;
class nsIDirectoryEnumerator;
class nsITimer;


#ifdef DEBUG
#define DEBUG_STATS 1
#endif

namespace mozilla {
namespace net {

class CacheFileMetadata;
class FileOpenHelper;
class CacheIndexIterator;

typedef struct {
  
  
  uint32_t mVersion;

  
  
  
  
  
  uint32_t mTimeStamp;

  
  
  
  
  uint32_t mIsDirty;
} CacheIndexHeader;

struct CacheIndexRecord {
  SHA1Sum::Hash mHash;
  uint32_t      mFrecency;
  uint32_t      mExpirationTime;
  uint32_t      mAppId;

  









  uint32_t      mFlags;

  CacheIndexRecord()
    : mFrecency(0)
    , mExpirationTime(nsICacheEntry::NO_EXPIRATION_TIME)
    , mAppId(nsILoadContextInfo::NO_APP_ID)
    , mFlags(0)
  {}
};

class CacheIndexEntry : public PLDHashEntryHdr
{
public:
  typedef const SHA1Sum::Hash& KeyType;
  typedef const SHA1Sum::Hash* KeyTypePointer;

  explicit CacheIndexEntry(KeyTypePointer aKey)
  {
    MOZ_COUNT_CTOR(CacheIndexEntry);
    mRec = new CacheIndexRecord();
    LOG(("CacheIndexEntry::CacheIndexEntry() - Created record [rec=%p]", mRec.get()));
    memcpy(&mRec->mHash, aKey, sizeof(SHA1Sum::Hash));
  }
  CacheIndexEntry(const CacheIndexEntry& aOther)
  {
    NS_NOTREACHED("CacheIndexEntry copy constructor is forbidden!");
  }
  ~CacheIndexEntry()
  {
    MOZ_COUNT_DTOR(CacheIndexEntry);
    LOG(("CacheIndexEntry::~CacheIndexEntry() - Deleting record [rec=%p]",
         mRec.get()));
  }

  
  bool KeyEquals(KeyTypePointer aKey) const
  {
    return memcmp(&mRec->mHash, aKey, sizeof(SHA1Sum::Hash)) == 0;
  }

  
  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

  
  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    return (reinterpret_cast<const uint32_t *>(aKey))[0];
  }

  
  
  enum { ALLOW_MEMMOVE = true };

  bool operator==(const CacheIndexEntry& aOther) const
  {
    return KeyEquals(&aOther.mRec->mHash);
  }

  CacheIndexEntry& operator=(const CacheIndexEntry& aOther)
  {
    MOZ_ASSERT(memcmp(&mRec->mHash, &aOther.mRec->mHash,
               sizeof(SHA1Sum::Hash)) == 0);
    mRec->mFrecency = aOther.mRec->mFrecency;
    mRec->mExpirationTime = aOther.mRec->mExpirationTime;
    mRec->mAppId = aOther.mRec->mAppId;
    mRec->mFlags = aOther.mRec->mFlags;
    return *this;
  }

  void InitNew()
  {
    mRec->mFrecency = 0;
    mRec->mExpirationTime = nsICacheEntry::NO_EXPIRATION_TIME;
    mRec->mAppId = nsILoadContextInfo::NO_APP_ID;
    mRec->mFlags = 0;
  }

  void Init(uint32_t aAppId, bool aAnonymous, bool aInBrowser)
  {
    MOZ_ASSERT(mRec->mFrecency == 0);
    MOZ_ASSERT(mRec->mExpirationTime == nsICacheEntry::NO_EXPIRATION_TIME);
    MOZ_ASSERT(mRec->mAppId == nsILoadContextInfo::NO_APP_ID);
    
    MOZ_ASSERT((mRec->mFlags & ~kDirtyMask) == kFreshMask);

    mRec->mAppId = aAppId;
    mRec->mFlags |= kInitializedMask;
    if (aAnonymous) {
      mRec->mFlags |= kAnonymousMask;
    }
    if (aInBrowser) {
      mRec->mFlags |= kInBrowserMask;
    }
  }

  const SHA1Sum::Hash * Hash() const { return &mRec->mHash; }

  bool IsInitialized() const { return !!(mRec->mFlags & kInitializedMask); }

  uint32_t AppId() const { return mRec->mAppId; }
  bool     Anonymous() const { return !!(mRec->mFlags & kAnonymousMask); }
  bool     InBrowser() const { return !!(mRec->mFlags & kInBrowserMask); }

  bool IsRemoved() const { return !!(mRec->mFlags & kRemovedMask); }
  void MarkRemoved() { mRec->mFlags |= kRemovedMask; }

  bool IsDirty() const { return !!(mRec->mFlags & kDirtyMask); }
  void MarkDirty() { mRec->mFlags |= kDirtyMask; }
  void ClearDirty() { mRec->mFlags &= ~kDirtyMask; }

  bool IsFresh() const { return !!(mRec->mFlags & kFreshMask); }
  void MarkFresh() { mRec->mFlags |= kFreshMask; }

  void     SetFrecency(uint32_t aFrecency) { mRec->mFrecency = aFrecency; }
  uint32_t GetFrecency() const { return mRec->mFrecency; }

  void     SetExpirationTime(uint32_t aExpirationTime)
  {
    mRec->mExpirationTime = aExpirationTime;
  }
  uint32_t GetExpirationTime() const { return mRec->mExpirationTime; }

  
  void     SetFileSize(uint32_t aFileSize)
  {
    if (aFileSize > kFileSizeMask) {
      LOG(("CacheIndexEntry::SetFileSize() - FileSize is too large, "
           "truncating to %u", kFileSizeMask));
      aFileSize = kFileSizeMask;
    }
    mRec->mFlags &= ~kFileSizeMask;
    mRec->mFlags |= aFileSize;
  }
  
  uint32_t GetFileSize() const { return GetFileSize(mRec); }
  static uint32_t GetFileSize(CacheIndexRecord *aRec)
  {
    return aRec->mFlags & kFileSizeMask;
  }
  bool     IsFileEmpty() const { return GetFileSize() == 0; }

  void WriteToBuf(void *aBuf)
  {
    CacheIndexRecord *dst = reinterpret_cast<CacheIndexRecord *>(aBuf);

    
    memcpy(aBuf, mRec, sizeof(CacheIndexRecord));

    
    
    dst->mFlags &= ~kDirtyMask;
    dst->mFlags &= ~kFreshMask;

#if defined(IS_LITTLE_ENDIAN)
    
    
    NetworkEndian::writeUint32(&dst->mFrecency, dst->mFrecency);
    NetworkEndian::writeUint32(&dst->mExpirationTime, dst->mExpirationTime);
    NetworkEndian::writeUint32(&dst->mAppId, dst->mAppId);
    NetworkEndian::writeUint32(&dst->mFlags, dst->mFlags);
#endif
  }

  void ReadFromBuf(void *aBuf)
  {
    CacheIndexRecord *src= reinterpret_cast<CacheIndexRecord *>(aBuf);
    MOZ_ASSERT(memcmp(&mRec->mHash, &src->mHash,
               sizeof(SHA1Sum::Hash)) == 0);

    mRec->mFrecency = NetworkEndian::readUint32(&src->mFrecency);
    mRec->mExpirationTime = NetworkEndian::readUint32(&src->mExpirationTime);
    mRec->mAppId = NetworkEndian::readUint32(&src->mAppId);
    mRec->mFlags = NetworkEndian::readUint32(&src->mFlags);
  }

  void Log() const {
    LOG(("CacheIndexEntry::Log() [this=%p, hash=%08x%08x%08x%08x%08x, fresh=%u,"
         " initialized=%u, removed=%u, dirty=%u, anonymous=%u, inBrowser=%u, "
         "appId=%u, frecency=%u, expirationTime=%u, size=%u]",
         this, LOGSHA1(mRec->mHash), IsFresh(), IsInitialized(), IsRemoved(),
         IsDirty(), Anonymous(), InBrowser(), AppId(), GetFrecency(),
         GetExpirationTime(), GetFileSize()));
  }

  static bool RecordMatchesLoadContextInfo(CacheIndexRecord *aRec,
                                           nsILoadContextInfo *aInfo)
  {
    if (!aInfo->IsPrivate() &&
        aInfo->AppId() == aRec->mAppId &&
        aInfo->IsAnonymous() == !!(aRec->mFlags & kAnonymousMask) &&
        aInfo->IsInBrowserElement() == !!(aRec->mFlags & kInBrowserMask)) {
      return true;
    }

    return false;
  }

  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
  {
    return mallocSizeOf(mRec.get());
  }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
  {
    return mallocSizeOf(this) + SizeOfExcludingThis(mallocSizeOf);
  }

private:
  friend class CacheIndexEntryUpdate;
  friend class CacheIndex;
  friend class CacheIndexEntryAutoManage;

  static const uint32_t kInitializedMask = 0x80000000;
  static const uint32_t kAnonymousMask   = 0x40000000;
  static const uint32_t kInBrowserMask   = 0x20000000;

  
  
  static const uint32_t kRemovedMask     = 0x10000000;

  
  
  static const uint32_t kDirtyMask       = 0x08000000;

  
  
  
  static const uint32_t kFreshMask       = 0x04000000;

  static const uint32_t kReservedMask    = 0x03000000;

  
  static const uint32_t kFileSizeMask    = 0x00FFFFFF;

  nsAutoPtr<CacheIndexRecord> mRec;
};

class CacheIndexEntryUpdate : public CacheIndexEntry
{
public:
  explicit CacheIndexEntryUpdate(CacheIndexEntry::KeyTypePointer aKey)
    : CacheIndexEntry(aKey)
    , mUpdateFlags(0)
  {
    MOZ_COUNT_CTOR(CacheIndexEntryUpdate);
    LOG(("CacheIndexEntryUpdate::CacheIndexEntryUpdate()"));
  }
  ~CacheIndexEntryUpdate()
  {
    MOZ_COUNT_DTOR(CacheIndexEntryUpdate);
    LOG(("CacheIndexEntryUpdate::~CacheIndexEntryUpdate()"));
  }

  CacheIndexEntryUpdate& operator=(const CacheIndexEntry& aOther)
  {
    MOZ_ASSERT(memcmp(&mRec->mHash, &aOther.mRec->mHash,
               sizeof(SHA1Sum::Hash)) == 0);
    mUpdateFlags = 0;
    *(static_cast<CacheIndexEntry *>(this)) = aOther;
    return *this;
  }

  void InitNew()
  {
    mUpdateFlags = kFrecencyUpdatedMask | kExpirationUpdatedMask |
                   kFileSizeUpdatedMask;
    CacheIndexEntry::InitNew();
  }

  void SetFrecency(uint32_t aFrecency)
  {
    mUpdateFlags |= kFrecencyUpdatedMask;
    CacheIndexEntry::SetFrecency(aFrecency);
  }

  void SetExpirationTime(uint32_t aExpirationTime)
  {
    mUpdateFlags |= kExpirationUpdatedMask;
    CacheIndexEntry::SetExpirationTime(aExpirationTime);
  }

  void SetFileSize(uint32_t aFileSize)
  {
    mUpdateFlags |= kFileSizeUpdatedMask;
    CacheIndexEntry::SetFileSize(aFileSize);
  }

  void ApplyUpdate(CacheIndexEntry *aDst) {
    MOZ_ASSERT(memcmp(&mRec->mHash, &aDst->mRec->mHash,
               sizeof(SHA1Sum::Hash)) == 0);
    if (mUpdateFlags & kFrecencyUpdatedMask) {
      aDst->mRec->mFrecency = mRec->mFrecency;
    }
    if (mUpdateFlags & kExpirationUpdatedMask) {
      aDst->mRec->mExpirationTime = mRec->mExpirationTime;
    }
    aDst->mRec->mAppId = mRec->mAppId;
    if (mUpdateFlags & kFileSizeUpdatedMask) {
      aDst->mRec->mFlags = mRec->mFlags;
    } else {
      
      aDst->mRec->mFlags &= kFileSizeMask;
      aDst->mRec->mFlags |= (mRec->mFlags & ~kFileSizeMask);
    }
  }

private:
  static const uint32_t kFrecencyUpdatedMask = 0x00000001;
  static const uint32_t kExpirationUpdatedMask = 0x00000002;
  static const uint32_t kFileSizeUpdatedMask = 0x00000004;

  uint32_t mUpdateFlags;
};

class CacheIndexStats
{
public:
  CacheIndexStats()
    : mCount(0)
    , mNotInitialized(0)
    , mRemoved(0)
    , mDirty(0)
    , mFresh(0)
    , mEmpty(0)
    , mSize(0)
#ifdef DEBUG
    , mStateLogged(false)
    , mDisableLogging(false)
#endif
  {
  }

  bool operator==(const CacheIndexStats& aOther) const
  {
    return
#ifdef DEBUG
           aOther.mStateLogged == mStateLogged &&
#endif
           aOther.mCount == mCount &&
           aOther.mNotInitialized == mNotInitialized &&
           aOther.mRemoved == mRemoved &&
           aOther.mDirty == mDirty &&
           aOther.mFresh == mFresh &&
           aOther.mEmpty == mEmpty &&
           aOther.mSize == mSize;
  }

#ifdef DEBUG
  void DisableLogging() {
    mDisableLogging = true;
  }
#endif

  void Log() {
    LOG(("CacheIndexStats::Log() [count=%u, notInitialized=%u, removed=%u, "
         "dirty=%u, fresh=%u, empty=%u, size=%u]", mCount, mNotInitialized,
         mRemoved, mDirty, mFresh, mEmpty, mSize));
  }

  void Clear() {
    MOZ_ASSERT(!mStateLogged, "CacheIndexStats::Clear() - state logged!");

    mCount = 0;
    mNotInitialized = 0;
    mRemoved = 0;
    mDirty = 0;
    mFresh = 0;
    mEmpty = 0;
    mSize = 0;
  }

#ifdef DEBUG
  bool StateLogged() {
    return mStateLogged;
  }
#endif

  uint32_t Count() {
    MOZ_ASSERT(!mStateLogged, "CacheIndexStats::Count() - state logged!");
    return mCount;
  }

  uint32_t Dirty() {
    MOZ_ASSERT(!mStateLogged, "CacheIndexStats::Dirty() - state logged!");
    return mDirty;
  }

  uint32_t Fresh() {
    MOZ_ASSERT(!mStateLogged, "CacheIndexStats::Fresh() - state logged!");
    return mFresh;
  }

  uint32_t ActiveEntriesCount() {
    MOZ_ASSERT(!mStateLogged, "CacheIndexStats::ActiveEntriesCount() - state "
               "logged!");
    return mCount - mRemoved - mNotInitialized - mEmpty;
  }

  uint32_t Size() {
    MOZ_ASSERT(!mStateLogged, "CacheIndexStats::Size() - state logged!");
    return mSize;
  }

  void BeforeChange(const CacheIndexEntry *aEntry) {
#ifdef DEBUG_STATS
    if (!mDisableLogging) {
      LOG(("CacheIndexStats::BeforeChange()"));
      Log();
    }
#endif

    MOZ_ASSERT(!mStateLogged, "CacheIndexStats::BeforeChange() - state "
               "logged!");
#ifdef DEBUG
    mStateLogged = true;
#endif
    if (aEntry) {
      MOZ_ASSERT(mCount);
      mCount--;
      if (aEntry->IsDirty()) {
        MOZ_ASSERT(mDirty);
        mDirty--;
      }
      if (aEntry->IsFresh()) {
        MOZ_ASSERT(mFresh);
        mFresh--;
      }
      if (aEntry->IsRemoved()) {
        MOZ_ASSERT(mRemoved);
        mRemoved--;
      } else {
        if (!aEntry->IsInitialized()) {
          MOZ_ASSERT(mNotInitialized);
          mNotInitialized--;
        } else {
          if (aEntry->IsFileEmpty()) {
            MOZ_ASSERT(mEmpty);
            mEmpty--;
          } else {
            MOZ_ASSERT(mSize >= aEntry->GetFileSize());
            mSize -= aEntry->GetFileSize();
          }
        }
      }
    }
  }

  void AfterChange(const CacheIndexEntry *aEntry) {
    MOZ_ASSERT(mStateLogged, "CacheIndexStats::AfterChange() - state not "
               "logged!");
#ifdef DEBUG
    mStateLogged = false;
#endif
    if (aEntry) {
      ++mCount;
      if (aEntry->IsDirty()) {
        mDirty++;
      }
      if (aEntry->IsFresh()) {
        mFresh++;
      }
      if (aEntry->IsRemoved()) {
        mRemoved++;
      } else {
        if (!aEntry->IsInitialized()) {
          mNotInitialized++;
        } else {
          if (aEntry->IsFileEmpty()) {
            mEmpty++;
          } else {
            mSize += aEntry->GetFileSize();
          }
        }
      }
    }

#ifdef DEBUG_STATS
    if (!mDisableLogging) {
      LOG(("CacheIndexStats::AfterChange()"));
      Log();
    }
#endif
  }

private:
  uint32_t mCount;
  uint32_t mNotInitialized;
  uint32_t mRemoved;
  uint32_t mDirty;
  uint32_t mFresh;
  uint32_t mEmpty;
  uint32_t mSize;
#ifdef DEBUG
  
  
  
  
  
  bool     mStateLogged;

  
  bool     mDisableLogging;
#endif
};

class CacheIndex : public CacheFileIOListener
                 , public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  CacheIndex();

  static nsresult Init(nsIFile *aCacheDirectory);
  static nsresult PreShutdown();
  static nsresult Shutdown();

  

  
  
  
  
  static nsresult AddEntry(const SHA1Sum::Hash *aHash);

  
  
  
  
  
  static nsresult EnsureEntryExists(const SHA1Sum::Hash *aHash);

  
  
  static nsresult InitEntry(const SHA1Sum::Hash *aHash,
                            uint32_t             aAppId,
                            bool                 aAnonymous,
                            bool                 aInBrowser);

  
  static nsresult RemoveEntry(const SHA1Sum::Hash *aHash);

  
  
  
  
  static nsresult UpdateEntry(const SHA1Sum::Hash *aHash,
                              const uint32_t      *aFrecency,
                              const uint32_t      *aExpirationTime,
                              const uint32_t      *aSize);

  
  static nsresult RemoveAll();

  enum EntryStatus {
    EXISTS         = 0,
    DOES_NOT_EXIST = 1,
    DO_NOT_KNOW    = 2
  };

  
  
  static nsresult HasEntry(const nsACString &aKey, EntryStatus *_retval);

  
  
  
  
  static nsresult GetEntryForEviction(bool aIgnoreEmptyEntries, SHA1Sum::Hash *aHash, uint32_t *aCnt);

  
  
  
  static bool IsForcedValidEntry(const SHA1Sum::Hash *aHash);

  
  static nsresult GetCacheSize(uint32_t *_retval);

  
  static nsresult GetEntryFileCount(uint32_t *_retval);

  
  
  static nsresult GetCacheStats(nsILoadContextInfo *aInfo, uint32_t *aSize, uint32_t *aCount);

  
  static nsresult AsyncGetDiskConsumption(nsICacheStorageConsumptionObserver* aObserver);

  
  
  
  
  
  static nsresult GetIterator(nsILoadContextInfo *aInfo, bool aAddNew,
                              CacheIndexIterator **_retval);

  
  
  static nsresult IsUpToDate(bool *_retval);

  
  static size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);
  static size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

private:
  friend class CacheIndexEntryAutoManage;
  friend class CacheIndexAutoLock;
  friend class CacheIndexAutoUnlock;
  friend class FileOpenHelper;
  friend class CacheIndexIterator;

  virtual ~CacheIndex();

  NS_IMETHOD OnFileOpened(CacheFileHandle *aHandle, nsresult aResult) MOZ_OVERRIDE;
  nsresult   OnFileOpenedInternal(FileOpenHelper *aOpener,
                                  CacheFileHandle *aHandle, nsresult aResult);
  NS_IMETHOD OnDataWritten(CacheFileHandle *aHandle, const char *aBuf,
                           nsresult aResult) MOZ_OVERRIDE;
  NS_IMETHOD OnDataRead(CacheFileHandle *aHandle, char *aBuf, nsresult aResult) MOZ_OVERRIDE;
  NS_IMETHOD OnFileDoomed(CacheFileHandle *aHandle, nsresult aResult) MOZ_OVERRIDE;
  NS_IMETHOD OnEOFSet(CacheFileHandle *aHandle, nsresult aResult) MOZ_OVERRIDE;
  NS_IMETHOD OnFileRenamed(CacheFileHandle *aHandle, nsresult aResult) MOZ_OVERRIDE;

  void     Lock();
  void     Unlock();
  void     AssertOwnsLock();

  nsresult InitInternal(nsIFile *aCacheDirectory);
  void     PreShutdownInternal();

  
  bool IsIndexUsable();

  
  
  
  
  static bool IsCollision(CacheIndexEntry *aEntry,
                          uint32_t         aAppId,
                          bool             aAnonymous,
                          bool             aInBrowser);

  
  static bool HasEntryChanged(CacheIndexEntry *aEntry,
                              const uint32_t  *aFrecency,
                              const uint32_t  *aExpirationTime,
                              const uint32_t  *aSize);

  
  void ProcessPendingOperations();
  static PLDHashOperator UpdateEntryInIndex(CacheIndexEntryUpdate *aEntry,
                                            void* aClosure);

  
  
  
  
  
  
  
  
  
  
  bool WriteIndexToDiskIfNeeded();
  
  void WriteIndexToDisk();
  
  
  void WriteRecords();
  
  void FinishWrite(bool aSucceeded);

  static PLDHashOperator CopyRecordsToRWBuf(CacheIndexEntry *aEntry,
                                            void* aClosure);
  static PLDHashOperator ApplyIndexChanges(CacheIndexEntry *aEntry,
                                           void* aClosure);

  
  
  
  
  
  
  
  nsresult GetFile(const nsACString &aName, nsIFile **_retval);
  nsresult RemoveFile(const nsACString &aName);
  void     RemoveIndexFromDisk();
  
  nsresult WriteLogToDisk();

  static PLDHashOperator WriteEntryToLog(CacheIndexEntry *aEntry,
                                         void* aClosure);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void ReadIndexFromDisk();
  
  void StartReadingIndex();
  
  void ParseRecords();
  
  void StartReadingJournal();
  
  void ParseJournal();
  
  void MergeJournal();
  
  
  void EnsureNoFreshEntry();
  
  
  void EnsureCorrectStats();
  static PLDHashOperator SumIndexStats(CacheIndexEntry *aEntry, void* aClosure);
  
  void FinishRead(bool aSucceeded);

  static PLDHashOperator ProcessJournalEntry(CacheIndexEntry *aEntry,
                                             void* aClosure);

  
  
  static void DelayedUpdate(nsITimer *aTimer, void *aClosure);
  
  nsresult ScheduleUpdateTimer(uint32_t aDelay);
  nsresult SetupDirectoryEnumerator();
  void InitEntryFromDiskData(CacheIndexEntry *aEntry,
                             CacheFileMetadata *aMetaData,
                             int64_t aFileSize);
  
  bool IsUpdatePending();
  
  
  void BuildIndex();

  bool StartUpdatingIndexIfNeeded(bool aSwitchingToReadyState = false);
  
  
  void StartUpdatingIndex(bool aRebuild);
  
  
  
  void UpdateIndex();
  
  void FinishUpdate(bool aSucceeded);

  static PLDHashOperator RemoveNonFreshEntries(CacheIndexEntry *aEntry,
                                               void* aClosure);

  enum EState {
    
    
    
    INITIAL  = 0,

    
    
    
    
    
    
    
    
    
    READING  = 1,

    
    
    
    
    
    
    
    WRITING  = 2,

    
    
    
    
    
    BUILDING = 3,

    
    
    
    
    
    UPDATING = 4,

    
    
    
    
    READY    = 5,

    
    SHUTDOWN = 6
  };

#ifdef PR_LOGGING
  static char const * StateString(EState aState);
#endif
  void ChangeState(EState aNewState);

  
  void AllocBuffer();
  void ReleaseBuffer();

  
  void InsertRecordToFrecencyArray(CacheIndexRecord *aRecord);
  void RemoveRecordFromFrecencyArray(CacheIndexRecord *aRecord);

  
  void AddRecordToIterators(CacheIndexRecord *aRecord);
  void RemoveRecordFromIterators(CacheIndexRecord *aRecord);
  void ReplaceRecordInIterators(CacheIndexRecord *aOldRecord,
                                CacheIndexRecord *aNewRecord);

  
  size_t SizeOfExcludingThisInternal(mozilla::MallocSizeOf mallocSizeOf) const;

  static CacheIndex *gInstance;

  nsCOMPtr<nsIFile> mCacheDirectory;

  mozilla::Mutex mLock;
  EState         mState;
  
  
  TimeStamp      mStartTime;
  
  
  bool           mShuttingDown;
  
  
  
  
  
  
  bool           mIndexNeedsUpdate;
  
  
  
  
  
  bool           mRemovingAll;
  
  bool           mIndexOnDiskIsValid;
  
  
  
  bool           mDontMarkIndexClean;
  
  
  uint32_t       mIndexTimeStamp;
  
  
  
  TimeStamp      mLastDumpTime;

  
  nsCOMPtr<nsITimer> mUpdateTimer;
  
  bool               mUpdateEventPending;

  
  
  
  
  uint32_t                  mSkipEntries;
  
  
  
  uint32_t                  mProcessEntries;
  char                     *mRWBuf;
  uint32_t                  mRWBufSize;
  uint32_t                  mRWBufPos;
  nsRefPtr<CacheHash>       mRWHash;

  
  bool                      mJournalReadSuccessfully;

  
  nsRefPtr<CacheFileHandle> mIndexHandle;
  
  nsRefPtr<CacheFileHandle> mJournalHandle;
  
  nsRefPtr<CacheFileHandle> mTmpHandle;

  nsRefPtr<FileOpenHelper>  mIndexFileOpener;
  nsRefPtr<FileOpenHelper>  mJournalFileOpener;
  nsRefPtr<FileOpenHelper>  mTmpFileOpener;

  
  nsCOMPtr<nsIDirectoryEnumerator> mDirEnumerator;

  
  nsTHashtable<CacheIndexEntry> mIndex;

  
  
  nsTHashtable<CacheIndexEntryUpdate> mPendingUpdates;

  
  CacheIndexStats               mIndexStats;

  
  
  
  
  nsTHashtable<CacheIndexEntry> mTmpJournal;

  
  
  
  
  nsTArray<CacheIndexRecord *>  mFrecencyArray;

  nsTArray<CacheIndexIterator *> mIterators;

  class DiskConsumptionObserver : public nsRunnable
  {
  public:
    static DiskConsumptionObserver* Init(nsICacheStorageConsumptionObserver* aObserver)
    {
      nsWeakPtr observer = do_GetWeakReference(aObserver);
      if (!observer)
        return nullptr;

      return new DiskConsumptionObserver(observer);
    }

    void OnDiskConsumption(int64_t aSize)
    {
      mSize = aSize;
      NS_DispatchToMainThread(this);
    }

  private:
    explicit DiskConsumptionObserver(nsWeakPtr const &aWeakObserver)
      : mObserver(aWeakObserver) { }
    virtual ~DiskConsumptionObserver() {
      if (mObserver && !NS_IsMainThread()) {
        nsIWeakReference *obs;
        mObserver.forget(&obs);

        nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
        if (mainThread) {
          NS_ProxyRelease(mainThread, obs);
        } else {
          NS_WARNING("Cannot get main thread, leaking weak reference to "
                     "CacheStorageConsumptionObserver.");
        }
      }
    }

    NS_IMETHODIMP Run()
    {
      MOZ_ASSERT(NS_IsMainThread());

      nsCOMPtr<nsICacheStorageConsumptionObserver> observer =
        do_QueryReferent(mObserver);

      mObserver = nullptr;

      if (observer) {
        observer->OnNetworkCacheDiskConsumption(mSize);
      }

      return NS_OK;
    }

    nsWeakPtr mObserver;
    int64_t mSize;
  };

  
  nsTArray<nsRefPtr<DiskConsumptionObserver> > mDiskConsumptionObservers;
};

class CacheIndexAutoLock {
public:
  explicit CacheIndexAutoLock(CacheIndex *aIndex)
    : mIndex(aIndex)
    , mLocked(true)
  {
    mIndex->Lock();
  }
  ~CacheIndexAutoLock()
  {
    if (mLocked) {
      mIndex->Unlock();
    }
  }
  void Lock()
  {
    MOZ_ASSERT(!mLocked);
    mIndex->Lock();
    mLocked = true;
  }
  void Unlock()
  {
    MOZ_ASSERT(mLocked);
    mIndex->Unlock();
    mLocked = false;
  }

private:
  nsRefPtr<CacheIndex> mIndex;
  bool mLocked;
};

class CacheIndexAutoUnlock {
public:
  explicit CacheIndexAutoUnlock(CacheIndex *aIndex)
    : mIndex(aIndex)
    , mLocked(false)
  {
    mIndex->Unlock();
  }
  ~CacheIndexAutoUnlock()
  {
    if (!mLocked) {
      mIndex->Lock();
    }
  }
  void Lock()
  {
    MOZ_ASSERT(!mLocked);
    mIndex->Lock();
    mLocked = true;
  }
  void Unlock()
  {
    MOZ_ASSERT(mLocked);
    mIndex->Unlock();
    mLocked = false;
  }

private:
  nsRefPtr<CacheIndex> mIndex;
  bool mLocked;
};

} 
} 

#endif
