



#ifndef CacheEntry__h__
#define CacheEntry__h__

#include "nsICacheEntry.h"
#include "CacheFile.h"

#include "nsIRunnable.h"
#include "nsIOutputStream.h"
#include "nsICacheEntryOpenCallback.h"
#include "nsICacheEntryDoomCallback.h"

#include "nsCOMPtr.h"
#include "nsRefPtrHashtable.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsString.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"

static inline uint32_t
PRTimeToSeconds(PRTime t_usec)
{
  PRTime usec_per_sec = PR_USEC_PER_SEC;
  return uint32_t(t_usec /= usec_per_sec);
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())

class nsIOutputStream;
class nsIURI;
class nsIThread;

namespace mozilla {
namespace net {

class CacheStorageService;
class CacheStorage;
class CacheOutputCloseListener;
class CacheEntryHandle;

class CacheEntry final : public nsICacheEntry
                       , public nsIRunnable
                       , public CacheFileListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICACHEENTRY
  NS_DECL_NSIRUNNABLE

  CacheEntry(const nsACString& aStorageID, nsIURI* aURI, const nsACString& aEnhanceID,
             bool aUseDisk);

  void AsyncOpen(nsICacheEntryOpenCallback* aCallback, uint32_t aFlags);

  CacheEntryHandle* NewHandle();

public:
  uint32_t GetMetadataMemoryConsumption();
  nsCString const &GetStorageID() const { return mStorageID; }
  nsCString const &GetEnhanceID() const { return mEnhanceID; }
  nsIURI* GetURI() const { return mURI; }
  
  bool IsUsingDisk() const { return mUseDisk; }
  bool IsReferenced() const;
  bool IsFileDoomed();
  bool IsDoomed() const { return mIsDoomed; }

  
  

  
  double GetFrecency() const;
  uint32_t GetExpirationTime() const;
  uint32_t UseCount() const { return mUseCount; }

  bool IsRegistered() const;
  bool CanRegister() const;
  void SetRegistered(bool aRegistered);

  TimeStamp const& LoadStart() const { return mLoadStart; }

  enum EPurge {
    PURGE_DATA_ONLY_DISK_BACKED,
    PURGE_WHOLE_ONLY_DISK_BACKED,
    PURGE_WHOLE,
  };

  bool Purge(uint32_t aWhat);
  void PurgeAndDoom();
  void DoomAlreadyRemoved();

  nsresult HashingKeyWithStorage(nsACString &aResult) const;
  nsresult HashingKey(nsACString &aResult) const;

  static nsresult HashingKey(nsCSubstring const& aStorageID,
                             nsCSubstring const& aEnhanceID,
                             nsIURI* aURI,
                             nsACString &aResult);

  static nsresult HashingKey(nsCSubstring const& aStorageID,
                             nsCSubstring const& aEnhanceID,
                             nsCSubstring const& aURISpec,
                             nsACString &aResult);

  
  double mFrecency;
  ::mozilla::Atomic<uint32_t, ::mozilla::Relaxed> mSortingExpirationTime;

  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

private:
  virtual ~CacheEntry();

  
  NS_IMETHOD OnFileReady(nsresult aResult, bool aIsNew) override;
  NS_IMETHOD OnFileDoomed(nsresult aResult) override;

  
  nsRefPtr<CacheStorageService> mService;

  
  
  
  class Callback
  {
  public:
    Callback(CacheEntry* aEntry,
             nsICacheEntryOpenCallback *aCallback,
             bool aReadOnly, bool aCheckOnAnyThread, bool aSecret);
    Callback(Callback const &aThat);
    ~Callback();

    
    
    void ExchangeEntry(CacheEntry* aEntry);

    
    
    
    nsRefPtr<CacheEntry> mEntry;
    nsCOMPtr<nsICacheEntryOpenCallback> mCallback;
    nsCOMPtr<nsIThread> mTargetThread;
    bool mReadOnly : 1;
    bool mRevalidating : 1;
    bool mCheckOnAnyThread : 1;
    bool mRecheckAfterWrite : 1;
    bool mNotWanted : 1;
    bool mSecret : 1;

    nsresult OnCheckThread(bool *aOnCheckThread) const;
    nsresult OnAvailThread(bool *aOnAvailThread) const;
  };

  
  
  class AvailableCallbackRunnable : public nsRunnable
  {
  public:
    AvailableCallbackRunnable(CacheEntry* aEntry,
                              Callback const &aCallback)
      : mEntry(aEntry)
      , mCallback(aCallback)
    {}

  private:
    NS_IMETHOD Run()
    {
      mEntry->InvokeAvailableCallback(mCallback);
      return NS_OK;
    }

    nsRefPtr<CacheEntry> mEntry;
    Callback mCallback;
  };

  
  
  class DoomCallbackRunnable : public nsRunnable
  {
  public:
    DoomCallbackRunnable(CacheEntry* aEntry, nsresult aRv)
      : mEntry(aEntry), mRv(aRv) {}

