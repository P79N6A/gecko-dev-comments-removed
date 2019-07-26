



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
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"

static inline uint32_t
PRTimeToSeconds(PRTime t_usec)
{
  PRTime usec_per_sec = PR_USEC_PER_SEC;
  return uint32_t(t_usec /= usec_per_sec);
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())

class nsIStorageStream;
class nsIOutputStream;
class nsIURI;
class nsIThread;

namespace mozilla {
namespace net {

class CacheStorageService;
class CacheStorage;
class CacheFileOutputStream;
class CacheOutputCloseListener;
class CacheEntryHandle;

class CacheEntry : public nsICacheEntry
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
  bool UsingDisk() const;
  bool SetUsingDisk(bool aUsingDisk);
  bool IsReferenced() const;
  bool IsFileDoomed();

  
  

  
  double GetFrecency() const;
  uint32_t GetExpirationTime() const;

  bool IsRegistered() const;
  bool CanRegister() const;
  void SetRegistered(bool aRegistered);

  enum EPurge {
    PURGE_DATA_ONLY_DISK_BACKED,
    PURGE_WHOLE_ONLY_DISK_BACKED,
    PURGE_WHOLE,
  };

  bool Purge(uint32_t aWhat);
  void PurgeAndDoom();
  void DoomAlreadyRemoved();

  nsresult HashingKeyWithStorage(nsACString &aResult);
  nsresult HashingKey(nsACString &aResult);

  static nsresult HashingKey(nsCSubstring const& aStorageID,
                             nsCSubstring const& aEnhanceID,
                             nsIURI* aURI,
                             nsACString &aResult);

  static nsresult HashingKey(nsCSubstring const& aStorageID,
                             nsCSubstring const& aEnhanceID,
                             nsCSubstring const& aURISpec,
                             nsACString &aResult);

  
  double mFrecency;
  uint32_t mSortingExpirationTime;

  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

private:
  virtual ~CacheEntry();

  
  NS_IMETHOD OnFileReady(nsresult aResult, bool aIsNew);
  NS_IMETHOD OnFileDoomed(nsresult aResult);

  
  nsRefPtr<CacheStorageService> mService;

  
  
  
  class Callback
  {
  public:
    Callback(CacheEntry* aEntry,
             nsICacheEntryOpenCallback *aCallback,
             bool aReadOnly, bool aCheckOnAnyThread);
    Callback(Callback const &aThat);
    ~Callback();

    
    
    void ExchangeEntry(CacheEntry* aEntry);

    
    
    
    nsRefPtr<CacheEntry> mEntry;
    nsCOMPtr<nsICacheEntryOpenCallback> mCallback;
    nsCOMPtr<nsIThread> mTargetThread;
    bool mReadOnly : 1;
    bool mCheckOnAnyThread : 1;
    bool mRecheckAfterWrite : 1;
    bool mNotWanted : 1;

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

  
  bool Load(bool aTruncate, bool aPriority);
  void OnLoaded();

  void RememberCallback(Callback const & aCallback);
  void InvokeCallbacksLock();
  void InvokeCallbacks();
  bool InvokeCallbacks(bool aReadOnly);
  bool InvokeCallback(Callback & aCallback);
  void InvokeAvailableCallback(Callback const & aCallback);

  nsresult OpenOutputStreamInternal(int64_t offset, nsIOutputStream * *_retval);

  
  
  CacheEntryHandle* NewWriteHandle();
  void OnHandleClosed(CacheEntryHandle const* aHandle);

private:
  friend class CacheOutputCloseListener;
  void OnOutputClosed();

  
  
  
  void BackgroundOp(uint32_t aOperation, bool aForceAsync = false);
  void StoreFrecency();

  
  void DoomFile();

  already_AddRefed<CacheEntryHandle> ReopenTruncated(bool aMemoryOnly,
                                                     nsICacheEntryOpenCallback* aCallback);
  void TransferCallbacks(CacheEntry & aFromEntry);

  mozilla::Mutex mLock;

  
  friend class CacheEntryHandle;
  ::mozilla::ThreadSafeAutoRefCnt mHandlersCount;

  nsTArray<Callback> mCallbacks;
  nsCOMPtr<nsICacheEntryDoomCallback> mDoomCallback;

  nsRefPtr<CacheFile> mFile;
  nsresult mFileStatus;
  nsCOMPtr<nsIURI> mURI;
  nsCString mEnhanceID;
  nsCString mStorageID;

  
  
  
  bool mUseDisk;

  
  
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
  nsCOMPtr<nsIThread> mReleaseThread;
};


class CacheEntryHandle : public nsICacheEntry
{
public:
  CacheEntryHandle(CacheEntry* aEntry);
  virtual ~CacheEntryHandle();
  CacheEntry* Entry() const { return mEntry; }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_FORWARD_NSICACHEENTRY(mEntry->)
private:
  nsRefPtr<CacheEntry> mEntry;
};


class CacheOutputCloseListener : public nsRunnable
{
public:
  void OnOutputClosed();
  virtual ~CacheOutputCloseListener();

private:
  friend class CacheEntry;

  NS_DECL_NSIRUNNABLE
  CacheOutputCloseListener(CacheEntry* aEntry);

private:
  nsRefPtr<CacheEntry> mEntry;
};

} 
} 

#endif
