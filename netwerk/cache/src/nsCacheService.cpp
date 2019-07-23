









































#include "necko-config.h"

#include "nsCache.h"
#include "nsCacheService.h"
#include "nsCacheRequest.h"
#include "nsCacheEntry.h"
#include "nsCacheEntryDescriptor.h"
#include "nsCacheDevice.h"
#include "nsMemoryCacheDevice.h"
#include "nsICacheVisitor.h"
#include "nsDiskCacheDevice.h"

#ifdef NECKO_OFFLINE_CACHE
#include "nsDiskCacheDeviceSQL.h"
#endif

#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsILocalFile.h"
#include "nsIOService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "nsVoidArray.h"
#include "nsDeleteDir.h"
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"
#include <math.h>  





#ifdef XP_MAC
#pragma mark nsCacheProfilePrefObserver
#endif

#define DISK_CACHE_ENABLE_PREF      "browser.cache.disk.enable"
#define DISK_CACHE_DIR_PREF         "browser.cache.disk.parent_directory"
#define DISK_CACHE_CAPACITY_PREF    "browser.cache.disk.capacity"
#define DISK_CACHE_MAX_ENTRY_SIZE_PREF "browser.cache.disk.max_entry_size"
#define DISK_CACHE_CAPACITY         51200

#define OFFLINE_CACHE_ENABLE_PREF   "browser.cache.offline.enable"
#define OFFLINE_CACHE_DIR_PREF      "browser.cache.offline.parent_directory"
#define OFFLINE_CACHE_CAPACITY_PREF "browser.cache.offline.capacity"
#define OFFLINE_CACHE_CAPACITY      512000

#define MEMORY_CACHE_ENABLE_PREF    "browser.cache.memory.enable"
#define MEMORY_CACHE_CAPACITY_PREF  "browser.cache.memory.capacity"
#define MEMORY_CACHE_MAX_ENTRY_SIZE_PREF "browser.cache.memory.max_entry_size"

static const char * observerList[] = { 
    "profile-before-change",
    "profile-after-change",
    NS_XPCOM_SHUTDOWN_OBSERVER_ID,
    NS_PRIVATE_BROWSING_SWITCH_TOPIC
};
static const char * prefList[] = { 
#ifdef NECKO_DISK_CACHE
    DISK_CACHE_ENABLE_PREF,
    DISK_CACHE_CAPACITY_PREF,
    DISK_CACHE_DIR_PREF,
#endif
#ifdef NECKO_OFFLINE_CACHE
    OFFLINE_CACHE_ENABLE_PREF,
    OFFLINE_CACHE_CAPACITY_PREF,
    OFFLINE_CACHE_DIR_PREF,
#endif
    MEMORY_CACHE_ENABLE_PREF,
    MEMORY_CACHE_CAPACITY_PREF
};

class nsCacheProfilePrefObserver : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsCacheProfilePrefObserver()
        : mHaveProfile(PR_FALSE)
        , mDiskCacheEnabled(PR_FALSE)
        , mDiskCacheCapacity(0)
        , mOfflineCacheEnabled(PR_FALSE)
        , mOfflineCacheCapacity(0)
        , mMemoryCacheEnabled(PR_TRUE)
        , mMemoryCacheCapacity(-1)
        , mInPrivateBrowsing(PR_FALSE)
    {
    }

    virtual ~nsCacheProfilePrefObserver() {}
    
    nsresult        Install();
    void            Remove();
    nsresult        ReadPrefs(nsIPrefBranch* branch);
    
    PRBool          DiskCacheEnabled();
    PRInt32         DiskCacheCapacity()         { return mDiskCacheCapacity; }
    nsILocalFile *  DiskCacheParentDirectory()  { return mDiskCacheParentDirectory; }

    PRBool          OfflineCacheEnabled();
    PRInt32         OfflineCacheCapacity()         { return mOfflineCacheCapacity; }
    nsILocalFile *  OfflineCacheParentDirectory()  { return mOfflineCacheParentDirectory; }
    
    PRBool          MemoryCacheEnabled();
    PRInt32         MemoryCacheCapacity();

private:
    PRBool                  mHaveProfile;
    
    PRBool                  mDiskCacheEnabled;
    PRInt32                 mDiskCacheCapacity; 
    nsCOMPtr<nsILocalFile>  mDiskCacheParentDirectory;

    PRBool                  mOfflineCacheEnabled;
    PRInt32                 mOfflineCacheCapacity; 
    nsCOMPtr<nsILocalFile>  mOfflineCacheParentDirectory;
    
    PRBool                  mMemoryCacheEnabled;
    PRInt32                 mMemoryCacheCapacity; 

    PRBool                  mInPrivateBrowsing;
};

NS_IMPL_ISUPPORTS1(nsCacheProfilePrefObserver, nsIObserver)


nsresult
nsCacheProfilePrefObserver::Install()
{
    nsresult rv, rv2 = NS_OK;
    
    
    nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_ARG(observerService);
    
    for (unsigned int i=0; i<NS_ARRAY_LENGTH(observerList); i++) {
        rv = observerService->AddObserver(this, observerList[i], PR_FALSE);
        if (NS_FAILED(rv)) 
            rv2 = rv;
    }
    
    
    nsCOMPtr<nsIPrefBranch2> branch = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!branch) return NS_ERROR_FAILURE;

    for (unsigned int i=0; i<NS_ARRAY_LENGTH(prefList); i++) {
        rv = branch->AddObserver(prefList[i], this, PR_FALSE);
        if (NS_FAILED(rv))
            rv2 = rv;
    }

    
    nsCOMPtr<nsIPrivateBrowsingService> pbs =
      do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbs)
      pbs->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);

    
    
    
    
    
    

    nsCOMPtr<nsIFile>  directory;
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(directory));
    if (NS_SUCCEEDED(rv)) {
        mHaveProfile = PR_TRUE;
    }

    rv = ReadPrefs(branch);
    NS_ENSURE_SUCCESS(rv, rv);

    return rv2;
}


void
nsCacheProfilePrefObserver::Remove()
{
    
    nsCOMPtr<nsIObserverService> obs =
            do_GetService("@mozilla.org/observer-service;1");
    if (obs) {
        for (unsigned int i=0; i<NS_ARRAY_LENGTH(observerList); i++) {
            obs->RemoveObserver(this, observerList[i]);
        }
    }

    
    nsCOMPtr<nsIPrefBranch2> prefs =
           do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        for (unsigned int i=0; i<NS_ARRAY_LENGTH(prefList); i++) {
            
            prefs->RemoveObserver(prefList[i], this);
        }
    }
}


