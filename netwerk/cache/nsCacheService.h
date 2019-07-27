





#ifndef _nsCacheService_h_
#define _nsCacheService_h_

#include "nsICacheService.h"
#include "nsCacheSession.h"
#include "nsCacheDevice.h"
#include "nsCacheEntry.h"
#include "nsThreadUtils.h"
#include "nsICacheListener.h"
#include "nsIMemoryReporter.h"

#include "prthread.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsRefPtrHashtable.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "mozilla/Telemetry.h"

class nsCacheRequest;
class nsCacheProfilePrefObserver;
class nsDiskCacheDevice;
class nsMemoryCacheDevice;
class nsOfflineCacheDevice;
class nsCacheServiceAutoLock;
class nsITimer;
class mozIStorageService;






class nsNotifyDoomListener : public nsRunnable {
public:
    nsNotifyDoomListener(nsICacheListener *listener,
                         nsresult status)
        : mListener(listener)      
        , mStatus(status)
    {}

    NS_IMETHOD Run()
    {
        mListener->OnCacheEntryDoomed(mStatus);
        NS_RELEASE(mListener);
        return NS_OK;
    }

private:
    nsICacheListener *mListener;
    nsresult          mStatus;
};





class nsCacheService : public nsICacheServiceInternal,
                       public nsIMemoryReporter
{
    virtual ~nsCacheService();

public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSICACHESERVICE
    NS_DECL_NSICACHESERVICEINTERNAL
    NS_DECL_NSIMEMORYREPORTER

    nsCacheService();

    
    static nsresult
    Create(nsISupports* outer, const nsIID& iid, void* *result);


    


    static nsresult  OpenCacheEntry(nsCacheSession *           session,
                                    const nsACString &         key,
                                    nsCacheAccessMode          accessRequested,
                                    bool                       blockingMode,
                                    nsICacheListener *         listener,
                                    nsICacheEntryDescriptor ** result);

    static nsresult  EvictEntriesForSession(nsCacheSession *   session);

    static nsresult  IsStorageEnabledForPolicy(nsCacheStoragePolicy  storagePolicy,
                                               bool *              result);

    static nsresult  DoomEntry(nsCacheSession   *session,
                               const nsACString &key,
                               nsICacheListener *listener);

    



    static void      CloseDescriptor(nsCacheEntryDescriptor * descriptor);

    static nsresult  GetFileForEntry(nsCacheEntry *         entry,
                                     nsIFile **             result);

    static nsresult  OpenInputStreamForEntry(nsCacheEntry *     entry,
                                             nsCacheAccessMode  mode,
                                             uint32_t           offset,
                                             nsIInputStream **  result);

    static nsresult  OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                              nsCacheAccessMode  mode,
                                              uint32_t           offset,
                                              nsIOutputStream ** result);

    static nsresult  OnDataSizeChange(nsCacheEntry * entry, int32_t deltaSize);

    static nsresult  SetCacheElement(nsCacheEntry * entry, nsISupports * element);

    static nsresult  ValidateEntry(nsCacheEntry * entry);

    static int32_t   CacheCompressionLevel();

    static bool      GetClearingEntries();

    static void      GetCacheBaseDirectoty(nsIFile ** result);
    static void      GetDiskCacheDirectory(nsIFile ** result);
    static void      GetAppCacheDirectory(nsIFile ** result);

    



    static
    nsCacheService * GlobalInstance()   { return gService; }

    static nsresult  DoomEntry(nsCacheEntry * entry);

    static bool      IsStorageEnabledForPolicy_Locked(nsCacheStoragePolicy policy);

    


    static void      MarkStartingFresh();

    



    nsresult GetOfflineDevice(nsOfflineCacheDevice ** aDevice);

    




    nsresult GetCustomOfflineDevice(nsIFile *aProfileDir,
                                    int32_t aQuota,
                                    nsOfflineCacheDevice **aDevice);

    
    
    
    
    
    static void      ReleaseObject_Locked(nsISupports *    object,
                                          nsIEventTarget * target = nullptr);

    static nsresult DispatchToCacheIOThread(nsIRunnable* event);

    
    
    
    static nsresult SyncWithCacheIOThread();


    


    static void      OnProfileShutdown(bool cleanse);
    static void      OnProfileChanged();

    static void      SetDiskCacheEnabled(bool    enabled);
    
    static void      SetDiskCacheCapacity(int32_t  capacity);
    
    
    static void      SetDiskCacheMaxEntrySize(int32_t  maxSize);
    
    
    static void      SetMemoryCacheMaxEntrySize(int32_t  maxSize);

    static void      SetOfflineCacheEnabled(bool    enabled);
    
    static void      SetOfflineCacheCapacity(int32_t  capacity);

    static void      SetMemoryCache();

    static void      SetCacheCompressionLevel(int32_t level);

    
    static nsresult  SetDiskSmartSize();

    static void      MoveOrRemoveDiskCache(nsIFile *aOldCacheDir,
                                           nsIFile *aNewCacheDir,
                                           const char *aCacheSubdir);

    nsresult         Init();
    void             Shutdown();

    static bool      IsInitialized()
    {
      if (!gService) {
          return false;
      }
      return gService->mInitialized;
    }

    static void      AssertOwnsLock()
    { gService->mLock.AssertCurrentThreadOwns(); }

    static void      LeavePrivateBrowsing();
    bool             IsDoomListEmpty();

    typedef bool (*DoomCheckFn)(nsCacheEntry* entry);

    
    nsresult CreateSessionInternal(const char *          clientID,
                                   nsCacheStoragePolicy  storagePolicy,
                                   bool                  streamBased,
                                   nsICacheSession     **result);
    nsresult VisitEntriesInternal(nsICacheVisitor *visitor);
    nsresult EvictEntriesInternal(nsCacheStoragePolicy storagePolicy);

