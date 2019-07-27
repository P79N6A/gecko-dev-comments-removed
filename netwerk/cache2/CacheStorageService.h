



#ifndef CacheStorageService__h__
#define CacheStorageService__h__

#include "nsICacheStorageService.h"
#include "nsIMemoryReporter.h"

#include "nsITimer.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "mozilla/Mutex.h"
#include "mozilla/Atomics.h"
#include "mozilla/TimeStamp.h"
#include "nsTArray.h"

class nsIURI;
class nsICacheEntryDoomCallback;
class nsICacheStorageVisitor;
class nsIRunnable;
class nsIThread;
class nsIEventTarget;

namespace mozilla {
namespace net {

class CacheStorageService;
class CacheStorage;
class CacheEntry;
class CacheEntryHandle;

class CacheMemoryConsumer
{
private:
  friend class CacheStorageService;
  uint32_t mReportedMemoryConsumption : 30;
  uint32_t mFlags : 2;

private:
  CacheMemoryConsumer() = delete;

protected:
  enum {
    
    NORMAL = 0,
    
    
    MEMORY_ONLY = 1 << 0,
    
    
    
    DONT_REPORT = 1 << 1
  };

  explicit CacheMemoryConsumer(uint32_t aFlags);
  ~CacheMemoryConsumer() { DoMemoryReport(0); }
  void DoMemoryReport(uint32_t aCurrentSize);
};

class CacheStorageService final : public nsICacheStorageService
                                , public nsIMemoryReporter
                                , public nsITimerCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICACHESTORAGESERVICE
  NS_DECL_NSIMEMORYREPORTER
  NS_DECL_NSITIMERCALLBACK

  CacheStorageService();

  void Shutdown();
  void DropPrivateBrowsingEntries();

  
  
  static void CleaupCacheDirectories(uint32_t aVersion, uint32_t aActive);

  static CacheStorageService* Self() { return sSelf; }
  static nsISupports* SelfISupports() { return static_cast<nsICacheStorageService*>(Self()); }
  nsresult Dispatch(nsIRunnable* aEvent);
  static bool IsRunning() { return sSelf && !sSelf->mShutdown; }
  static bool IsOnManagementThread();
  already_AddRefed<nsIEventTarget> Thread() const;
  mozilla::Mutex& Lock() { return mLock; }

  
  nsDataHashtable<nsCStringHashKey, TimeStamp> mForcedValidEntries;
  void ForcedValidEntriesPrune(TimeStamp &now);

  
  
  
  class EntryInfoCallback {
  public:
    virtual void OnEntryInfo(const nsACString & aURISpec, const nsACString & aIdEnhance,
                             int64_t aDataSize, int32_t aFetchCount,
                             uint32_t aLastModifiedTime, uint32_t aExpirationTime) = 0;
  };

  
  static void GetCacheEntryInfo(CacheEntry* aEntry, EntryInfoCallback *aVisitor);

  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf)

private:
  virtual ~CacheStorageService();
  void ShutdownBackground();

private:
  
  
  friend class CacheEntry;

  






  void RegisterEntry(CacheEntry* aEntry);

  



  void UnregisterEntry(CacheEntry* aEntry);

  


  bool RemoveEntry(CacheEntry* aEntry, bool aOnlyUnreferenced = false);

  



  void RecordMemoryOnlyEntry(CacheEntry* aEntry,
                             bool aOnlyInMemory,
                             bool aOverwrite);

  




  void ForceEntryValidFor(nsACString &aCacheEntryKey,
                          uint32_t aSecondsToTheFuture);

private:
  friend class CacheIndex;

  






  bool IsForcedValidEntry(nsACString &aCacheEntryKey);

private:
  
  void TelemetryPrune(TimeStamp &now);
  void TelemetryRecordEntryCreation(CacheEntry const* entry);
  void TelemetryRecordEntryRemoval(CacheEntry const* entry);

private:
  
  friend class CacheStorage;

  



  nsresult AddStorageEntry(CacheStorage const* aStorage,
                           nsIURI* aURI,
                           const nsACString & aIdExtension,
                           bool aCreateIfNotExist,
                           bool aReplace,
                           CacheEntryHandle** aResult);

  



  nsresult CheckStorageEntry(CacheStorage const* aStorage,
                             nsIURI* aURI,
                             const nsACString & aIdExtension,
                             bool* aResult);

  



  nsresult DoomStorageEntry(CacheStorage const* aStorage,
                            nsIURI* aURI,
                            const nsACString & aIdExtension,
                            nsICacheEntryDoomCallback* aCallback);

  