NS_IMETHODIMP
nsCacheProfilePrefObserver::Observe(nsISupports *     subject,
                                    const char *      topic,
                                    const PRUnichar * data_unicode)
{
    nsresult rv;
    NS_ConvertUTF16toUTF8 data(data_unicode);
    CACHE_LOG_ALWAYS(("Observe [topic=%s data=%s]\n", topic, data.get()));

    if (!strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, topic)) {
        
        if (nsCacheService::GlobalInstance())
            nsCacheService::GlobalInstance()->Shutdown();
    
    } else if (!strcmp("profile-before-change", topic)) {
        
        mHaveProfile = PR_FALSE;

        
        nsCacheService::OnProfileShutdown(!strcmp("shutdown-cleanse",
                                                  data.get()));
        
    } else if (!strcmp("profile-after-change", topic)) {
        
        mHaveProfile = PR_TRUE;
        nsCOMPtr<nsIPrefBranch> branch = do_GetService(NS_PREFSERVICE_CONTRACTID);
        ReadPrefs(branch);
        nsCacheService::OnProfileChanged();
    
    } else if (!strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, topic)) {

        
        if (!mHaveProfile)  return NS_OK;

        nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(subject, &rv);
        if (NS_FAILED(rv))  return rv;

#ifdef NECKO_DISK_CACHE
        
        if (!strcmp(DISK_CACHE_ENABLE_PREF, data.get())) {

            if (!mInPrivateBrowsing) {
                rv = branch->GetBoolPref(DISK_CACHE_ENABLE_PREF,
                                         &mDiskCacheEnabled);
                if (NS_FAILED(rv))  return rv;
                nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());
            }

        } else if (!strcmp(DISK_CACHE_CAPACITY_PREF, data.get())) {

            PRInt32 capacity = 0;
            rv = branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &capacity);
            if (NS_FAILED(rv))  return rv;
            mDiskCacheCapacity = PR_MAX(0, capacity);
            nsCacheService::SetDiskCacheCapacity(mDiskCacheCapacity);
#if 0            
        } else if (!strcmp(DISK_CACHE_DIR_PREF, data.get())) {
            
            
            
            
#endif            
        } else 
#endif

#ifdef NECKO_OFFLINE_CACHE
        
        if (!strcmp(OFFLINE_CACHE_ENABLE_PREF, data.get())) {

            if (!mInPrivateBrowsing) {
                rv = branch->GetBoolPref(OFFLINE_CACHE_ENABLE_PREF,
                                         &mOfflineCacheEnabled);
                if (NS_FAILED(rv))  return rv;
                nsCacheService::SetOfflineCacheEnabled(OfflineCacheEnabled());
            }

        } else if (!strcmp(OFFLINE_CACHE_CAPACITY_PREF, data.get())) {

            PRInt32 capacity = 0;
            rv = branch->GetIntPref(OFFLINE_CACHE_CAPACITY_PREF, &capacity);
            if (NS_FAILED(rv))  return rv;
            mOfflineCacheCapacity = PR_MAX(0, capacity);
            nsCacheService::SetOfflineCacheCapacity(mOfflineCacheCapacity);
#if 0
        } else if (!strcmp(OFFLINE_CACHE_DIR_PREF, data.get())) {
            
            
            
            
#endif
        } else
#endif

        if (!strcmp(MEMORY_CACHE_ENABLE_PREF, data.get())) {

            rv = branch->GetBoolPref(MEMORY_CACHE_ENABLE_PREF,
                                     &mMemoryCacheEnabled);
            if (NS_FAILED(rv))  return rv;
            nsCacheService::SetMemoryCache();
            
        } else if (!strcmp(MEMORY_CACHE_CAPACITY_PREF, data.get())) {

            mMemoryCacheCapacity = -1;
            (void) branch->GetIntPref(MEMORY_CACHE_CAPACITY_PREF,
                                      &mMemoryCacheCapacity);
            nsCacheService::SetMemoryCache();
        }
    } else if (!strcmp(NS_PRIVATE_BROWSING_SWITCH_TOPIC, topic)) {
        if (!strcmp(NS_PRIVATE_BROWSING_ENTER, data.get())) {
            mInPrivateBrowsing = PR_TRUE;

            nsCacheService::OnEnterExitPrivateBrowsing();

#ifdef NECKO_DISK_CACHE
            mDiskCacheEnabled = PR_FALSE;
            nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());
#endif 

#ifdef NECKO_OFFLINE_CACHE
            mOfflineCacheEnabled = PR_FALSE;
            nsCacheService::SetOfflineCacheEnabled(OfflineCacheEnabled());
#endif 
        } else if (!strcmp(NS_PRIVATE_BROWSING_LEAVE, data.get())) {
            mInPrivateBrowsing = PR_FALSE;

            nsCacheService::OnEnterExitPrivateBrowsing();

#if defined(NECKO_DISK_CACHE) || defined(NECKO_OFFLINE_CACHE)
            nsCOMPtr<nsIPrefBranch> branch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
            if (NS_FAILED(rv))  return rv;
#endif 

#ifdef NECKO_DISK_CACHE
            mDiskCacheEnabled = PR_TRUE; 
            (void) branch->GetBoolPref(DISK_CACHE_ENABLE_PREF,
                                       &mDiskCacheEnabled);
            nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());
#endif 

#ifdef NECKO_OFFLINE_CACHE
            mOfflineCacheEnabled = PR_TRUE; 
            (void) branch->GetBoolPref(OFFLINE_CACHE_ENABLE_PREF,
                                       &mOfflineCacheEnabled);
            nsCacheService::SetOfflineCacheEnabled(OfflineCacheEnabled());
#endif 
        }
    }
    
    return NS_OK;
}


nsresult
nsCacheProfilePrefObserver::ReadPrefs(nsIPrefBranch* branch)
{
    nsresult rv = NS_OK;

#ifdef NECKO_DISK_CACHE
    
    if (!mInPrivateBrowsing) {
        mDiskCacheEnabled = PR_TRUE;  
        (void) branch->GetBoolPref(DISK_CACHE_ENABLE_PREF, &mDiskCacheEnabled);
    }

    mDiskCacheCapacity = DISK_CACHE_CAPACITY;
    (void)branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &mDiskCacheCapacity);
    mDiskCacheCapacity = PR_MAX(0, mDiskCacheCapacity);

    (void) branch->GetComplexValue(DISK_CACHE_DIR_PREF,     
                                   NS_GET_IID(nsILocalFile),
                                   getter_AddRefs(mDiskCacheParentDirectory));
    
    if (!mDiskCacheParentDirectory) {
        nsCOMPtr<nsIFile>  directory;

        
        rv = NS_GetSpecialDirectory(NS_APP_CACHE_PARENT_DIR,
                                    getter_AddRefs(directory));
        if (NS_FAILED(rv)) {
            
            nsCOMPtr<nsIFile> profDir;
            NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                   getter_AddRefs(profDir));
            NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                   getter_AddRefs(directory));
            if (!directory)
                directory = profDir;
            else if (profDir) {
                PRBool same;
                if (NS_SUCCEEDED(profDir->Equals(directory, &same)) && !same) {
                    
                    
                    rv = profDir->AppendNative(NS_LITERAL_CSTRING("Cache"));
                    if (NS_SUCCEEDED(rv)) {
                        PRBool exists;
                        if (NS_SUCCEEDED(profDir->Exists(&exists)) && exists)
                            DeleteDir(profDir, PR_FALSE, PR_FALSE);
                    }
                }
            }
        }
        
        if (!directory && PR_GetEnv("NECKO_DEV_ENABLE_DISK_CACHE")) {
            rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                        getter_AddRefs(directory));
        }
        if (directory)
            mDiskCacheParentDirectory = do_QueryInterface(directory, &rv);
    }
#endif 