private:
    friend class nsCacheServiceAutoLock;
    friend class nsOfflineCacheDevice;
    friend class nsProcessRequestEvent;
    friend class nsSetSmartSizeEvent;
    friend class nsBlockOnCacheThreadEvent;
    friend class nsSetDiskSmartSizeCallback;
    friend class nsDoomEvent;
    friend class nsDisableOldMaxSmartSizePrefEvent;
    friend class nsDiskCacheMap;
    friend class nsAsyncDoomEvent;
    friend class nsCacheEntryDescriptor;

    



    static void      Lock(::mozilla::Telemetry::ID mainThreadLockerID);
    static void      Unlock();
    void             LockAcquired();
    void             LockReleased();

    nsresult         CreateDiskDevice();
    nsresult         CreateOfflineDevice();
    nsresult         CreateCustomOfflineDevice(nsIFile *aProfileDir,
                                               int32_t aQuota,
                                               nsOfflineCacheDevice **aDevice);
    nsresult         CreateMemoryDevice();

    nsresult         RemoveCustomOfflineDevice(nsOfflineCacheDevice *aDevice);

    nsresult         CreateRequest(nsCacheSession *   session,
                                   const nsACString & clientKey,
                                   nsCacheAccessMode  accessRequested,
                                   bool               blockingMode,
                                   nsICacheListener * listener,
                                   nsCacheRequest **  request);

    nsresult         DoomEntry_Internal(nsCacheEntry * entry,
                                        bool doProcessPendingRequests);

    nsresult         EvictEntriesForClient(const char *          clientID,
                                           nsCacheStoragePolicy  storagePolicy);

    
    
    
    nsresult         NotifyListener(nsCacheRequest *          request,
                                    nsICacheEntryDescriptor * descriptor,
                                    nsCacheAccessMode         accessGranted,
                                    nsresult                  error);

    nsresult         ActivateEntry(nsCacheRequest * request,
                                   nsCacheEntry ** entry,
                                   nsCacheEntry ** doomedEntry);

    nsCacheDevice *  EnsureEntryHasDevice(nsCacheEntry * entry);

    nsCacheEntry *   SearchCacheDevices(nsCString * key, nsCacheStoragePolicy policy, bool *collision);

    void             DeactivateEntry(nsCacheEntry * entry);

    nsresult         ProcessRequest(nsCacheRequest *           request,
                                    bool                       calledFromOpenCacheEntry,
                                    nsICacheEntryDescriptor ** result);

    nsresult         ProcessPendingRequests(nsCacheEntry * entry);

    void             ClearDoomList(void);
    void             DoomActiveEntries(DoomCheckFn check);
    void             CloseAllStreams();
    void             FireClearNetworkCacheStoredAnywhereNotification();

    static
    PLDHashOperator  GetActiveEntries(PLDHashTable *    table,
                                      PLDHashEntryHdr * hdr,
                                      uint32_t          number,
                                      void *            arg);
    static
    PLDHashOperator  RemoveActiveEntry(PLDHashTable *    table,
                                       PLDHashEntryHdr * hdr,
                                       uint32_t          number,
                                       void *            arg);

    static
    PLDHashOperator  ShutdownCustomCacheDeviceEnum(const nsAString& aProfileDir,
                                                   nsRefPtr<nsOfflineCacheDevice>& aDevice,
                                                   void* aUserArg);
#if defined(PR_LOGGING)
    void LogCacheStatistics();
#endif

    nsresult         SetDiskSmartSize_Locked();

    



    static nsCacheService *         gService;  

    nsCOMPtr<mozIStorageService>    mStorageService;

    nsCacheProfilePrefObserver *    mObserver;

    mozilla::Mutex                  mLock;
    mozilla::CondVar                mCondVar;

    mozilla::Mutex                  mTimeStampLock;
    mozilla::TimeStamp              mLockAcquiredTimeStamp;

    nsCOMPtr<nsIThread>             mCacheIOThread;

    nsTArray<nsISupports*>          mDoomedObjects;
    nsCOMPtr<nsITimer>              mSmartSizeTimer;

    bool                            mInitialized;
    bool                            mClearingEntries;

    bool                            mEnableMemoryDevice;
    bool                            mEnableDiskDevice;
    bool                            mEnableOfflineDevice;

    nsMemoryCacheDevice *           mMemoryDevice;
    nsDiskCacheDevice *             mDiskDevice;
    nsOfflineCacheDevice *          mOfflineDevice;

    nsRefPtrHashtable<nsStringHashKey, nsOfflineCacheDevice> mCustomOfflineDevices;

    nsCacheEntryHashTable           mActiveEntries;
    PRCList                         mDoomedEntries;

    

    uint32_t                        mTotalEntries;
    uint32_t                        mCacheHits;
    uint32_t                        mCacheMisses;
    uint32_t                        mMaxKeyLength;
    uint32_t                        mMaxDataSize;
    uint32_t                        mMaxMetaSize;

    
    uint32_t                        mDeactivateFailures;
    uint32_t                        mDeactivatedUnboundEntries;
};





#define LOCK_TELEM(x) \
  (::mozilla::Telemetry::CACHE_SERVICE_LOCK_WAIT_MAINTHREAD_##x)



class nsCacheServiceAutoLock {
public:
    explicit nsCacheServiceAutoLock(mozilla::Telemetry::ID mainThreadLockerID) {
        nsCacheService::Lock(mainThreadLockerID);
    }
    ~nsCacheServiceAutoLock() {
        nsCacheService::Unlock();
    }
};

#endif 
