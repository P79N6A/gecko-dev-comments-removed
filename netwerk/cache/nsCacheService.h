











































#ifndef _nsCacheService_h_
#define _nsCacheService_h_

#include "nsICacheService.h"
#include "nsCacheSession.h"
#include "nsCacheDevice.h"
#include "nsCacheEntry.h"

#include "prthread.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "nsProxiedService.h"
#include "nsTArray.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"

class nsCacheRequest;
class nsCacheProfilePrefObserver;
class nsDiskCacheDevice;
class nsMemoryCacheDevice;
class nsOfflineCacheDevice;
class nsCacheServiceAutoLock;






class nsCacheService : public nsICacheService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHESERVICE
    
    nsCacheService();
    virtual ~nsCacheService();

    
    static nsresult
    Create(nsISupports* outer, const nsIID& iid, void* *result);


    


    static nsresult  OpenCacheEntry(nsCacheSession *           session,
                                    const nsACString &         key,
                                    nsCacheAccessMode          accessRequested,
                                    PRBool                     blockingMode,
                                    nsICacheListener *         listener,
                                    nsICacheEntryDescriptor ** result);

    static nsresult  EvictEntriesForSession(nsCacheSession *   session);

    static nsresult  IsStorageEnabledForPolicy(nsCacheStoragePolicy  storagePolicy,
                                               PRBool *              result);

    



    static void      CloseDescriptor(nsCacheEntryDescriptor * descriptor);

    static nsresult  GetFileForEntry(nsCacheEntry *         entry,
                                     nsIFile **             result);

    static nsresult  OpenInputStreamForEntry(nsCacheEntry *     entry,
                                             nsCacheAccessMode  mode,
                                             PRUint32           offset,
                                             nsIInputStream **  result);

    static nsresult  OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                              nsCacheAccessMode  mode,
                                              PRUint32           offset,
                                              nsIOutputStream ** result);

    static nsresult  OnDataSizeChange(nsCacheEntry * entry, PRInt32 deltaSize);

    static nsresult  SetCacheElement(nsCacheEntry * entry, nsISupports * element);

    static nsresult  ValidateEntry(nsCacheEntry * entry);


    



    static
    nsCacheService * GlobalInstance()   { return gService; }
    
    static nsresult  DoomEntry(nsCacheEntry * entry);

    static PRBool    IsStorageEnabledForPolicy_Locked(nsCacheStoragePolicy policy);

    
    
    
    
    
    static void      ReleaseObject_Locked(nsISupports *    object,
                                          nsIEventTarget * target = nsnull);

    static nsresult DispatchToCacheIOThread(nsIRunnable* event);

    
    
    
    static nsresult SyncWithCacheIOThread();


    


    static void      OnProfileShutdown(PRBool cleanse);
    static void      OnProfileChanged();

    static void      SetDiskCacheEnabled(PRBool  enabled);
    
    static void      SetDiskCacheCapacity(PRInt32  capacity);
    
    static void      SetDiskCacheMaxEntrySize(PRInt32  maxSize);
    
    static void      SetMemoryCacheMaxEntrySize(PRInt32  maxSize);

    static void      SetOfflineCacheEnabled(PRBool  enabled);
    
    static void      SetOfflineCacheCapacity(PRInt32  capacity);

    static void      SetMemoryCache();

    static void      OnEnterExitPrivateBrowsing();

    nsresult         Init();
    void             Shutdown();

    static void      AssertOwnsLock()
    { gService->mLock.AssertCurrentThreadOwns(); }

private:
    friend class nsCacheServiceAutoLock;
    friend class nsOfflineCacheDevice;
    friend class nsProcessRequestEvent;
    friend class nsSetSmartSizeEvent;
    friend class nsBlockOnCacheThreadEvent;

    



    static void      Lock();
    static void      Unlock();

    nsresult         CreateDiskDevice();
    nsresult         CreateOfflineDevice();
    nsresult         CreateMemoryDevice();

    nsresult         CreateRequest(nsCacheSession *   session,
                                   const nsACString & clientKey,
                                   nsCacheAccessMode  accessRequested,
                                   PRBool             blockingMode,
                                   nsICacheListener * listener,
                                   nsCacheRequest **  request);

    nsresult         DoomEntry_Internal(nsCacheEntry * entry,
                                        PRBool doProcessPendingRequests);

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

    nsCacheEntry *   SearchCacheDevices(nsCString * key, nsCacheStoragePolicy policy, PRBool *collision);

    void             DeactivateEntry(nsCacheEntry * entry);

    nsresult         ProcessRequest(nsCacheRequest *           request,
                                    PRBool                     calledFromOpenCacheEntry,
                                    nsICacheEntryDescriptor ** result);

    nsresult         ProcessPendingRequests(nsCacheEntry * entry);

    void             ClearPendingRequests(nsCacheEntry * entry);
    void             ClearDoomList(void);
    void             ClearActiveEntries(void);
    void             DoomActiveEntries(void);

    static
    PLDHashOperator  DeactivateAndClearEntry(PLDHashTable *    table,
                                             PLDHashEntryHdr * hdr,
                                             PRUint32          number,
                                             void *            arg);
    static
    PLDHashOperator  RemoveActiveEntry(PLDHashTable *    table,
                                       PLDHashEntryHdr * hdr,
                                       PRUint32          number,
                                       void *            arg);
#if defined(PR_LOGGING)
    void LogCacheStatistics();
#endif

    



    static nsCacheService *         gService;  
    
    nsCacheProfilePrefObserver *    mObserver;
    
    mozilla::Mutex                  mLock;
    mozilla::CondVar                mCondVar;

    nsCOMPtr<nsIThread>             mCacheIOThread;

    nsTArray<nsISupports*>          mDoomedObjects;
    
    PRBool                          mInitialized;
    
    PRBool                          mEnableMemoryDevice;
    PRBool                          mEnableDiskDevice;
    PRBool                          mEnableOfflineDevice;

    nsMemoryCacheDevice *           mMemoryDevice;
    nsDiskCacheDevice *             mDiskDevice;
    nsOfflineCacheDevice *          mOfflineDevice;

    nsCacheEntryHashTable           mActiveEntries;
    PRCList                         mDoomedEntries;

    
    
    PRUint32                        mTotalEntries;
    PRUint32                        mCacheHits;
    PRUint32                        mCacheMisses;
    PRUint32                        mMaxKeyLength;
    PRUint32                        mMaxDataSize;
    PRUint32                        mMaxMetaSize;

    
    PRUint32                        mDeactivateFailures;
    PRUint32                        mDeactivatedUnboundEntries;
};







class nsCacheServiceAutoLock {
public:
    nsCacheServiceAutoLock() {
        nsCacheService::Lock();
    }
    ~nsCacheServiceAutoLock() {
        nsCacheService::Unlock();
    }
};

#endif 