#ifdef NECKO_OFFLINE_CACHE
    
    if (!mInPrivateBrowsing) {
        mOfflineCacheEnabled = PR_TRUE;  
        (void) branch->GetBoolPref(OFFLINE_CACHE_ENABLE_PREF,
                                   &mOfflineCacheEnabled);
    }

    mOfflineCacheCapacity = OFFLINE_CACHE_CAPACITY;
    (void)branch->GetIntPref(OFFLINE_CACHE_CAPACITY_PREF,
                             &mOfflineCacheCapacity);
    mOfflineCacheCapacity = PR_MAX(0, mOfflineCacheCapacity);

    (void) branch->GetComplexValue(OFFLINE_CACHE_DIR_PREF,     
                                   NS_GET_IID(nsILocalFile),
                                   getter_AddRefs(mOfflineCacheParentDirectory));

    if (!mOfflineCacheParentDirectory) {
        nsCOMPtr<nsIFile>  directory;

        
        rv = NS_GetSpecialDirectory(NS_APP_CACHE_PARENT_DIR,
                                    getter_AddRefs(directory));
        if (NS_FAILED(rv)) {
            
            nsCOMPtr<nsIFile> profDir;
            NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                   getter_AddRefs(profDir));
            NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                   getter_AddRefs(directory));
            if (!directory)
                directory = profDir;
        }
#if DEBUG
        if (!directory) {
            
            rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                        getter_AddRefs(directory));
        }
#endif
        if (directory)
            mOfflineCacheParentDirectory = do_QueryInterface(directory, &rv);
    }
#endif 
    
    
    (void) branch->GetBoolPref(MEMORY_CACHE_ENABLE_PREF, &mMemoryCacheEnabled);

    mMemoryCacheCapacity = -1;
    (void) branch->GetIntPref(MEMORY_CACHE_CAPACITY_PREF,
                              &mMemoryCacheCapacity);
        
    return rv;
}


PRBool
nsCacheProfilePrefObserver::DiskCacheEnabled()
{
    if ((mDiskCacheCapacity == 0) || (!mDiskCacheParentDirectory))  return PR_FALSE;
    return mDiskCacheEnabled;
}


PRBool
nsCacheProfilePrefObserver::OfflineCacheEnabled()
{
    if ((mOfflineCacheCapacity == 0) || (!mOfflineCacheParentDirectory))
        return PR_FALSE;

    return mOfflineCacheEnabled;
}


PRBool
nsCacheProfilePrefObserver::MemoryCacheEnabled()
{
    if (mMemoryCacheCapacity == 0)  return PR_FALSE;
    return mMemoryCacheEnabled;
}

































PRInt32
nsCacheProfilePrefObserver::MemoryCacheCapacity()
{
    PRInt32 capacity = mMemoryCacheCapacity;
    if (capacity >= 0) {
        CACHE_LOG_DEBUG(("Memory cache capacity forced to %d\n", capacity));
        return capacity;
    }

    PRUint64 bytes = PR_GetPhysicalMemorySize();
    CACHE_LOG_DEBUG(("Physical Memory size is %llu\n", bytes));

    
    
    
    if (bytes == 0)
        bytes = 32 * 1024 * 1024;

    
    
    
    if (LL_CMP(bytes, >, LL_MAXINT))
        bytes = LL_MAXINT;

    PRUint64 kbytes;
    LL_SHR(kbytes, bytes, 10);

    double kBytesD;
    LL_L2D(kBytesD, (PRInt64) kbytes);

    double x = log(kBytesD)/log(2.0) - 14;
    if (x > 0) {
        capacity = (PRInt32)(x * x / 3.0 + x + 2.0 / 3 + 0.1); 
        if (capacity > 32)
            capacity = 32;
        capacity   *= 1024;
    } else {
        capacity    = 0;
    }

    return capacity;
}




#ifdef XP_MAC
#pragma mark -
#pragma mark nsCacheService
#endif

nsCacheService *   nsCacheService::gService = nsnull;

NS_IMPL_THREADSAFE_ISUPPORTS1(nsCacheService, nsICacheService)

nsCacheService::nsCacheService()
    : mLock(nsnull),
      mInitialized(PR_FALSE),
      mEnableMemoryDevice(PR_TRUE),
      mEnableDiskDevice(PR_TRUE),
      mMemoryDevice(nsnull),
      mDiskDevice(nsnull),
      mOfflineDevice(nsnull),
      mTotalEntries(0),
      mCacheHits(0),
      mCacheMisses(0),
      mMaxKeyLength(0),
      mMaxDataSize(0),
      mMaxMetaSize(0),
      mDeactivateFailures(0),
      mDeactivatedUnboundEntries(0)
{
    NS_ASSERTION(gService==nsnull, "multiple nsCacheService instances!");
    gService = this;

    
    PR_INIT_CLIST(&mDoomedEntries);
  
    
    mLock = PR_NewLock();

#if defined(DEBUG)
    mLockedThread = nsnull;
#endif
}

nsCacheService::~nsCacheService()
{
    if (mInitialized) 
        (void) Shutdown();

    PR_DestroyLock(mLock);
    gService = nsnull;
}


nsresult
nsCacheService::Init()
{
    NS_ASSERTION(!mInitialized, "nsCacheService already initialized.");
    if (mInitialized)
        return NS_ERROR_ALREADY_INITIALIZED;

    if (mLock == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    CACHE_LOG_INIT();

    
    nsresult rv = mActiveEntries.Init();
    if (NS_FAILED(rv)) return rv;
    
    
    mObserver = new nsCacheProfilePrefObserver();
    if (!mObserver)  return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mObserver);
    
    mObserver->Install();
    mEnableDiskDevice    = mObserver->DiskCacheEnabled();
    mEnableOfflineDevice = mObserver->OfflineCacheEnabled();
    mEnableMemoryDevice  = mObserver->MemoryCacheEnabled();

    mInitialized = PR_TRUE;
    return NS_OK;
}


void
nsCacheService::Shutdown()
{
    nsCacheServiceAutoLock lock;
    NS_ASSERTION(mInitialized, 
                 "can't shutdown nsCacheService unless it has been initialized.");

    if (mInitialized) {

        mInitialized = PR_FALSE;

        mObserver->Remove();
        NS_RELEASE(mObserver);
        
        
        ClearDoomList();
        ClearActiveEntries();

        
        delete mMemoryDevice;
        mMemoryDevice = nsnull;

#ifdef NECKO_DISK_CACHE
        delete mDiskDevice;
        mDiskDevice = nsnull;
#endif 

#ifdef NECKO_OFFLINE_CACHE
        NS_IF_RELEASE(mOfflineDevice);
#endif 

#if defined(NECKO_DISK_CACHE) && defined(PR_LOGGING)
        LogCacheStatistics();
#endif
    }
}


NS_METHOD
nsCacheService::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult)
{
    nsresult  rv;

    if (aOuter != nsnull)
        return NS_ERROR_NO_AGGREGATION;

    nsCacheService * cacheService = new nsCacheService();
    if (cacheService == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(cacheService);
    rv = cacheService->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = cacheService->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(cacheService);
    return rv;
}


NS_IMETHODIMP
nsCacheService::CreateSession(const char *          clientID,
                              nsCacheStoragePolicy  storagePolicy, 
                              PRBool                streamBased,
                              nsICacheSession     **result)
{
    *result = nsnull;

    if (this == nsnull)  return NS_ERROR_NOT_AVAILABLE;

    nsCacheSession * session = new nsCacheSession(clientID, storagePolicy, streamBased);
    if (!session)  return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*result = session);

    return NS_OK;
}


nsresult
nsCacheService::EvictEntriesForSession(nsCacheSession * session)
{
    NS_ASSERTION(gService, "nsCacheService::gService is null.");
    return gService->EvictEntriesForClient(session->ClientID()->get(),
                                 session->StoragePolicy());
}


