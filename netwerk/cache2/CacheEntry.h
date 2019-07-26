



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

namespace mozilla {
namespace net {

class CacheStorageService;
class CacheStorage;

namespace {
class FrecencyComparator;
class ExpirationComparator;
class EvictionRunnable;
class WalkRunnable;
}

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

public:
  uint32_t GetMetadataMemoryConsumption();
  nsCString const &GetStorageID() const { return mStorageID; }
  nsCString const &GetEnhanceID() const { return mEnhanceID; }
  nsIURI* GetURI() const { return mURI; }
  bool UsingDisk() const;
  bool SetUsingDisk(bool aUsingDisk);

  
  

  
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

  
  double mFrecency;
  uint32_t mSortingExpirationTime;

private:
  virtual ~CacheEntry();

  
  NS_IMETHOD OnFileReady(nsresult aResult, bool aIsNew);
  NS_IMETHOD OnFileDoomed(nsresult aResult);

  
  nsRefPtr<CacheStorageService> mService;

  
  
  
  class Handle : public nsICacheEntry
  {
  public:
    Handle(CacheEntry* aEntry);
    virtual ~Handle();

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_FORWARD_NSICACHEENTRY(mEntry->)
  private:
    nsRefPtr<CacheEntry> mEntry;
  };

  
  
  class AvailableCallbackRunnable : public nsRunnable
  {
  public:
    AvailableCallbackRunnable(CacheEntry* aEntry,
                              nsICacheEntryOpenCallback* aCallback,
                              bool aReadOnly,
                              bool aNotWanted)
      : mEntry(aEntry), mCallback(aCallback)
      , mReadOnly(aReadOnly), mNotWanted(aNotWanted) {}

  private:
    NS_IMETHOD Run()
    {
      mEntry->InvokeAvailableCallback(mCallback, mReadOnly, mNotWanted);
      return NS_OK;
    }

    nsRefPtr<CacheEntry> mEntry;
    nsCOMPtr<nsICacheEntryOpenCallback> mCallback;
    bool mReadOnly : 1;
    bool mNotWanted : 1;
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

  void RememberCallback(nsICacheEntryOpenCallback* aCallback, bool aReadOnly);
  bool PendingCallbacks();
  void InvokeCallbacks();
  bool InvokeCallback(nsICacheEntryOpenCallback* aCallback, bool aReadOnly);
  void InvokeAvailableCallback(nsICacheEntryOpenCallback* aCallback, bool aReadOnly, bool aNotWanted);
  void InvokeCallbacksMainThread();

  nsresult OpenOutputStreamInternal(int64_t offset, nsIOutputStream * *_retval);

  
  
  Handle* NewWriteHandle();
  void OnWriterClosed(Handle const* aHandle);

  
  
  
  void BackgroundOp(uint32_t aOperation, bool aForceAsync = false);

  already_AddRefed<CacheEntry> ReopenTruncated(nsICacheEntryOpenCallback* aCallback);
  void TransferCallbacks(CacheEntry const& aFromEntry);

  mozilla::Mutex mLock;

  nsCOMArray<nsICacheEntryOpenCallback> mCallbacks, mReadOnlyCallbacks;
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
  
  bool mHasMainThreadOnlyCallback : 1;
  
  
  
  
  
  
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

  
  
  
  Handle* mWriter;

  
  
  class Ops {
  public:
    static uint32_t const REGISTER =          1 << 0;
    static uint32_t const FRECENCYUPDATE =    1 << 1;
    static uint32_t const DOOM =              1 << 2;
    static uint32_t const CALLBACKS =         1 << 3;

    Ops() : mFlags(0) { }
    uint32_t Grab() { uint32_t flags = mFlags; mFlags = 0; return flags; }
    bool Set(uint32_t aFlags) { if (mFlags & aFlags) return false; mFlags |= aFlags; return true; }
  private:
    uint32_t mFlags;
  } mBackgroundOperations;

  nsCOMPtr<nsISupports> mSecurityInfo;

  int64_t mPredictedDataSize;
  uint32_t mDataSize; 

  mozilla::TimeStamp mLoadStart;
};

} 
} 

#endif