  nsresult DoomStorageEntries(CacheStorage const* aStorage,
                              nsICacheEntryDoomCallback* aCallback);

  


  nsresult WalkStorageEntries(CacheStorage const* aStorage,
                              bool aVisitEntries,
                              nsICacheStorageVisitor* aVisitor);

private:
  friend class CacheFileIOManager;

  




  void CacheFileDoomed(nsILoadContextInfo* aLoadContextInfo,
                       const nsACString & aIdExtension,
                       const nsACString & aURISpec);

  







  bool GetCacheEntryInfo(nsILoadContextInfo* aLoadContextInfo,
                         const nsACString & aIdExtension,
                         const nsACString & aURISpec,
                         EntryInfoCallback *aCallback);

private:
  friend class CacheMemoryConsumer;

  




  void OnMemoryConsumptionChange(CacheMemoryConsumer* aConsumer,
                                 uint32_t aCurrentMemoryConsumption);

  



  void SchedulePurgeOverMemoryLimit();

  




  void PurgeOverMemoryLimit();

private:
  nsresult DoomStorageEntries(nsCSubstring const& aContextKey,
                              nsILoadContextInfo* aContext,
                              bool aDiskStorage,
                              nsICacheEntryDoomCallback* aCallback);
  nsresult AddStorageEntry(nsCSubstring const& aContextKey,
                           nsIURI* aURI,
                           const nsACString & aIdExtension,
                           bool aWriteToDisk,
                           bool aCreateIfNotExist,
                           bool aReplace,
                           CacheEntryHandle** aResult);

  static CacheStorageService* sSelf;

  mozilla::Mutex mLock;
  mozilla::Mutex mForcedValidEntriesLock;

  bool mShutdown;

  
  class MemoryPool
  {
  public:
    enum EType
    {
      DISK,
      MEMORY,
    } mType;

    explicit MemoryPool(EType aType);
    ~MemoryPool();

    nsTArray<nsRefPtr<CacheEntry> > mFrecencyArray;
    nsTArray<nsRefPtr<CacheEntry> > mExpirationArray;
    mozilla::Atomic<uint32_t> mMemorySize;

    bool OnMemoryConsumptionChange(uint32_t aSavedMemorySize,
                                   uint32_t aCurrentMemoryConsumption);
    


    void PurgeOverMemoryLimit();
    void PurgeExpired();
    void PurgeByFrecency(bool &aFrecencyNeedsSort, uint32_t aWhat);
    void PurgeAll(uint32_t aWhat);

  private:
    uint32_t const Limit() const;
    MemoryPool() = delete;
  };

  MemoryPool mDiskPool;
  MemoryPool mMemoryPool;
  MemoryPool& Pool(bool aUsingDisk)
  {
    return aUsingDisk ? mDiskPool : mMemoryPool;
  }
  MemoryPool const& Pool(bool aUsingDisk) const
  {
    return aUsingDisk ? mDiskPool : mMemoryPool;
  }

  nsCOMPtr<nsITimer> mPurgeTimer;

  class PurgeFromMemoryRunnable : public nsRunnable
  {
  public:
    PurgeFromMemoryRunnable(CacheStorageService* aService, uint32_t aWhat)
      : mService(aService), mWhat(aWhat) { }

  private:
    virtual ~PurgeFromMemoryRunnable() { }

    NS_IMETHOD Run()
    {
      
      mService->Pool(true).PurgeAll(mWhat);
      mService->Pool(false).PurgeAll(mWhat);
      return NS_OK;
    }

    nsRefPtr<CacheStorageService> mService;
    uint32_t mWhat;
  };

  
  
  
  
  nsDataHashtable<nsCStringHashKey, mozilla::TimeStamp> mPurgeTimeStamps;
};

template<class T>
void ProxyRelease(nsCOMPtr<T> &object, nsIThread* thread)
{
  T* release;
  object.forget(&release);

  NS_ProxyRelease(thread, release);
}

template<class T>
void ProxyReleaseMainThread(nsCOMPtr<T> &object)
{
  nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
  ProxyRelease(object, mainThread);
}

} 
} 

#define NS_CACHE_STORAGE_SERVICE_CID \
  { 0xea70b098, 0x5014, 0x4e21, \
  { 0xae, 0xe1, 0x75, 0xe6, 0xb2, 0xc4, 0xb8, 0xe0 } } \

#define NS_CACHE_STORAGE_SERVICE_CONTRACTID \
  "@mozilla.org/netwerk/cache-storage-service;1"

#define NS_CACHE_STORAGE_SERVICE_CONTRACTID2 \
  "@mozilla.org/network/cache-storage-service;1"

#endif