nsresult
nsCacheService::EvictEntriesForClient(const char *          clientID,
                                      nsCacheStoragePolicy  storagePolicy)
{
    if (this == nsnull) return NS_ERROR_NOT_AVAILABLE; 

    nsCOMPtr<nsIObserverService> obsSvc =
        do_GetService("@mozilla.org/observer-service;1");
    if (obsSvc) {
        
        
        

        nsCOMPtr<nsIObserverService> obsProxy;
        NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                             NS_GET_IID(nsIObserverService), obsSvc,
                             NS_PROXY_ASYNC, getter_AddRefs(obsProxy));

        if (obsProxy) {
            obsProxy->NotifyObservers(this,
                                      NS_CACHESERVICE_EMPTYCACHE_TOPIC_ID,
                                      nsnull);
        }
    }

    nsCacheServiceAutoLock lock;
    nsresult rv = NS_OK;

#ifdef NECKO_DISK_CACHE
    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_ON_DISK) {

        if (mEnableDiskDevice) {
            if (!mDiskDevice) {
                rv = CreateDiskDevice();
                if (NS_FAILED(rv)) return rv;
            }
            rv = mDiskDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) return rv;
        }
    }
#endif 

#ifdef NECKO_OFFLINE_CACHE
    
    if (storagePolicy == nsICache::STORE_OFFLINE) {
        if (mEnableOfflineDevice) {
            if (!mOfflineDevice) {
                rv = CreateOfflineDevice();
                if (NS_FAILED(rv)) return rv;
            }
            rv = mOfflineDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) return rv;
        }
    }