  private:
    NS_IMETHOD Run()
    {
      nsCOMPtr<nsICacheEntryDoomCallback> callback;
      {
        mozilla::MutexAutoLock lock(mEntry->mLock);
        mEntry->mDoomCallback.swap(callback);
      }

      if (callback)
        callback->OnCacheEntryDoomed(mRv);
      return NS_OK;
    }

    nsRefPtr<CacheEntry> mEntry;
    nsresult mRv;
  };

  
  
  bool Open(Callback & aCallback, bool aTruncate, bool aPriority, bool aBypassIfBusy);
  
  bool Load(bool aTruncate, bool aPriority);
  void OnLoaded();

  void RememberCallback(Callback & aCallback);
  void InvokeCallbacksLock();
  void InvokeCallbacks();
  bool InvokeCallbacks(bool aReadOnly);
  bool InvokeCallback(Callback & aCallback);
  void InvokeAvailableCallback(Callback const & aCallback);

  nsresult OpenOutputStreamInternal(int64_t offset, nsIOutputStream * *_retval);

  
  
  CacheEntryHandle* NewWriteHandle();
  void OnHandleClosed(CacheEntryHandle const* aHandle);

private:
  friend class CacheEntryHandle;
  
  void AddHandleRef() { ++mHandlesCount; }
  void ReleaseHandleRef() { --mHandlesCount; }
  
  uint32_t HandlesCount() const { return mHandlesCount; }

private:
  friend class CacheOutputCloseListener;
  void OnOutputClosed();

private:
  
  
  
  void BackgroundOp(uint32_t aOperation, bool aForceAsync = false);
  void StoreFrecency(double aFrecency);

  
  void DoomFile();

  already_AddRefed<CacheEntryHandle> ReopenTruncated(bool aMemoryOnly,
                                                     nsICacheEntryOpenCallback* aCallback);
  void TransferCallbacks(CacheEntry & aFromEntry);

  mozilla::Mutex mLock;

  
  ::mozilla::ThreadSafeAutoRefCnt mHandlesCount;

  nsTArray<Callback> mCallbacks;
  nsCOMPtr<nsICacheEntryDoomCallback> mDoomCallback;

  nsRefPtr<CacheFile> mFile;

  
  
  
  ::mozilla::Atomic<nsresult, ::mozilla::ReleaseAcquire> mFileStatus;
  nsCOMPtr<nsIURI> mURI;
  nsCString mEnhanceID;
  nsCString mStorageID;

  
  bool const mUseDisk;

  
  
  bool mIsDoomed;

  

  
  bool mSecurityInfoLoaded : 1;
  
  bool mPreventCallbacks : 1;
  
  
  
  
  
  
  bool mHasData : 1;

#ifdef PR_LOG
  static char const * StateString(uint32_t aState);
#endif

  enum EState {      
    NOTLOADED = 0,   
    LOADING = 1,     
    EMPTY = 2,       
    WRITING = 3,     
    READY = 4,       
    REVALIDATING = 5 
  };

  
  EState mState;

  enum ERegistration {
    NEVERREGISTERED = 0, 
    REGISTERED = 1,      
    DEREGISTERED = 2     
  };

  
  
  ERegistration mRegistration;

  
  
  
  
  nsCOMPtr<nsIOutputStream> mOutputStream;

  
  
  
  CacheEntryHandle* mWriter;

  
  
  class Ops {
  public:
    static uint32_t const REGISTER =          1 << 0;
    static uint32_t const FRECENCYUPDATE =    1 << 1;
    static uint32_t const CALLBACKS =         1 << 2;
    static uint32_t const UNREGISTER =        1 << 3;

    Ops() : mFlags(0) { }
    uint32_t Grab() { uint32_t flags = mFlags; mFlags = 0; return flags; }
    bool Set(uint32_t aFlags) { if (mFlags & aFlags) return false; mFlags |= aFlags; return true; }
  private:
    uint32_t mFlags;
  } mBackgroundOperations;

  nsCOMPtr<nsISupports> mSecurityInfo;
  int64_t mPredictedDataSize;
  mozilla::TimeStamp mLoadStart;
  uint32_t mUseCount;
  nsCOMPtr<nsIThread> mReleaseThread;
};


class CacheEntryHandle : public nsICacheEntry
{
public:
  explicit CacheEntryHandle(CacheEntry* aEntry);
  CacheEntry* Entry() const { return mEntry; }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_FORWARD_NSICACHEENTRY(mEntry->)
private:
  virtual ~CacheEntryHandle();
  nsRefPtr<CacheEntry> mEntry;
};


class CacheOutputCloseListener final : public nsRunnable
{
public:
  void OnOutputClosed();

private:
  friend class CacheEntry;

  virtual ~CacheOutputCloseListener();

  NS_DECL_NSIRUNNABLE
  explicit CacheOutputCloseListener(CacheEntry* aEntry);

private:
  nsRefPtr<CacheEntry> mEntry;
};

} 
} 

#endif
