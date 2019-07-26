



#ifndef CacheStorageService__h__
#define CacheStorageService__h__

#include "nsICacheStorageService.h"

#include "nsClassHashtable.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "mozilla/Mutex.h"
#include "mozilla/Atomics.h"
#include "nsTArray.h"

class nsIURI;
class nsICacheEntryOpenCallback;
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
class CacheEntryTable;

class CacheMemoryConsumer
{
private:
  friend class CacheStorageService;
  uint32_t mReportedMemoryConsumption;
protected:
  CacheMemoryConsumer();
  void DoMemoryReport(uint32_t aCurrentSize);
};

class CacheStorageService : public nsICacheStorageService
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICACHESTORAGESERVICE

  CacheStorageService();

  void Shutdown();
  void DropPrivateBrowsingEntries();

  static CacheStorageService* Self() { return sSelf; }
  nsresult Dispatch(nsIRunnable* aEvent);
  static bool IsOnManagementThread() { return sSelf && NS_GetCurrentThread() == sSelf->mThread; }
  static bool IsRunning() { return sSelf && !sSelf->mShutdown; }
  nsIEventTarget* Thread() const { return mThread; }
  mozilla::Mutex& Lock() { return mLock; }

private:
  virtual ~CacheStorageService();
  void ShutdownBackground();

private:
  
  
  friend class CacheEntry;

  






  void RegisterEntry(CacheEntry* aEntry);

  



  void UnregisterEntry(CacheEntry* aEntry);

  


  void RemoveEntry(CacheEntry* aEntry);

  



  void RecordMemoryOnlyEntry(CacheEntry* aEntry,
                             bool aOnlyInMemory,
                             bool aOverwrite);

private:
  
  friend class CacheStorage;

  



  nsresult AddStorageEntry(CacheStorage const* aStorage,
                           nsIURI* aURI,
                           const nsACString & aIdExtension,
                           bool aCreateIfNotExist,
                           bool aReplace,
                           CacheEntry** aResult);

  



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
  friend class CacheMemoryConsumer;

  




  void OnMemoryConsumptionChange(CacheMemoryConsumer* aConsumer,
                                 uint32_t aCurrentMemoryConsumption);
  void PurgeOverMemoryLimit();

private:
  class PurgeFromMemoryRunnable : public nsRunnable
  {
  public:
    PurgeFromMemoryRunnable(CacheStorageService* aService, uint32_t aWhat)
      : mService(aService), mWhat(aWhat) { }

  private:
    virtual ~PurgeFromMemoryRunnable() { }

    NS_IMETHOD Run() {
      mService->PurgeAll(mWhat);
      return NS_OK;
    }

    nsRefPtr<CacheStorageService> mService;
    uint32_t mWhat;
  };

  


  void PurgeByFrecency(bool &aFrecencyNeedsSort, uint32_t aWhat);
  void PurgeExpired();
  void PurgeAll(uint32_t aWhat);

  nsresult DoomStorageEntries(nsCSubstring const& aContextKey,
                              bool aDiskStorage,
                              nsICacheEntryDoomCallback* aCallback);
  nsresult AddStorageEntry(nsCSubstring const& aContextKey,
                           nsIURI* aURI,
                           const nsACString & aIdExtension,
                           bool aWriteToDisk,
                           bool aCreateIfNotExist,
                           bool aReplace,
                           CacheEntry** aResult);

  static CacheStorageService* sSelf;

  mozilla::Mutex mLock;

  bool mShutdown;

  
  nsCOMPtr<nsIThread> mThread;

  
  nsTArray<nsRefPtr<CacheEntry> > mFrecencyArray;
  nsTArray<nsRefPtr<CacheEntry> > mExpirationArray;
  mozilla::Atomic<uint32_t> mMemorySize;
  bool mPurging;
};

template<class T>
void ProxyReleaseMainThread(nsCOMPtr<T> &object)
{
  T* release;
  object.forget(&release);

  nsCOMPtr<nsIThread> mainThread;
  NS_GetMainThread(getter_AddRefs(mainThread));
  NS_ProxyRelease(mainThread, release);
}

} 
} 

#define NS_CACHE_STORAGE_SERVICE_CID \
  { 0xea70b098, 0x5014, 0x4e21, \
  { 0xae, 0xe1, 0x75, 0xe6, 0xb2, 0xc4, 0xb8, 0xe0 } } \

#define NS_CACHE_STORAGE_SERVICE_CONTRACTID \
  "@mozilla.org/netwerk/cache-storage-service;1"

#endif