#endif 

    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_IN_MEMORY) {

        
        if (mMemoryDevice) {
            rv = mMemoryDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}


nsresult        
nsCacheService::IsStorageEnabledForPolicy(nsCacheStoragePolicy  storagePolicy,
                                          PRBool *              result)
{
    if (gService == nsnull) return NS_ERROR_NOT_AVAILABLE;
    nsCacheServiceAutoLock lock;

    *result = gService->IsStorageEnabledForPolicy_Locked(storagePolicy);
    return NS_OK;
}


PRBool        
nsCacheService::IsStorageEnabledForPolicy_Locked(nsCacheStoragePolicy  storagePolicy)
{
    if (gService->mEnableMemoryDevice &&
        (storagePolicy == nsICache::STORE_ANYWHERE ||
         storagePolicy == nsICache::STORE_IN_MEMORY)) {
        return PR_TRUE;
    }
    if (gService->mEnableDiskDevice &&
        (storagePolicy == nsICache::STORE_ANYWHERE ||
         storagePolicy == nsICache::STORE_ON_DISK  ||
         storagePolicy == nsICache::STORE_ON_DISK_AS_FILE)) {
        return PR_TRUE;
    }
    if (gService->mEnableOfflineDevice &&
        storagePolicy == nsICache::STORE_OFFLINE) {
        return PR_TRUE;
    }
    
    return PR_FALSE;
}

NS_IMETHODIMP nsCacheService::VisitEntries(nsICacheVisitor *visitor)
{
    NS_ENSURE_ARG_POINTER(visitor);

    nsCacheServiceAutoLock lock;

    if (!(mEnableDiskDevice || mEnableMemoryDevice))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    nsresult rv = NS_OK;
    
    if (mMemoryDevice) {
        rv = mMemoryDevice->Visit(visitor);
        if (NS_FAILED(rv)) return rv;
    }

#ifdef NECKO_DISK_CACHE
    if (mEnableDiskDevice) {
        if (!mDiskDevice) {
            rv = CreateDiskDevice();
            if (NS_FAILED(rv)) return rv;
        }
        rv = mDiskDevice->Visit(visitor);
        if (NS_FAILED(rv)) return rv;
    }
#endif 

#ifdef NECKO_OFFLINE_CACHE
    if (mEnableOfflineDevice) {
        if (!mOfflineDevice) {
            rv = CreateOfflineDevice();
            if (NS_FAILED(rv)) return rv;
        }
        rv = mOfflineDevice->Visit(visitor);
        if (NS_FAILED(rv)) return rv;
    }
#endif 

    
    

    return NS_OK;
}


NS_IMETHODIMP nsCacheService::EvictEntries(nsCacheStoragePolicy storagePolicy)
{
    return  EvictEntriesForClient(nsnull, storagePolicy);
}




nsresult
nsCacheService::CreateDiskDevice()
{
#ifdef NECKO_DISK_CACHE
    if (!mInitialized)      return NS_ERROR_NOT_AVAILABLE;
    if (!mEnableDiskDevice) return NS_ERROR_NOT_AVAILABLE;
    if (mDiskDevice)        return NS_OK;

    mDiskDevice = new nsDiskCacheDevice;
    if (!mDiskDevice)       return NS_ERROR_OUT_OF_MEMORY;

    
    mDiskDevice->SetCacheParentDirectory(mObserver->DiskCacheParentDirectory());
    mDiskDevice->SetCapacity(mObserver->DiskCacheCapacity());
    
    nsresult rv = mDiskDevice->Init();
    if (NS_FAILED(rv)) {
#if DEBUG
        printf("###\n");
        printf("### mDiskDevice->Init() failed (0x%.8x)\n", rv);
        printf("###    - disabling disk cache for this session.\n");
        printf("###\n");
#endif        
        mEnableDiskDevice = PR_FALSE;
        delete mDiskDevice;
        mDiskDevice = nsnull;
    }
    return rv;
#else 
    NS_NOTREACHED("nsCacheService::CreateDiskDevice");
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

nsresult
nsCacheService::CreateOfflineDevice()
{
#ifdef NECKO_OFFLINE_CACHE
    CACHE_LOG_ALWAYS(("Creating offline device"));

    if (!mInitialized)         return NS_ERROR_NOT_AVAILABLE;
    if (!mEnableOfflineDevice) return NS_ERROR_NOT_AVAILABLE;
    if (mOfflineDevice)        return NS_OK;

    mOfflineDevice = new nsOfflineCacheDevice;
    if (!mOfflineDevice)       return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(mOfflineDevice);

    
    mOfflineDevice->SetCacheParentDirectory(
        mObserver->OfflineCacheParentDirectory());
    mOfflineDevice->SetCapacity(mObserver->OfflineCacheCapacity());

    nsresult rv = mOfflineDevice->Init();
    if (NS_FAILED(rv)) {
        CACHE_LOG_DEBUG(("mOfflineDevice->Init() failed (0x%.8x)\n", rv));
        CACHE_LOG_DEBUG(("    - disabling offline cache for this session.\n"));

        mEnableOfflineDevice = PR_FALSE;
        NS_RELEASE(mOfflineDevice);
    }
    return rv;
#else 
    NS_NOTREACHED("nsCacheService::CreateOfflineDevice");
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

nsresult
nsCacheService::CreateMemoryDevice()
{
    if (!mInitialized)        return NS_ERROR_NOT_AVAILABLE;
    if (!mEnableMemoryDevice) return NS_ERROR_NOT_AVAILABLE;
    if (mMemoryDevice)        return NS_OK;

    mMemoryDevice = new nsMemoryCacheDevice;
    if (!mMemoryDevice)       return NS_ERROR_OUT_OF_MEMORY;
    
    
    PRInt32 capacity = mObserver->MemoryCacheCapacity();
    CACHE_LOG_DEBUG(("Creating memory device with capacity %d\n", capacity));
    mMemoryDevice->SetCapacity(capacity);

    nsresult rv = mMemoryDevice->Init();
    if (NS_FAILED(rv)) {
        NS_WARNING("Initialization of Memory Cache failed.");
        delete mMemoryDevice;
        mMemoryDevice = nsnull;
    }
    return rv;
}


nsresult
nsCacheService::CreateRequest(nsCacheSession *   session,
                              const nsACString & clientKey,
                              nsCacheAccessMode  accessRequested,
                              PRBool             blockingMode,
                              nsICacheListener * listener,
                              nsCacheRequest **  request)
{
    NS_ASSERTION(request, "CreateRequest: request is null");
     
    nsCString * key = new nsCString(*session->ClientID());
    if (!key)
        return NS_ERROR_OUT_OF_MEMORY;
    key->Append(':');
    key->Append(clientKey);

    if (mMaxKeyLength < key->Length()) mMaxKeyLength = key->Length();

    
    *request = new  nsCacheRequest(key, listener, accessRequested, blockingMode, session);    
    if (!*request) {
        delete key;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!listener)  return NS_OK;  

    
    (*request)->mThread = do_GetCurrentThread();
    
    return NS_OK;
}


class nsCacheListenerEvent : public nsRunnable
{
public:
    nsCacheListenerEvent(nsICacheListener *listener,
                         nsICacheEntryDescriptor *descriptor,
                         nsCacheAccessMode accessGranted,
                         nsresult status)
        : mListener(listener)      
        , mDescriptor(descriptor)  
        , mAccessGranted(accessGranted)
        , mStatus(status)
    {}

    NS_IMETHOD Run()
    {
        mListener->OnCacheEntryAvailable(mDescriptor, mAccessGranted, mStatus);

        NS_RELEASE(mListener);
        NS_IF_RELEASE(mDescriptor);
        return NS_OK;
    }

private:
    
    
    

    nsICacheListener        *mListener;
    nsICacheEntryDescriptor *mDescriptor;
    nsCacheAccessMode        mAccessGranted;
    nsresult                 mStatus;
};


nsresult
nsCacheService::NotifyListener(nsCacheRequest *          request,
                               nsICacheEntryDescriptor * descriptor,
                               nsCacheAccessMode         accessGranted,
                               nsresult                  status)
{
    NS_ASSERTION(request->mThread, "no thread set in async request!");

    
    nsICacheListener *listener = request->mListener;
    request->mListener = nsnull;

    nsCOMPtr<nsIRunnable> ev =
            new nsCacheListenerEvent(listener, descriptor,
                                     accessGranted, status);
    if (!ev) {
        
        
        
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return request->mThread->Dispatch(ev, NS_DISPATCH_NORMAL);
}


nsresult
nsCacheService::ProcessRequest(nsCacheRequest *           request,
                               PRBool                     calledFromOpenCacheEntry,
                               nsICacheEntryDescriptor ** result)
{
    
    nsresult           rv;
    nsCacheEntry *     entry = nsnull;
    nsCacheAccessMode  accessGranted = nsICache::ACCESS_NONE;
    if (result) *result = nsnull;

    while(1) {  
        rv = ActivateEntry(request, &entry);  
        if (NS_FAILED(rv))  break;

        while(1) { 
            NS_ASSERTION(entry, "no entry in Request Access loop!");
            
            rv = entry->RequestAccess(request, &accessGranted);
            if (rv != NS_ERROR_CACHE_WAIT_FOR_VALIDATION) break;
            
            if (request->mListener) 
                return rv;
            
            if (request->IsBlocking()) {
                
                Unlock();
                rv = request->WaitForValidation();
                Lock();
            }

            PR_REMOVE_AND_INIT_LINK(request);
            if (NS_FAILED(rv)) break;   
            
        }
        if (rv != NS_ERROR_CACHE_ENTRY_DOOMED)  break;

        if (entry->IsNotInUse()) {
            
            DeactivateEntry(entry);
        }
        
    }

    nsICacheEntryDescriptor *descriptor = nsnull;
    
    if (NS_SUCCEEDED(rv))
        rv = entry->CreateDescriptor(request, accessGranted, &descriptor);

    if (request->mListener) {  
    
        if (NS_FAILED(rv) && calledFromOpenCacheEntry)
            return rv;  
            
        
        nsresult rv2 = NotifyListener(request, descriptor, accessGranted, rv);
        if (NS_FAILED(rv2) && NS_SUCCEEDED(rv)) {
            rv = rv2;  
        }
    } else {        
        *result = descriptor;
    }
    return rv;
}


nsresult
nsCacheService::OpenCacheEntry(nsCacheSession *           session,
                               const nsACString &         key,
                               nsCacheAccessMode          accessRequested,
                               PRBool                     blockingMode,
                               nsICacheListener *         listener,
                               nsICacheEntryDescriptor ** result)
{
    CACHE_LOG_DEBUG(("Opening entry for session %p, key %s, mode %d, blocking %d\n",
                     session, PromiseFlatCString(key).get(), accessRequested,
                     blockingMode));
    NS_ASSERTION(gService, "nsCacheService::gService is null.");
    if (result)
        *result = nsnull;

    if (!gService->mInitialized)
        return NS_ERROR_NOT_INITIALIZED;

    nsCacheRequest * request = nsnull;

    nsCacheServiceAutoLock lock;
    nsresult rv = gService->CreateRequest(session,
                                          key,
                                          accessRequested,
                                          blockingMode,
                                          listener,
                                          &request);
    if (NS_FAILED(rv))  return rv;

    CACHE_LOG_DEBUG(("Created request %p\n", request));

    rv = gService->ProcessRequest(request, PR_TRUE, result);

    
    if (!(listener && (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION)))
        delete request;

    return rv;
}


nsresult
nsCacheService::ActivateEntry(nsCacheRequest * request, 
                              nsCacheEntry ** result)
{
    CACHE_LOG_DEBUG(("Activate entry for request %p\n", request));
    
    nsresult        rv = NS_OK;

    NS_ASSERTION(request != nsnull, "ActivateEntry called with no request");
    if (result) *result = nsnull;
    if ((!request) || (!result))  return NS_ERROR_NULL_POINTER;

    
    if (!mEnableMemoryDevice && !request->IsStreamBased())
        return NS_ERROR_FAILURE;
    if (!IsStorageEnabledForPolicy_Locked(request->StoragePolicy()))
        return NS_ERROR_FAILURE;

    
    nsCacheEntry *entry = mActiveEntries.GetEntry(request->mKey);
    CACHE_LOG_DEBUG(("Active entry for request %p is %p\n", request, entry));

    if (!entry) {
        
        PRBool collision = PR_FALSE;
        entry = SearchCacheDevices(request->mKey, request->StoragePolicy(), &collision);
        CACHE_LOG_DEBUG(("Device search for request %p returned %p\n",
                         request, entry));
        
        if (collision) return NS_ERROR_CACHE_IN_USE;

        if (entry)  entry->MarkInitialized();
    }

    if (entry) {
        ++mCacheHits;
        entry->Fetched();
    } else {
        ++mCacheMisses;
    }

    if (entry &&
        ((request->AccessRequested() == nsICache::ACCESS_WRITE) ||
         ((request->StoragePolicy() != nsICache::STORE_OFFLINE) &&
          (entry->mExpirationTime <= SecondsFromPRTime(PR_Now()) &&
           request->WillDoomEntriesIfExpired()))))

    {
        
        rv = DoomEntry_Internal(entry);
        if (NS_FAILED(rv)) {
            
        }
        entry = nsnull;
    }

    if (!entry) {
        if (! (request->AccessRequested() & nsICache::ACCESS_WRITE)) {
            
            rv = NS_ERROR_CACHE_KEY_NOT_FOUND;
            goto error;
        }

        entry = new nsCacheEntry(request->mKey,
                                 request->IsStreamBased(),
                                 request->StoragePolicy());
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        
        entry->Fetched();
        ++mTotalEntries;
        
        
    }

    if (!entry->IsActive()) {
        rv = mActiveEntries.AddEntry(entry);
        if (NS_FAILED(rv)) goto error;
        CACHE_LOG_DEBUG(("Added entry %p to mActiveEntries\n", entry));
        entry->MarkActive();  
    }
    *result = entry;
    return NS_OK;
    
 error:
    *result = nsnull;
    if (entry) {
        delete entry;
    }
    return rv;
}


nsCacheEntry *
nsCacheService::SearchCacheDevices(nsCString * key, nsCacheStoragePolicy policy, PRBool *collision)
{
    nsCacheEntry * entry = nsnull;

    CACHE_LOG_DEBUG(("mMemoryDevice: 0x%p\n", mMemoryDevice));

    *collision = PR_FALSE;
    if ((policy == nsICache::STORE_ANYWHERE) || (policy == nsICache::STORE_IN_MEMORY)) {
        
        if (mMemoryDevice) {
            entry = mMemoryDevice->FindEntry(key, collision);
            CACHE_LOG_DEBUG(("Searching mMemoryDevice for key %s found: 0x%p, "
                             "collision: %d\n", key->get(), entry, collision));
        }
    }

    if (!entry && 
        ((policy == nsICache::STORE_ANYWHERE) || (policy == nsICache::STORE_ON_DISK))) {

#ifdef NECKO_DISK_CACHE
        if (mEnableDiskDevice) {
            if (!mDiskDevice) {
                nsresult rv = CreateDiskDevice();
                if (NS_FAILED(rv))
                    return nsnull;
            }
            
            entry = mDiskDevice->FindEntry(key, collision);
        }
#endif 
    }

    if (!entry && (policy == nsICache::STORE_OFFLINE ||
                   (policy == nsICache::STORE_ANYWHERE &&
                    gIOService->IsOffline()))) {

#ifdef NECKO_OFFLINE_CACHE
        if (mEnableOfflineDevice) {
            if (!mOfflineDevice) {
                nsresult rv = CreateOfflineDevice();
                if (NS_FAILED(rv))
                    return nsnull;
            }

            entry = mOfflineDevice->FindEntry(key, collision);
        }
#endif 
    }

    return entry;
}


nsCacheDevice *
nsCacheService::EnsureEntryHasDevice(nsCacheEntry * entry)
{
    nsCacheDevice * device = entry->CacheDevice();
    if (device)  return device;

#ifdef NECKO_DISK_CACHE
    if (entry->IsStreamData() && entry->IsAllowedOnDisk() && mEnableDiskDevice) {
        
        if (!mDiskDevice) {
            (void)CreateDiskDevice();  
        }

        if (mDiskDevice) {
            entry->MarkBinding();  
            nsresult rv = mDiskDevice->BindEntry(entry);
            entry->ClearBinding(); 
            if (NS_SUCCEEDED(rv))
                device = mDiskDevice;
        }
    }
#endif 
     
    
    if (!device && mEnableMemoryDevice && entry->IsAllowedInMemory()) {        
        if (!mMemoryDevice) {
            (void)CreateMemoryDevice();  
        }
        if (mMemoryDevice) {
            entry->MarkBinding();  
            nsresult rv = mMemoryDevice->BindEntry(entry);
            entry->ClearBinding(); 
            if (NS_SUCCEEDED(rv))
                device = mMemoryDevice;
        }
    }

#ifdef NECKO_OFFLINE_CACHE
    if (!device && entry->IsStreamData() &&
        entry->IsAllowedOffline() && mEnableOfflineDevice) {
        if (!mOfflineDevice) {
            (void)CreateOfflineDevice(); 
        }

        if (mOfflineDevice) {
            entry->MarkBinding();
            nsresult rv = mOfflineDevice->BindEntry(entry);
            entry->ClearBinding();
            if (NS_SUCCEEDED(rv))
                device = mOfflineDevice;
        }
    }
#endif 

    if (device) 
        entry->SetCacheDevice(device);
    return device;
}


nsresult
nsCacheService::DoomEntry(nsCacheEntry * entry)
{
    return gService->DoomEntry_Internal(entry);
}


nsresult
nsCacheService::DoomEntry_Internal(nsCacheEntry * entry)
{
    if (entry->IsDoomed())  return NS_OK;
    
    CACHE_LOG_DEBUG(("Dooming entry %p\n", entry));
    nsresult  rv = NS_OK;
    entry->MarkDoomed();
    
    NS_ASSERTION(!entry->IsBinding(), "Dooming entry while binding device.");
    nsCacheDevice * device = entry->CacheDevice();
    if (device)  device->DoomEntry(entry);

    if (entry->IsActive()) {
        
        mActiveEntries.RemoveEntry(entry);
        CACHE_LOG_DEBUG(("Removed entry %p from mActiveEntries\n", entry));
        entry->MarkInactive();
     }

    
    NS_ASSERTION(PR_CLIST_IS_EMPTY(entry), "doomed entry still on device list");
    PR_APPEND_LINK(entry, &mDoomedEntries);

    
    rv = ProcessPendingRequests(entry);
    
    
    if (entry->IsNotInUse()) {
        DeactivateEntry(entry); 
    }
    return rv;
}


void
nsCacheService::OnProfileShutdown(PRBool cleanse)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;

    gService->DoomActiveEntries();
    gService->ClearDoomList();

#ifdef NECKO_DISK_CACHE
    if (gService->mDiskDevice && gService->mEnableDiskDevice) {
        if (cleanse)
            gService->mDiskDevice->EvictEntries(nsnull);

        gService->mDiskDevice->Shutdown();
    }
    gService->mEnableDiskDevice = PR_FALSE;
#endif 

#ifdef NECKO_OFFLINE_CACHE
    if (gService->mOfflineDevice && gService->mEnableOfflineDevice) {
        if (cleanse)
            gService->mOfflineDevice->EvictEntries(nsnull);

        gService->mOfflineDevice->Shutdown();
    }
    gService->mEnableOfflineDevice = PR_FALSE;
#endif 

    if (gService->mMemoryDevice) {
        
        gService->mMemoryDevice->EvictEntries(nsnull);
    }

}


void
nsCacheService::OnProfileChanged()
{
    if (!gService)  return;

    CACHE_LOG_DEBUG(("nsCacheService::OnProfileChanged"));
 
    nsCacheServiceAutoLock lock;
    
    gService->mEnableDiskDevice    = gService->mObserver->DiskCacheEnabled();
    gService->mEnableOfflineDevice = gService->mObserver->OfflineCacheEnabled();
    gService->mEnableMemoryDevice  = gService->mObserver->MemoryCacheEnabled();

#ifdef NECKO_DISK_CACHE
    if (gService->mDiskDevice) {
        gService->mDiskDevice->SetCacheParentDirectory(gService->mObserver->DiskCacheParentDirectory());
        gService->mDiskDevice->SetCapacity(gService->mObserver->DiskCacheCapacity());

        
        nsresult rv = gService->mDiskDevice->Init();
        if (NS_FAILED(rv)) {
            NS_ERROR("nsCacheService::OnProfileChanged: Re-initializing disk device failed");
            gService->mEnableDiskDevice = PR_FALSE;
            
        }
    }
#endif 

#ifdef NECKO_OFFLINE_CACHE
    if (gService->mOfflineDevice) {
        gService->mOfflineDevice->SetCacheParentDirectory(gService->mObserver->OfflineCacheParentDirectory());
        gService->mOfflineDevice->SetCapacity(gService->mObserver->OfflineCacheCapacity());

        
        nsresult rv = gService->mOfflineDevice->Init();
        if (NS_FAILED(rv)) {
            NS_ERROR("nsCacheService::OnProfileChanged: Re-initializing offline device failed");
            gService->mEnableOfflineDevice = PR_FALSE;
            
        }
    }
#endif 
    
    
    if (gService->mMemoryDevice) {
        if (gService->mEnableMemoryDevice) {
            
            PRInt32 capacity = gService->mObserver->MemoryCacheCapacity();
            CACHE_LOG_DEBUG(("Resetting memory device capacity to %d\n",
                             capacity));
            gService->mMemoryDevice->SetCapacity(capacity);
        } else {
            
            CACHE_LOG_DEBUG(("memory device disabled\n"));
            gService->mMemoryDevice->SetCapacity(0);
            
        }
    }
}


void
nsCacheService::SetDiskCacheEnabled(PRBool  enabled)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;
    gService->mEnableDiskDevice = enabled;
}


void
nsCacheService::SetDiskCacheCapacity(PRInt32  capacity)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;

#ifdef NECKO_DISK_CACHE
    if (gService->mDiskDevice) {
        gService->mDiskDevice->SetCapacity(capacity);
    }
#endif 
    
    gService->mEnableDiskDevice = gService->mObserver->DiskCacheEnabled();
}

void
nsCacheService::SetOfflineCacheEnabled(PRBool  enabled)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;
    gService->mEnableOfflineDevice = enabled;
}

void
nsCacheService::SetOfflineCacheCapacity(PRInt32  capacity)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;

#ifdef NECKO_OFFLINE_CACHE
    if (gService->mOfflineDevice) {
        gService->mOfflineDevice->SetCapacity(capacity);
    }
#endif 

    gService->mEnableOfflineDevice = gService->mObserver->OfflineCacheEnabled();
}


void
nsCacheService::SetMemoryCache()
{
    if (!gService)  return;

    CACHE_LOG_DEBUG(("nsCacheService::SetMemoryCache"));

    nsCacheServiceAutoLock lock;

    gService->mEnableMemoryDevice = gService->mObserver->MemoryCacheEnabled();

    if (gService->mEnableMemoryDevice) {
        if (gService->mMemoryDevice) {
            PRInt32 capacity = gService->mObserver->MemoryCacheCapacity();
            
            CACHE_LOG_DEBUG(("Resetting memory device capacity to %d\n",
                             capacity));
            gService->mMemoryDevice->SetCapacity(capacity);
        }
    } else {
        if (gService->mMemoryDevice) {
            
            CACHE_LOG_DEBUG(("memory device disabled\n"));
            gService->mMemoryDevice->SetCapacity(0);
            
        }
    }
}





#ifdef XP_MAC
#pragma mark -
#endif

void
nsCacheService::CloseDescriptor(nsCacheEntryDescriptor * descriptor)
{
    
    nsCacheEntry * entry       = descriptor->CacheEntry();
    PRBool         stillActive = entry->RemoveDescriptor(descriptor);
    nsresult       rv          = NS_OK;

    if (!entry->IsValid()) {
        rv = gService->ProcessPendingRequests(entry);
    }

    if (!stillActive) {
        gService->DeactivateEntry(entry);
    }
}


nsresult        
nsCacheService::GetFileForEntry(nsCacheEntry *         entry,
                                nsIFile **             result)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;
    
    return device->GetFileForEntry(entry, result);
}


nsresult
nsCacheService::OpenInputStreamForEntry(nsCacheEntry *     entry,
                                        nsCacheAccessMode  mode,
                                        PRUint32           offset,
                                        nsIInputStream  ** result)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;

    return device->OpenInputStreamForEntry(entry, mode, offset, result);
}

nsresult
nsCacheService::OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                         nsCacheAccessMode  mode,
                                         PRUint32           offset,
                                         nsIOutputStream ** result)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;

    return device->OpenOutputStreamForEntry(entry, mode, offset, result);
}


nsresult
nsCacheService::OnDataSizeChange(nsCacheEntry * entry, PRInt32 deltaSize)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;

    return device->OnDataSizeChange(entry, deltaSize);
}

void
nsCacheService::Lock()
{
    PR_Lock(gService->mLock);

#if defined(DEBUG)
    gService->mLockedThread = PR_GetCurrentThread();
#endif
}

void
nsCacheService::Unlock()
{
    NS_ASSERTION(gService->mLockedThread == PR_GetCurrentThread(), "oops");

    nsTArray<nsISupports*> doomed;
    doomed.SwapElements(gService->mDoomedObjects);

#if defined(DEBUG)
    gService->mLockedThread = nsnull;
#endif
    PR_Unlock(gService->mLock);

    for (PRUint32 i = 0; i < doomed.Length(); ++i)
        doomed[i]->Release();
}

void
nsCacheService::ReleaseObject_Locked(nsISupports * obj,
                                     nsIEventTarget * target)
{
    NS_ASSERTION(gService->mLockedThread == PR_GetCurrentThread(), "oops");

    PRBool isCur;
    if (!target || NS_SUCCEEDED(target->IsOnCurrentThread(&isCur)) && isCur) {
        gService->mDoomedObjects.AppendElement(obj);
    } else {
        NS_ProxyRelease(target, obj);
    }
}


nsresult
nsCacheService::SetCacheElement(nsCacheEntry * entry, nsISupports * element)
{
    entry->SetData(element);
    entry->TouchData();
    return NS_OK;
}


nsresult
nsCacheService::ValidateEntry(nsCacheEntry * entry)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;

    entry->MarkValid();
    nsresult rv = gService->ProcessPendingRequests(entry);
    NS_ASSERTION(rv == NS_OK, "ProcessPendingRequests failed.");
    

    return rv;
}

#ifdef XP_MAC
#pragma mark -
#endif


void
nsCacheService::DeactivateEntry(nsCacheEntry * entry)
{
    CACHE_LOG_DEBUG(("Deactivating entry %p\n", entry));
    nsresult  rv = NS_OK;
    NS_ASSERTION(entry->IsNotInUse(), "### deactivating an entry while in use!");
    nsCacheDevice * device = nsnull;

    if (mMaxDataSize < entry->DataSize() )     mMaxDataSize = entry->DataSize();
    if (mMaxMetaSize < entry->MetaDataSize() ) mMaxMetaSize = entry->MetaDataSize();

    if (entry->IsDoomed()) {
        
        PR_REMOVE_AND_INIT_LINK(entry);
    } else if (entry->IsActive()) {
        
        mActiveEntries.RemoveEntry(entry);
        CACHE_LOG_DEBUG(("Removed deactivated entry %p from mActiveEntries\n",
                         entry));
        entry->MarkInactive();

        
        device = EnsureEntryHasDevice(entry); 
        if (!device) {
            CACHE_LOG_DEBUG(("DeactivateEntry: unable to bind active "
                             "entry %p\n",
                             entry));
            NS_WARNING("DeactivateEntry: unable to bind active entry\n");
            return;
        }
    } else {
        
        
        NS_ASSERTION(!mInitialized, "DeactivateEntry: bad cache entry state.");
    }

    device = entry->CacheDevice();
    if (device) {
        rv = device->DeactivateEntry(entry);
        if (NS_FAILED(rv)) {
            
            ++mDeactivateFailures;
        }
    } else {
        
        ++mDeactivatedUnboundEntries;
        delete entry; 
    }
}


nsresult
nsCacheService::ProcessPendingRequests(nsCacheEntry * entry)
{
    nsresult            rv = NS_OK;
    nsCacheRequest *    request = (nsCacheRequest *)PR_LIST_HEAD(&entry->mRequestQ);
    nsCacheRequest *    nextRequest;
    PRBool              newWriter = PR_FALSE;
    
    if (request == &entry->mRequestQ)  return NS_OK;    

    if (!entry->IsDoomed() && entry->IsInvalid()) {
        
        NS_ASSERTION(PR_CLIST_IS_EMPTY(&entry->mDescriptorQ), "shouldn't be here with open descriptors");

#if DEBUG
        
        while (request != &entry->mRequestQ) {
            NS_ASSERTION(request->AccessRequested() != nsICache::ACCESS_WRITE,
                         "ACCESS_WRITE request should have been given a new entry");
            request = (nsCacheRequest *)PR_NEXT_LINK(request);
        }
        request = (nsCacheRequest *)PR_LIST_HEAD(&entry->mRequestQ);        
#endif
        
        while (request != &entry->mRequestQ) {
            if (request->AccessRequested() == nsICache::ACCESS_READ_WRITE) {
                newWriter = PR_TRUE;
                break;
            }

            request = (nsCacheRequest *)PR_NEXT_LINK(request);
        }
        
        if (request == &entry->mRequestQ)   
            request = (nsCacheRequest *)PR_LIST_HEAD(&entry->mRequestQ);
        
        
        
        
    }

    nsCacheAccessMode  accessGranted = nsICache::ACCESS_NONE;

    while (request != &entry->mRequestQ) {
        nextRequest = (nsCacheRequest *)PR_NEXT_LINK(request);

        if (request->mListener) {

            
            PR_REMOVE_AND_INIT_LINK(request);

            if (entry->IsDoomed()) {
                rv = ProcessRequest(request, PR_FALSE, nsnull);
                if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION)
                    rv = NS_OK;
                else
                    delete request;

                if (NS_FAILED(rv)) {
                    
                }
            } else if (entry->IsValid() || newWriter) {
                rv = entry->RequestAccess(request, &accessGranted);
                NS_ASSERTION(NS_SUCCEEDED(rv),
                             "if entry is valid, RequestAccess must succeed.");
                

                
                nsICacheEntryDescriptor *descriptor = nsnull;
                rv = entry->CreateDescriptor(request,
                                             accessGranted,
                                             &descriptor);
                
                
                rv = NotifyListener(request, descriptor, accessGranted, rv);
                delete request;
                if (NS_FAILED(rv)) {
                    
                }
                
            } else {
                
            }
        } else {

            
            request->WakeUp();
        }
        if (newWriter)  break;  
        request = nextRequest;
    }

    return NS_OK;
}


void
nsCacheService::ClearPendingRequests(nsCacheEntry * entry)
{
    nsCacheRequest * request = (nsCacheRequest *)PR_LIST_HEAD(&entry->mRequestQ);
    
    while (request != &entry->mRequestQ) {
        nsCacheRequest * next = (nsCacheRequest *)PR_NEXT_LINK(request);

        
        PR_REMOVE_AND_INIT_LINK(request);
        delete request;
        request = next;
    }
}


void
nsCacheService::ClearDoomList()
{
    nsCacheEntry * entry = (nsCacheEntry *)PR_LIST_HEAD(&mDoomedEntries);

    while (entry != &mDoomedEntries) {
        nsCacheEntry * next = (nsCacheEntry *)PR_NEXT_LINK(entry);
        
         entry->DetachDescriptors();
         DeactivateEntry(entry);
         entry = next;
    }        
}


void
nsCacheService::ClearActiveEntries()
{
    mActiveEntries.VisitEntries(DeactivateAndClearEntry, nsnull);
    mActiveEntries.Shutdown();
}


PLDHashOperator
nsCacheService::DeactivateAndClearEntry(PLDHashTable *    table,
                                        PLDHashEntryHdr * hdr,
                                        PRUint32          number,
                                        void *            arg)
{
    nsCacheEntry * entry = ((nsCacheEntryHashTableEntry *)hdr)->cacheEntry;
    NS_ASSERTION(entry, "### active entry = nsnull!");
    gService->ClearPendingRequests(entry);
    entry->DetachDescriptors();
    
    entry->MarkInactive();  
    gService->DeactivateEntry(entry);
    
    return PL_DHASH_REMOVE; 
}


void
nsCacheService::DoomActiveEntries()
{
    nsAutoTArray<nsCacheEntry*, 8> array;

    mActiveEntries.VisitEntries(RemoveActiveEntry, &array);

    PRUint32 count = array.Length();
    for (PRUint32 i=0; i < count; ++i)
        DoomEntry_Internal(array[i]);
}


PLDHashOperator
nsCacheService::RemoveActiveEntry(PLDHashTable *    table,
                                  PLDHashEntryHdr * hdr,
                                  PRUint32          number,
                                  void *            arg)
{
    nsCacheEntry * entry = ((nsCacheEntryHashTableEntry *)hdr)->cacheEntry;
    NS_ASSERTION(entry, "### active entry = nsnull!");

    nsTArray<nsCacheEntry*> * array = (nsTArray<nsCacheEntry*> *) arg;
    NS_ASSERTION(array, "### array = nsnull!");
    array->AppendElement(entry);

    
    entry->MarkInactive();
    return PL_DHASH_REMOVE; 
}


#if defined(PR_LOGGING)
void
nsCacheService::LogCacheStatistics()
{
    PRUint32 hitPercentage = (PRUint32)((((double)mCacheHits) /
        ((double)(mCacheHits + mCacheMisses))) * 100);
    CACHE_LOG_ALWAYS(("\nCache Service Statistics:\n\n"));
    CACHE_LOG_ALWAYS(("    TotalEntries   = %d\n", mTotalEntries));
    CACHE_LOG_ALWAYS(("    Cache Hits     = %d\n", mCacheHits));
    CACHE_LOG_ALWAYS(("    Cache Misses   = %d\n", mCacheMisses));
    CACHE_LOG_ALWAYS(("    Cache Hit %%    = %d%%\n", hitPercentage));
    CACHE_LOG_ALWAYS(("    Max Key Length = %d\n", mMaxKeyLength));
    CACHE_LOG_ALWAYS(("    Max Meta Size  = %d\n", mMaxMetaSize));
    CACHE_LOG_ALWAYS(("    Max Data Size  = %d\n", mMaxDataSize));
    CACHE_LOG_ALWAYS(("\n"));
    CACHE_LOG_ALWAYS(("    Deactivate Failures         = %d\n",
                      mDeactivateFailures));
    CACHE_LOG_ALWAYS(("    Deactivated Unbound Entries = %d\n",
                      mDeactivatedUnboundEntries));
}
#endif


void
nsCacheService::OnEnterExitPrivateBrowsing()
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;

    gService->DoomActiveEntries();

    if (gService->mMemoryDevice) {
        
        gService->mMemoryDevice->EvictEntries(nsnull);
    }
}
