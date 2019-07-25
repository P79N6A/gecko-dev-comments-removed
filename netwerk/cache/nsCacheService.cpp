










































#include "mozilla/Util.h"

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
#include "nsDiskCacheDeviceSQL.h"

#include "nsIMemoryReporter.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
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
#include "mozilla/Util.h" 
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"
#include "nsITimer.h"

#include "mozilla/FunctionTimer.h"

#include "mozilla/net/NeckoCommon.h"

using namespace mozilla;




#define DISK_CACHE_ENABLE_PREF      "browser.cache.disk.enable"
#define DISK_CACHE_DIR_PREF         "browser.cache.disk.parent_directory"
#define DISK_CACHE_SMART_SIZE_FIRST_RUN_PREF\
    "browser.cache.disk.smart_size.first_run"
#define DISK_CACHE_SMART_SIZE_ENABLED_PREF \
    "browser.cache.disk.smart_size.enabled"
#define DISK_CACHE_SMART_SIZE_PREF "browser.cache.disk.smart_size_cached_value"
#define DISK_CACHE_CAPACITY_PREF    "browser.cache.disk.capacity"
#define DISK_CACHE_MAX_ENTRY_SIZE_PREF "browser.cache.disk.max_entry_size"
#define DISK_CACHE_CAPACITY         256000

#define OFFLINE_CACHE_ENABLE_PREF   "browser.cache.offline.enable"
#define OFFLINE_CACHE_DIR_PREF      "browser.cache.offline.parent_directory"
#define OFFLINE_CACHE_CAPACITY_PREF "browser.cache.offline.capacity"
#define OFFLINE_CACHE_CAPACITY      512000

#define MEMORY_CACHE_ENABLE_PREF    "browser.cache.memory.enable"
#define MEMORY_CACHE_CAPACITY_PREF  "browser.cache.memory.capacity"
#define MEMORY_CACHE_MAX_ENTRY_SIZE_PREF "browser.cache.memory.max_entry_size"

#define CACHE_COMPRESSION_LEVEL_PREF "browser.cache.compression_level"
#define CACHE_COMPRESSION_LEVEL     1

#define SANITIZE_ON_SHUTDOWN_PREF   "privacy.sanitize.sanitizeOnShutdown"
#define CLEAR_ON_SHUTDOWN_PREF      "privacy.clearOnShutdown.cache"

static const char * observerList[] = { 
    "profile-before-change",
    "profile-do-change",
    NS_XPCOM_SHUTDOWN_OBSERVER_ID,
    NS_PRIVATE_BROWSING_SWITCH_TOPIC
};
static const char * prefList[] = { 
    DISK_CACHE_ENABLE_PREF,
    DISK_CACHE_SMART_SIZE_ENABLED_PREF,
    DISK_CACHE_CAPACITY_PREF,
    DISK_CACHE_DIR_PREF,
    DISK_CACHE_MAX_ENTRY_SIZE_PREF,
    OFFLINE_CACHE_ENABLE_PREF,
    OFFLINE_CACHE_CAPACITY_PREF,
    OFFLINE_CACHE_DIR_PREF,
    MEMORY_CACHE_ENABLE_PREF,
    MEMORY_CACHE_CAPACITY_PREF,
    MEMORY_CACHE_MAX_ENTRY_SIZE_PREF,
    CACHE_COMPRESSION_LEVEL_PREF,
    SANITIZE_ON_SHUTDOWN_PREF,
    CLEAR_ON_SHUTDOWN_PREF
};


const PRInt32 DEFAULT_CACHE_SIZE = 250 * 1024;  
const PRInt32 MIN_CACHE_SIZE = 50 * 1024;       
const PRInt32 MAX_CACHE_SIZE = 1024 * 1024;     

const PRInt32 PRE_GECKO_2_0_DEFAULT_CACHE_SIZE = 50 * 1024;

class nsCacheProfilePrefObserver : public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsCacheProfilePrefObserver()
        : mHaveProfile(false)
        , mDiskCacheEnabled(false)
        , mDiskCacheCapacity(0)
        , mDiskCacheMaxEntrySize(-1) 
        , mSmartSizeEnabled(false)
        , mOfflineCacheEnabled(false)
        , mOfflineCacheCapacity(0)
        , mMemoryCacheEnabled(true)
        , mMemoryCacheCapacity(-1)
        , mMemoryCacheMaxEntrySize(-1) 
        , mInPrivateBrowsing(false)
        , mCacheCompressionLevel(CACHE_COMPRESSION_LEVEL)
        , mSanitizeOnShutdown(false)
        , mClearCacheOnShutdown(false)
    {
    }

    virtual ~nsCacheProfilePrefObserver() {}
    
    nsresult        Install();
    void            Remove();
    nsresult        ReadPrefs(nsIPrefBranch* branch);
    
    bool            DiskCacheEnabled();
    PRInt32         DiskCacheCapacity()         { return mDiskCacheCapacity; }
    void            SetDiskCacheCapacity(PRInt32);
    PRInt32         DiskCacheMaxEntrySize()     { return mDiskCacheMaxEntrySize; }
    nsILocalFile *  DiskCacheParentDirectory()  { return mDiskCacheParentDirectory; }
    bool            SmartSizeEnabled()          { return mSmartSizeEnabled; }

    bool            OfflineCacheEnabled();
    PRInt32         OfflineCacheCapacity()         { return mOfflineCacheCapacity; }
    nsILocalFile *  OfflineCacheParentDirectory()  { return mOfflineCacheParentDirectory; }
    
    bool            MemoryCacheEnabled();
    PRInt32         MemoryCacheCapacity();
    PRInt32         MemoryCacheMaxEntrySize()     { return mMemoryCacheMaxEntrySize; }

    PRInt32         CacheCompressionLevel();

    bool            SanitizeAtShutdown() { return mSanitizeOnShutdown && mClearCacheOnShutdown; }

    static PRUint32 GetSmartCacheSize(const nsAString& cachePath,
                                      PRUint32 currentSize);

private:
    bool                    PermittedToSmartSize(nsIPrefBranch*, bool firstRun);
    bool                    mHaveProfile;
    
    bool                    mDiskCacheEnabled;
    PRInt32                 mDiskCacheCapacity; 
    PRInt32                 mDiskCacheMaxEntrySize; 
    nsCOMPtr<nsILocalFile>  mDiskCacheParentDirectory;
    bool                    mSmartSizeEnabled;

    bool                    mOfflineCacheEnabled;
    PRInt32                 mOfflineCacheCapacity; 
    nsCOMPtr<nsILocalFile>  mOfflineCacheParentDirectory;
    
    bool                    mMemoryCacheEnabled;
    PRInt32                 mMemoryCacheCapacity; 
    PRInt32                 mMemoryCacheMaxEntrySize; 

    bool                    mInPrivateBrowsing;

    PRInt32                 mCacheCompressionLevel;

    bool                    mSanitizeOnShutdown;
    bool                    mClearCacheOnShutdown;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsCacheProfilePrefObserver, nsIObserver)

class nsSetDiskSmartSizeCallback : public nsITimerCallback
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Notify(nsITimer* aTimer) {
        if (nsCacheService::gService) {
            nsCacheServiceAutoLock autoLock;
            nsCacheService::gService->SetDiskSmartSize_Locked();
            nsCacheService::gService->mSmartSizeTimer = nsnull;
        }
        return NS_OK;
    }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSetDiskSmartSizeCallback, nsITimerCallback)



class nsSetSmartSizeEvent: public nsRunnable 
{
public:
    nsSetSmartSizeEvent(PRInt32 smartSize)
        : mSmartSize(smartSize) {}

    NS_IMETHOD Run() 
    {
        NS_ASSERTION(NS_IsMainThread(), 
                     "Setting smart size data off the main thread");

        
        if (!nsCacheService::gService || !nsCacheService::gService->mObserver)
            return NS_ERROR_NOT_AVAILABLE;

        
        
        
        if (!nsCacheService::gService->mObserver->SmartSizeEnabled())
            return NS_OK;

        nsCacheService::SetDiskCacheCapacity(mSmartSize);

        nsCOMPtr<nsIPrefBranch> ps = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (!ps ||
            NS_FAILED(ps->SetIntPref(DISK_CACHE_SMART_SIZE_PREF, mSmartSize)))
            NS_WARNING("Failed to set smart size pref");

        return NS_OK;
    }

private:
    PRInt32 mSmartSize;
};



class nsGetSmartSizeEvent: public nsRunnable
{
public:
    nsGetSmartSizeEvent(const nsAString& cachePath, PRUint32 currentSize)
      : mCachePath(cachePath)
      , mCurrentSize(currentSize)
    {}
   
    
    
    NS_IMETHOD Run()
    {
        PRUint32 size;
        size = nsCacheProfilePrefObserver::GetSmartCacheSize(mCachePath,
                                                             mCurrentSize);
        NS_DispatchToMainThread(new nsSetSmartSizeEvent(size));
        return NS_OK;
    }

private:
    nsString mCachePath;
    PRUint32 mCurrentSize;
};

class nsBlockOnCacheThreadEvent : public nsRunnable {
public:
    nsBlockOnCacheThreadEvent()
    {
    }
    NS_IMETHOD Run()
    {
        nsCacheServiceAutoLock autoLock;
#ifdef PR_LOGGING
        CACHE_LOG_DEBUG(("nsBlockOnCacheThreadEvent [%p]\n", this));
#endif
        nsCacheService::gService->mCondVar.Notify();
        return NS_OK;
    }
};


nsresult
nsCacheProfilePrefObserver::Install()
{
    
    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
    if (!observerService)
        return NS_ERROR_FAILURE;
    
    nsresult rv, rv2 = NS_OK;
    for (unsigned int i=0; i<ArrayLength(observerList); i++) {
        rv = observerService->AddObserver(this, observerList[i], false);
        if (NS_FAILED(rv)) 
            rv2 = rv;
    }
    
    
    nsCOMPtr<nsIPrefBranch> branch = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!branch) return NS_ERROR_FAILURE;

    for (unsigned int i=0; i<ArrayLength(prefList); i++) {
        rv = branch->AddObserver(prefList[i], this, false);
        if (NS_FAILED(rv))
            rv2 = rv;
    }

    
    nsCOMPtr<nsIPrivateBrowsingService> pbs =
      do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbs)
      pbs->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);

    
    
    
    
    
    

    nsCOMPtr<nsIFile> directory;
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(directory));
    if (NS_SUCCEEDED(rv))
        mHaveProfile = true;

    rv = ReadPrefs(branch);
    NS_ENSURE_SUCCESS(rv, rv);

    return rv2;
}


void
nsCacheProfilePrefObserver::Remove()
{
    
    nsCOMPtr<nsIObserverService> obs =
        mozilla::services::GetObserverService();
    if (obs) {
        for (unsigned int i=0; i<ArrayLength(observerList); i++) {
            obs->RemoveObserver(this, observerList[i]);
        }
    }

    
    nsCOMPtr<nsIPrefBranch> prefs =
        do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return;
    for (unsigned int i=0; i<ArrayLength(prefList); i++)
        prefs->RemoveObserver(prefList[i], this); 
}

void
nsCacheProfilePrefObserver::SetDiskCacheCapacity(PRInt32 capacity)
{
    mDiskCacheCapacity = NS_MAX(0, capacity);
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
        
        mHaveProfile = false;

        
        nsCacheService::OnProfileShutdown(!strcmp("shutdown-cleanse",
                                                  data.get()));
        
    } else if (!strcmp("profile-do-change", topic)) {
        
        mHaveProfile = true;
        nsCOMPtr<nsIPrefBranch> branch = do_GetService(NS_PREFSERVICE_CONTRACTID);
        ReadPrefs(branch);
        nsCacheService::OnProfileChanged();
    
    } else if (!strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, topic)) {

        
        if (!mHaveProfile)  
            return NS_OK;

        nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(subject, &rv);
        if (NS_FAILED(rv))  
            return rv;

        
        if (!strcmp(DISK_CACHE_ENABLE_PREF, data.get())) {

            if (!mInPrivateBrowsing) {
                rv = branch->GetBoolPref(DISK_CACHE_ENABLE_PREF,
                                         &mDiskCacheEnabled);
                if (NS_FAILED(rv))  
                    return rv;
                nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());
            }

        } else if (!strcmp(DISK_CACHE_CAPACITY_PREF, data.get())) {

            PRInt32 capacity = 0;
            rv = branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &capacity);
            if (NS_FAILED(rv))  
                return rv;
            mDiskCacheCapacity = NS_MAX(0, capacity);
            nsCacheService::SetDiskCacheCapacity(mDiskCacheCapacity);
       
        
        } else if (!strcmp(DISK_CACHE_SMART_SIZE_ENABLED_PREF, data.get())) {
            
            rv = branch->GetBoolPref(DISK_CACHE_SMART_SIZE_ENABLED_PREF,
                                     &mSmartSizeEnabled);
            if (NS_FAILED(rv)) 
                return rv;
            PRInt32 newCapacity = 0;
            if (mSmartSizeEnabled) {
                nsCacheService::SetDiskSmartSize();
            } else {
                
                rv = branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &newCapacity);
                if (NS_FAILED(rv)) 
                    return rv;
                mDiskCacheCapacity = NS_MAX(0, newCapacity);
                nsCacheService::SetDiskCacheCapacity(mDiskCacheCapacity);
            }
        } else if (!strcmp(DISK_CACHE_MAX_ENTRY_SIZE_PREF, data.get())) {
            PRInt32 newMaxSize;
            rv = branch->GetIntPref(DISK_CACHE_MAX_ENTRY_SIZE_PREF,
                                    &newMaxSize);
            if (NS_FAILED(rv)) 
                return rv;

            mDiskCacheMaxEntrySize = NS_MAX(-1, newMaxSize);
            nsCacheService::SetDiskCacheMaxEntrySize(mDiskCacheMaxEntrySize);
          
#if 0            
        } else if (!strcmp(DISK_CACHE_DIR_PREF, data.get())) {
            
            
            
            
#endif            
        } else

        
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
            mOfflineCacheCapacity = NS_MAX(0, capacity);
            nsCacheService::SetOfflineCacheCapacity(mOfflineCacheCapacity);
#if 0
        } else if (!strcmp(OFFLINE_CACHE_DIR_PREF, data.get())) {
            
            
            
            
#endif
        } else

        if (!strcmp(MEMORY_CACHE_ENABLE_PREF, data.get())) {

            rv = branch->GetBoolPref(MEMORY_CACHE_ENABLE_PREF,
                                     &mMemoryCacheEnabled);
            if (NS_FAILED(rv))  
                return rv;
            nsCacheService::SetMemoryCache();
            
        } else if (!strcmp(MEMORY_CACHE_CAPACITY_PREF, data.get())) {

            mMemoryCacheCapacity = -1;
            (void) branch->GetIntPref(MEMORY_CACHE_CAPACITY_PREF,
                                      &mMemoryCacheCapacity);
            nsCacheService::SetMemoryCache();
        } else if (!strcmp(MEMORY_CACHE_MAX_ENTRY_SIZE_PREF, data.get())) {
            PRInt32 newMaxSize;
            rv = branch->GetIntPref(MEMORY_CACHE_MAX_ENTRY_SIZE_PREF,
                                     &newMaxSize);
            if (NS_FAILED(rv)) 
                return rv;
            
            mMemoryCacheMaxEntrySize = NS_MAX(-1, newMaxSize);
            nsCacheService::SetMemoryCacheMaxEntrySize(mMemoryCacheMaxEntrySize);
        } else if (!strcmp(CACHE_COMPRESSION_LEVEL_PREF, data.get())) {
            mCacheCompressionLevel = CACHE_COMPRESSION_LEVEL;
            (void)branch->GetIntPref(CACHE_COMPRESSION_LEVEL_PREF,
                                     &mCacheCompressionLevel);
            mCacheCompressionLevel = NS_MAX(0, mCacheCompressionLevel);
            mCacheCompressionLevel = NS_MIN(9, mCacheCompressionLevel);
        } else if (!strcmp(SANITIZE_ON_SHUTDOWN_PREF, data.get())) {
            rv = branch->GetBoolPref(SANITIZE_ON_SHUTDOWN_PREF,
                                     &mSanitizeOnShutdown);
            if (NS_FAILED(rv))
                return rv;
            nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());
        } else if (!strcmp(CLEAR_ON_SHUTDOWN_PREF, data.get())) {
            rv = branch->GetBoolPref(CLEAR_ON_SHUTDOWN_PREF,
                                     &mClearCacheOnShutdown);
            if (NS_FAILED(rv))
                return rv;
            nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());
        }
    } else if (!strcmp(NS_PRIVATE_BROWSING_SWITCH_TOPIC, topic)) {
        if (!strcmp(NS_PRIVATE_BROWSING_ENTER, data.get())) {
            mInPrivateBrowsing = true;

            nsCacheService::OnEnterExitPrivateBrowsing();

            mDiskCacheEnabled = false;
            nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());

            mOfflineCacheEnabled = false;
            nsCacheService::SetOfflineCacheEnabled(OfflineCacheEnabled());
        } else if (!strcmp(NS_PRIVATE_BROWSING_LEAVE, data.get())) {
            mInPrivateBrowsing = false;

            nsCacheService::OnEnterExitPrivateBrowsing();

            nsCOMPtr<nsIPrefBranch> branch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
            if (NS_FAILED(rv))  
                return rv;

            mDiskCacheEnabled = true; 
            (void) branch->GetBoolPref(DISK_CACHE_ENABLE_PREF,
                                       &mDiskCacheEnabled);
            nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());

            mOfflineCacheEnabled = true; 
            (void) branch->GetBoolPref(OFFLINE_CACHE_ENABLE_PREF,
                                       &mOfflineCacheEnabled);
            nsCacheService::SetOfflineCacheEnabled(OfflineCacheEnabled());
        }
    }
    
    return NS_OK;
}



static PRUint32
SmartCacheSize(const PRUint32 availKB)
{
    if (availKB > 100 * 1024 * 1024)
        return MAX_CACHE_SIZE;  

    
    
    
    PRUint32 sz10MBs = 0;
    PRUint32 avail10MBs = availKB / (1024*10);

    
    if (avail10MBs > 2500) {
        sz10MBs += (avail10MBs - 2500)*.005;
        avail10MBs = 2500;
    }
    
    if (avail10MBs > 700) {
        sz10MBs += (avail10MBs - 700)*.01;
        avail10MBs = 700;
    }
    
    if (avail10MBs > 50) {
        sz10MBs += (avail10MBs - 50)*.05;
        avail10MBs = 50;
    }

    
    sz10MBs += NS_MAX<PRUint32>(5, avail10MBs * .4);

    return NS_MIN<PRUint32>(MAX_CACHE_SIZE, sz10MBs * 10 * 1024);
}

 










PRUint32
nsCacheProfilePrefObserver::GetSmartCacheSize(const nsAString& cachePath,
                                              PRUint32 currentSize)
{
    
    nsresult rv;
    nsCOMPtr<nsILocalFile> 
        cacheDirectory (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
    if (NS_FAILED(rv) || !cacheDirectory)
        return DEFAULT_CACHE_SIZE;
    rv = cacheDirectory->InitWithPath(cachePath);
    if (NS_FAILED(rv))
        return DEFAULT_CACHE_SIZE;
    PRInt64 bytesAvailable;
    rv = cacheDirectory->GetDiskSpaceAvailable(&bytesAvailable);
    if (NS_FAILED(rv))
        return DEFAULT_CACHE_SIZE;

    return SmartCacheSize((bytesAvailable / 1024) + currentSize);
}





bool
nsCacheProfilePrefObserver::PermittedToSmartSize(nsIPrefBranch* branch, bool
                                                 firstRun)
{
    nsresult rv;
    if (firstRun) {
        
        bool userSet;
        rv = branch->PrefHasUserValue(DISK_CACHE_CAPACITY_PREF, &userSet);
        if (NS_FAILED(rv)) userSet = true;
        if (userSet) {
            PRInt32 oldCapacity;
            
            
            rv = branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &oldCapacity);
            if (oldCapacity < PRE_GECKO_2_0_DEFAULT_CACHE_SIZE) {
                mSmartSizeEnabled = false;
                branch->SetBoolPref(DISK_CACHE_SMART_SIZE_ENABLED_PREF,
                                    mSmartSizeEnabled);
                return mSmartSizeEnabled;
            }
        }
        
        
        branch->SetIntPref(DISK_CACHE_CAPACITY_PREF, MAX_CACHE_SIZE);
    }

    rv = branch->GetBoolPref(DISK_CACHE_SMART_SIZE_ENABLED_PREF,
                             &mSmartSizeEnabled);
    if (NS_FAILED(rv))
        mSmartSizeEnabled = false;
    return mSmartSizeEnabled;
}


nsresult
nsCacheProfilePrefObserver::ReadPrefs(nsIPrefBranch* branch)
{
    nsresult rv = NS_OK;

    
    if (!mInPrivateBrowsing) {
        mDiskCacheEnabled = true;  
        (void) branch->GetBoolPref(DISK_CACHE_ENABLE_PREF, &mDiskCacheEnabled);
    }

    mDiskCacheCapacity = DISK_CACHE_CAPACITY;
    (void)branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &mDiskCacheCapacity);
    mDiskCacheCapacity = NS_MAX(0, mDiskCacheCapacity);

    (void) branch->GetIntPref(DISK_CACHE_MAX_ENTRY_SIZE_PREF,
                              &mDiskCacheMaxEntrySize);
    mDiskCacheMaxEntrySize = NS_MAX(-1, mDiskCacheMaxEntrySize);
    
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
                bool same;
                if (NS_SUCCEEDED(profDir->Equals(directory, &same)) && !same) {
                    
                    
                    rv = profDir->AppendNative(NS_LITERAL_CSTRING("Cache"));
                    if (NS_SUCCEEDED(rv)) {
                        bool exists;
                        if (NS_SUCCEEDED(profDir->Exists(&exists)) && exists)
                            nsDeleteDir::DeleteDir(profDir, false);
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
    if (mDiskCacheParentDirectory) {
        bool firstSmartSizeRun;
        rv = branch->GetBoolPref(DISK_CACHE_SMART_SIZE_FIRST_RUN_PREF, 
                                 &firstSmartSizeRun); 
        if (NS_FAILED(rv)) 
            firstSmartSizeRun = false;
        if (PermittedToSmartSize(branch, firstSmartSizeRun)) {
            
            
            rv = branch->GetIntPref(firstSmartSizeRun ?
                                    DISK_CACHE_CAPACITY_PREF :
                                    DISK_CACHE_SMART_SIZE_PREF,
                                    &mDiskCacheCapacity);
            if (NS_FAILED(rv))
                mDiskCacheCapacity = DEFAULT_CACHE_SIZE;
        }

        if (firstSmartSizeRun) {
            
            rv = branch->SetBoolPref(DISK_CACHE_SMART_SIZE_FIRST_RUN_PREF, 
                                     false);
            if (NS_FAILED(rv)) 
                NS_WARNING("Failed setting first_run pref in ReadPrefs.");
        }
    }

    
    if (!mInPrivateBrowsing) {
        mOfflineCacheEnabled = true;  
        (void) branch->GetBoolPref(OFFLINE_CACHE_ENABLE_PREF,
                                   &mOfflineCacheEnabled);
    }

    mOfflineCacheCapacity = OFFLINE_CACHE_CAPACITY;
    (void)branch->GetIntPref(OFFLINE_CACHE_CAPACITY_PREF,
                             &mOfflineCacheCapacity);
    mOfflineCacheCapacity = NS_MAX(0, mOfflineCacheCapacity);

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

    
    (void) branch->GetBoolPref(MEMORY_CACHE_ENABLE_PREF, &mMemoryCacheEnabled);

    mMemoryCacheCapacity = -1;
    (void) branch->GetIntPref(MEMORY_CACHE_CAPACITY_PREF,
                              &mMemoryCacheCapacity);

    (void) branch->GetIntPref(MEMORY_CACHE_MAX_ENTRY_SIZE_PREF,
                              &mMemoryCacheMaxEntrySize);
    mMemoryCacheMaxEntrySize = NS_MAX(-1, mMemoryCacheMaxEntrySize);

    
    mCacheCompressionLevel = CACHE_COMPRESSION_LEVEL;
    (void)branch->GetIntPref(CACHE_COMPRESSION_LEVEL_PREF,
                             &mCacheCompressionLevel);
    mCacheCompressionLevel = NS_MAX(0, mCacheCompressionLevel);
    mCacheCompressionLevel = NS_MIN(9, mCacheCompressionLevel);

    
    (void) branch->GetBoolPref(SANITIZE_ON_SHUTDOWN_PREF,
                               &mSanitizeOnShutdown);
    (void) branch->GetBoolPref(CLEAR_ON_SHUTDOWN_PREF,
                               &mClearCacheOnShutdown);

    return rv;
}

nsresult
nsCacheService::DispatchToCacheIOThread(nsIRunnable* event)
{
    if (!gService->mCacheIOThread) return NS_ERROR_NOT_AVAILABLE;
    return gService->mCacheIOThread->Dispatch(event, NS_DISPATCH_NORMAL);
}

nsresult
nsCacheService::SyncWithCacheIOThread()
{
    gService->mLock.AssertCurrentThreadOwns();
    if (!gService->mCacheIOThread) return NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<nsIRunnable> event = new nsBlockOnCacheThreadEvent();

    
    nsresult rv =
        gService->mCacheIOThread->Dispatch(event, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
        NS_WARNING("Failed dispatching block-event");
        return NS_ERROR_UNEXPECTED;
    }

    
    rv = gService->mCondVar.Wait();

    return rv;
}


bool
nsCacheProfilePrefObserver::DiskCacheEnabled()
{
    if ((mDiskCacheCapacity == 0) || (!mDiskCacheParentDirectory))  return false;
    return mDiskCacheEnabled && (!mSanitizeOnShutdown || !mClearCacheOnShutdown);
}


bool
nsCacheProfilePrefObserver::OfflineCacheEnabled()
{
    if ((mOfflineCacheCapacity == 0) || (!mOfflineCacheParentDirectory))
        return false;

    return mOfflineCacheEnabled;
}


bool
nsCacheProfilePrefObserver::MemoryCacheEnabled()
{
    if (mMemoryCacheCapacity == 0)  return false;
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

    static PRUint64 bytes = PR_GetPhysicalMemorySize();
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

PRInt32
nsCacheProfilePrefObserver::CacheCompressionLevel()
{
    return mCacheCompressionLevel;
}





class nsProcessRequestEvent : public nsRunnable {
public:
    nsProcessRequestEvent(nsCacheRequest *aRequest)
    {
        mRequest = aRequest;
    }

    NS_IMETHOD Run()
    {
        nsresult rv;

        NS_ASSERTION(mRequest->mListener,
                     "Sync OpenCacheEntry() posted to background thread!");

        nsCacheServiceAutoLock lock;
        rv = nsCacheService::gService->ProcessRequest(mRequest,
                                                      false,
                                                      nsnull);

        
        if (!(mRequest->IsBlocking() &&
            rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION))
            delete mRequest;

        return NS_OK;
    }

protected:
    virtual ~nsProcessRequestEvent() {}

private:
    nsCacheRequest *mRequest;
};





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





class nsDoomEvent : public nsRunnable {
public:
    nsDoomEvent(nsCacheSession *session,
                const nsACString &key,
                nsICacheListener *listener)
    {
        mKey = *session->ClientID();
        mKey.Append(':');
        mKey.Append(key);
        mStoragePolicy = session->StoragePolicy();
        mListener = listener;
        mThread = do_GetCurrentThread();
        
        
        
        
        NS_IF_ADDREF(mListener);
    }

    NS_IMETHOD Run()
    {
        nsCacheServiceAutoLock lock;

        bool foundActive = true;
        nsresult status = NS_ERROR_NOT_AVAILABLE;
        nsCacheEntry *entry;
        entry = nsCacheService::gService->mActiveEntries.GetEntry(&mKey);
        if (!entry) {
            bool collision = false;
            foundActive = false;
            entry = nsCacheService::gService->SearchCacheDevices(&mKey,
                                                                 mStoragePolicy,
                                                                 &collision);
        }

        if (entry) {
            status = NS_OK;
            nsCacheService::gService->DoomEntry_Internal(entry, foundActive);
        }

        if (mListener) {
            mThread->Dispatch(new nsNotifyDoomListener(mListener, status),
                              NS_DISPATCH_NORMAL);
            
            mListener = nsnull;
        }

        return NS_OK;
    }

private:
    nsCString             mKey;
    nsCacheStoragePolicy  mStoragePolicy;
    nsICacheListener     *mListener;
    nsCOMPtr<nsIThread>   mThread;
};




nsCacheService *   nsCacheService::gService = nsnull;

static nsCOMPtr<nsIMemoryReporter> MemoryCacheReporter = nsnull;

NS_THREADSAFE_MEMORY_REPORTER_IMPLEMENT(NetworkMemoryCache,
    "explicit/network-memory-cache",
    KIND_HEAP,
    UNITS_BYTES,
    nsCacheService::MemoryDeviceSize,
    "Memory used by the network memory cache.")

NS_IMPL_THREADSAFE_ISUPPORTS1(nsCacheService, nsICacheService)

nsCacheService::nsCacheService()
    : mLock("nsCacheService.mLock"),
      mCondVar(mLock, "nsCacheService.mCondVar"),
      mInitialized(false),
      mEnableMemoryDevice(true),
      mEnableDiskDevice(true),
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
}

nsCacheService::~nsCacheService()
{
    if (mInitialized) 
        (void) Shutdown();

    gService = nsnull;
}


nsresult
nsCacheService::Init()
{
    NS_TIME_FUNCTION;

    NS_ASSERTION(!mInitialized, "nsCacheService already initialized.");
    if (mInitialized)
        return NS_ERROR_ALREADY_INITIALIZED;

    if (mozilla::net::IsNeckoChild()) {
        return NS_ERROR_UNEXPECTED;
    }

    CACHE_LOG_INIT();

    nsresult rv = NS_NewThread(getter_AddRefs(mCacheIOThread));
    if (NS_FAILED(rv)) {
        NS_WARNING("Can't create cache IO thread");
    }

    rv = nsDeleteDir::Init();
    if (NS_FAILED(rv)) {
        NS_WARNING("Can't initialize nsDeleteDir");
    }

    
    rv = mActiveEntries.Init();
    if (NS_FAILED(rv)) return rv;
    
    
    mObserver = new nsCacheProfilePrefObserver();
    if (!mObserver)  return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mObserver);
    
    mObserver->Install();
    mEnableDiskDevice    = mObserver->DiskCacheEnabled();
    mEnableOfflineDevice = mObserver->OfflineCacheEnabled();
    mEnableMemoryDevice  = mObserver->MemoryCacheEnabled();

    mInitialized = true;
    return NS_OK;
}


void
nsCacheService::Shutdown()
{
    nsCOMPtr<nsIThread> cacheIOThread;
    Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_SHUTDOWN> totalTimer;

    bool shouldSanitize = false;
    nsCOMPtr<nsILocalFile> parentDir;

    {
    nsCacheServiceAutoLock lock;
    NS_ASSERTION(mInitialized, 
                 "can't shutdown nsCacheService unless it has been initialized.");

    if (mInitialized) {

        mInitialized = false;

        
        ClearDoomList();
        ClearActiveEntries();

        if (mSmartSizeTimer) {
            mSmartSizeTimer->Cancel();
            mSmartSizeTimer = nsnull;
        }

        
        
        (void) SyncWithCacheIOThread();

        
        parentDir = mObserver->DiskCacheParentDirectory();
        shouldSanitize = mObserver->SanitizeAtShutdown();
        mObserver->Remove();
        NS_RELEASE(mObserver);
        
        
        
        NS_UnregisterMemoryReporter(MemoryCacheReporter);
        MemoryCacheReporter = nsnull;

        
        delete mMemoryDevice;
        mMemoryDevice = nsnull;

        delete mDiskDevice;
        mDiskDevice = nsnull;

        if (mOfflineDevice)
            mOfflineDevice->Shutdown();

        NS_IF_RELEASE(mOfflineDevice);

#ifdef PR_LOGGING
        LogCacheStatistics();
#endif

        mCacheIOThread.swap(cacheIOThread);
    }
    } 

    if (cacheIOThread)
        cacheIOThread->Shutdown();

    if (shouldSanitize) {
        nsresult rv = parentDir->AppendNative(NS_LITERAL_CSTRING("Cache"));
        if (NS_SUCCEEDED(rv)) {
            bool exists;
            if (NS_SUCCEEDED(parentDir->Exists(&exists)) && exists)
                nsDeleteDir::DeleteDir(parentDir, false);
        }
        Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_SHUTDOWN_CLEAR_PRIVATE> timer;
        nsDeleteDir::Shutdown(shouldSanitize);
    } else {
        Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_DELETEDIR_SHUTDOWN> timer;
        nsDeleteDir::Shutdown(shouldSanitize);
    }
}


nsresult
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
                              bool                  streamBased,
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

namespace {

class EvictionNotifierRunnable : public nsRunnable
{
public:
    EvictionNotifierRunnable(nsISupports* aSubject)
        : mSubject(aSubject)
    { }

    NS_DECL_NSIRUNNABLE

private:
    nsCOMPtr<nsISupports> mSubject;
};

NS_IMETHODIMP
EvictionNotifierRunnable::Run()
{
    nsCOMPtr<nsIObserverService> obsSvc =
        mozilla::services::GetObserverService();
    if (obsSvc) {
        obsSvc->NotifyObservers(mSubject,
                                NS_CACHESERVICE_EMPTYCACHE_TOPIC_ID,
                                nsnull);
    }
    return NS_OK;
}

} 

nsresult
nsCacheService::EvictEntriesForClient(const char *          clientID,
                                      nsCacheStoragePolicy  storagePolicy)
{
    nsRefPtr<EvictionNotifierRunnable> r = new EvictionNotifierRunnable(this);
    NS_DispatchToMainThread(r);

    nsCacheServiceAutoLock lock;
    nsresult res = NS_OK;

    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_ON_DISK) {

        if (mEnableDiskDevice) {
            nsresult rv;
            if (!mDiskDevice)
                rv = CreateDiskDevice();
            if (mDiskDevice)
                rv = mDiskDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) res = rv;
        }
    }

    
    if (storagePolicy == nsICache::STORE_OFFLINE) {
        if (mEnableOfflineDevice) {
            nsresult rv;
            if (!mOfflineDevice)
                rv = CreateOfflineDevice();
            if (mOfflineDevice)
                rv = mOfflineDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) res = rv;
        }
    }

    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_IN_MEMORY) {

        
        if (mMemoryDevice) {
            nsresult rv;
            rv = mMemoryDevice->EvictEntries(clientID);
            if (NS_FAILED(rv)) res = rv;
        }
    }

    return res;
}


nsresult        
nsCacheService::IsStorageEnabledForPolicy(nsCacheStoragePolicy  storagePolicy,
                                          bool *              result)
{
    if (gService == nsnull) return NS_ERROR_NOT_AVAILABLE;
    nsCacheServiceAutoLock lock;

    *result = gService->IsStorageEnabledForPolicy_Locked(storagePolicy);
    return NS_OK;
}


nsresult
nsCacheService::DoomEntry(nsCacheSession   *session,
                          const nsACString &key,
                          nsICacheListener *listener)
{
    CACHE_LOG_DEBUG(("Dooming entry for session %p, key %s\n",
                     session, PromiseFlatCString(key).get()));
    NS_ASSERTION(gService, "nsCacheService::gService is null.");

    if (!gService->mInitialized)
        return NS_ERROR_NOT_INITIALIZED;

    return DispatchToCacheIOThread(new nsDoomEvent(session, key, listener));
}


bool          
nsCacheService::IsStorageEnabledForPolicy_Locked(nsCacheStoragePolicy  storagePolicy)
{
    if (gService->mEnableMemoryDevice &&
        (storagePolicy == nsICache::STORE_ANYWHERE ||
         storagePolicy == nsICache::STORE_IN_MEMORY)) {
        return true;
    }
    if (gService->mEnableDiskDevice &&
        (storagePolicy == nsICache::STORE_ANYWHERE ||
         storagePolicy == nsICache::STORE_ON_DISK  ||
         storagePolicy == nsICache::STORE_ON_DISK_AS_FILE)) {
        return true;
    }
    if (gService->mEnableOfflineDevice &&
        storagePolicy == nsICache::STORE_OFFLINE) {
        return true;
    }
    
    return false;
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

    if (mEnableDiskDevice) {
        if (!mDiskDevice) {
            rv = CreateDiskDevice();
            if (NS_FAILED(rv)) return rv;
        }
        rv = mDiskDevice->Visit(visitor);
        if (NS_FAILED(rv)) return rv;
    }

    if (mEnableOfflineDevice) {
        if (!mOfflineDevice) {
            rv = CreateOfflineDevice();
            if (NS_FAILED(rv)) return rv;
        }
        rv = mOfflineDevice->Visit(visitor);
        if (NS_FAILED(rv)) return rv;
    }

    
    

    return NS_OK;
}


NS_IMETHODIMP nsCacheService::EvictEntries(nsCacheStoragePolicy storagePolicy)
{
    return  EvictEntriesForClient(nsnull, storagePolicy);
}

NS_IMETHODIMP nsCacheService::GetCacheIOTarget(nsIEventTarget * *aCacheIOTarget)
{
    nsCacheServiceAutoLock lock;

    if (!mCacheIOThread)
        return NS_ERROR_NOT_AVAILABLE;

    NS_ADDREF(*aCacheIOTarget = mCacheIOThread);
    return NS_OK;
}




nsresult
nsCacheService::CreateDiskDevice()
{
    if (!mInitialized)      return NS_ERROR_NOT_AVAILABLE;
    if (!mEnableDiskDevice) return NS_ERROR_NOT_AVAILABLE;
    if (mDiskDevice)        return NS_OK;

    mDiskDevice = new nsDiskCacheDevice;
    if (!mDiskDevice)       return NS_ERROR_OUT_OF_MEMORY;

    
    mDiskDevice->SetCacheParentDirectory(mObserver->DiskCacheParentDirectory());
    mDiskDevice->SetCapacity(mObserver->DiskCacheCapacity());
    mDiskDevice->SetMaxEntrySize(mObserver->DiskCacheMaxEntrySize());
    
    nsresult rv = mDiskDevice->Init();
    if (NS_FAILED(rv)) {
#if DEBUG
        printf("###\n");
        printf("### mDiskDevice->Init() failed (0x%.8x)\n", rv);
        printf("###    - disabling disk cache for this session.\n");
        printf("###\n");
#endif        
        mEnableDiskDevice = false;
        delete mDiskDevice;
        mDiskDevice = nsnull;
        return rv;
    }

    NS_ASSERTION(!mSmartSizeTimer, "Smartsize timer was already fired!");

    
    
    
    mSmartSizeTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = mSmartSizeTimer->InitWithCallback(new nsSetDiskSmartSizeCallback(),
                                               1000*60*3,
                                               nsITimer::TYPE_ONE_SHOT);
        if (NS_FAILED(rv)) {
            NS_WARNING("Failed to post smart size timer");
            mSmartSizeTimer = nsnull;
        }
    } else {
        NS_WARNING("Can't create smart size timer");
    }
    
    

    return NS_OK;
}

nsresult
nsCacheService::CreateOfflineDevice()
{
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

        mEnableOfflineDevice = false;
        NS_RELEASE(mOfflineDevice);
    }
    return rv;
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
    mMemoryDevice->SetMaxEntrySize(mObserver->MemoryCacheMaxEntrySize());

    nsresult rv = mMemoryDevice->Init();
    if (NS_FAILED(rv)) {
        NS_WARNING("Initialization of Memory Cache failed.");
        delete mMemoryDevice;
        mMemoryDevice = nsnull;
    }

    MemoryCacheReporter =
        new NS_MEMORY_REPORTER_NAME(NetworkMemoryCache);
    NS_RegisterMemoryReporter(MemoryCacheReporter);

    return rv;
}


nsresult
nsCacheService::CreateRequest(nsCacheSession *   session,
                              const nsACString & clientKey,
                              nsCacheAccessMode  accessRequested,
                              bool               blockingMode,
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
                               bool                       calledFromOpenCacheEntry,
                               nsICacheEntryDescriptor ** result)
{
    
    nsresult           rv;
    nsCacheEntry *     entry = nsnull;
    nsCacheEntry *     doomedEntry = nsnull;
    nsCacheAccessMode  accessGranted = nsICache::ACCESS_NONE;
    if (result) *result = nsnull;

    while(1) {  
        rv = ActivateEntry(request, &entry, &doomedEntry);  
        if (NS_FAILED(rv))  break;

        while(1) { 
            NS_ASSERTION(entry, "no entry in Request Access loop!");
            
            rv = entry->RequestAccess(request, &accessGranted);
            if (rv != NS_ERROR_CACHE_WAIT_FOR_VALIDATION) break;

            if (request->IsBlocking()) {
                if (request->mListener) {
                    
                    return rv;
                }

                
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

    
    
    
    
    
    
    
    
    
    
    
    if (doomedEntry) {
        (void) ProcessPendingRequests(doomedEntry);
        if (doomedEntry->IsNotInUse())
            DeactivateEntry(doomedEntry);
        doomedEntry = nsnull;
    }

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
                               bool                       blockingMode,
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

    nsresult rv = gService->CreateRequest(session,
                                          key,
                                          accessRequested,
                                          blockingMode,
                                          listener,
                                          &request);
    if (NS_FAILED(rv))  return rv;

    CACHE_LOG_DEBUG(("Created request %p\n", request));

    
    
    if (NS_IsMainThread() && listener && gService->mCacheIOThread) {
        nsCOMPtr<nsIRunnable> ev =
            new nsProcessRequestEvent(request);
        rv = DispatchToCacheIOThread(ev);

        
        if (NS_FAILED(rv))
            delete request;
    }
    else {

        nsCacheServiceAutoLock lock;
        rv = gService->ProcessRequest(request, true, result);

        
        if (!(listener && blockingMode &&
            (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION)))
            delete request;
    }

    return rv;
}


nsresult
nsCacheService::ActivateEntry(nsCacheRequest * request, 
                              nsCacheEntry ** result,
                              nsCacheEntry ** doomedEntry)
{
    CACHE_LOG_DEBUG(("Activate entry for request %p\n", request));
    
    nsresult        rv = NS_OK;

    NS_ASSERTION(request != nsnull, "ActivateEntry called with no request");
    if (result) *result = nsnull;
    if (doomedEntry) *doomedEntry = nsnull;
    if ((!request) || (!result) || (!doomedEntry))
        return NS_ERROR_NULL_POINTER;

    
    if (!mEnableMemoryDevice && !request->IsStreamBased())
        return NS_ERROR_FAILURE;
    if (!IsStorageEnabledForPolicy_Locked(request->StoragePolicy()))
        return NS_ERROR_FAILURE;

    
    nsCacheEntry *entry = mActiveEntries.GetEntry(request->mKey);
    CACHE_LOG_DEBUG(("Active entry for request %p is %p\n", request, entry));

    if (!entry) {
        
        bool collision = false;
        entry = SearchCacheDevices(request->mKey, request->StoragePolicy(), &collision);
        CACHE_LOG_DEBUG(("Device search for request %p returned %p\n",
                         request, entry));
        
        if (collision) return NS_ERROR_CACHE_IN_USE;

        if (entry)  entry->MarkInitialized();
    } else {
        NS_ASSERTION(entry->IsActive(), "Inactive entry found in mActiveEntries!");
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
        
        
        
        rv = DoomEntry_Internal(entry, false);
        *doomedEntry = entry;
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
    delete entry;
    return rv;
}


nsCacheEntry *
nsCacheService::SearchCacheDevices(nsCString * key, nsCacheStoragePolicy policy, bool *collision)
{
    Telemetry::AutoTimer<Telemetry::CACHE_DEVICE_SEARCH> timer;
    nsCacheEntry * entry = nsnull;

    CACHE_LOG_DEBUG(("mMemoryDevice: 0x%p\n", mMemoryDevice));

    *collision = false;
    if ((policy == nsICache::STORE_ANYWHERE) || (policy == nsICache::STORE_IN_MEMORY)) {
        
        if (mMemoryDevice) {
            entry = mMemoryDevice->FindEntry(key, collision);
            CACHE_LOG_DEBUG(("Searching mMemoryDevice for key %s found: 0x%p, "
                             "collision: %d\n", key->get(), entry, collision));
        }
    }

    if (!entry && 
        ((policy == nsICache::STORE_ANYWHERE) || (policy == nsICache::STORE_ON_DISK))) {

        if (mEnableDiskDevice) {
            if (!mDiskDevice) {
                nsresult rv = CreateDiskDevice();
                if (NS_FAILED(rv))
                    return nsnull;
            }
            
            entry = mDiskDevice->FindEntry(key, collision);
        }
    }

    if (!entry && (policy == nsICache::STORE_OFFLINE ||
                   (policy == nsICache::STORE_ANYWHERE &&
                    gIOService->IsOffline()))) {

        if (mEnableOfflineDevice) {
            if (!mOfflineDevice) {
                nsresult rv = CreateOfflineDevice();
                if (NS_FAILED(rv))
                    return nsnull;
            }

            entry = mOfflineDevice->FindEntry(key, collision);
        }
    }

    return entry;
}


nsCacheDevice *
nsCacheService::EnsureEntryHasDevice(nsCacheEntry * entry)
{
    nsCacheDevice * device = entry->CacheDevice();
    
    
    if (device || entry->IsDoomed())  return device;

    PRInt64 predictedDataSize = entry->PredictedDataSize();
    if (entry->IsStreamData() && entry->IsAllowedOnDisk() && mEnableDiskDevice) {
        
        if (!mDiskDevice) {
            (void)CreateDiskDevice();  
        }

        if (mDiskDevice) {
            
            if (predictedDataSize != -1 &&
                entry->StoragePolicy() != nsICache::STORE_ON_DISK_AS_FILE &&
                mDiskDevice->EntryIsTooBig(predictedDataSize)) {
                DebugOnly<nsresult> rv = nsCacheService::DoomEntry(entry);
                NS_ASSERTION(NS_SUCCEEDED(rv),"DoomEntry() failed.");
                return nsnull;
            }

            entry->MarkBinding();  
            nsresult rv = mDiskDevice->BindEntry(entry);
            entry->ClearBinding(); 
            if (NS_SUCCEEDED(rv))
                device = mDiskDevice;
        }
    }

    
    if (!device && mEnableMemoryDevice && entry->IsAllowedInMemory()) {        
        if (!mMemoryDevice) {
            (void)CreateMemoryDevice();  
        }
        if (mMemoryDevice) {
            
            if (predictedDataSize != -1 &&
                mMemoryDevice->EntryIsTooBig(predictedDataSize)) {
                DebugOnly<nsresult> rv = nsCacheService::DoomEntry(entry);
                NS_ASSERTION(NS_SUCCEEDED(rv),"DoomEntry() failed.");
                return nsnull;
            }

            entry->MarkBinding();  
            nsresult rv = mMemoryDevice->BindEntry(entry);
            entry->ClearBinding(); 
            if (NS_SUCCEEDED(rv))
                device = mMemoryDevice;
        }
    }

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

    if (device) 
        entry->SetCacheDevice(device);
    return device;
}

PRInt64
nsCacheService::MemoryDeviceSize()
{
    nsMemoryCacheDevice *memoryDevice = GlobalInstance()->mMemoryDevice;
    return memoryDevice ? memoryDevice->TotalSize() : 0;
}

nsresult
nsCacheService::DoomEntry(nsCacheEntry * entry)
{
    return gService->DoomEntry_Internal(entry, true);
}


nsresult
nsCacheService::DoomEntry_Internal(nsCacheEntry * entry,
                                   bool doProcessPendingRequests)
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

    
    if (doProcessPendingRequests) {
        
        rv = ProcessPendingRequests(entry);

        
        if (entry->IsNotInUse()) {
            DeactivateEntry(entry); 
        }
    }
    return rv;
}


void
nsCacheService::OnProfileShutdown(bool cleanse)
{
    if (!gService)  return;
    if (!gService->mInitialized) {
        
        
        return;
    }
    nsCacheServiceAutoLock lock;

    gService->DoomActiveEntries();
    gService->ClearDoomList();

    
    
    (void) SyncWithCacheIOThread();

    if (gService->mDiskDevice && gService->mEnableDiskDevice) {
        if (cleanse)
            gService->mDiskDevice->EvictEntries(nsnull);

        gService->mDiskDevice->Shutdown();
    }
    gService->mEnableDiskDevice = false;

    if (gService->mOfflineDevice && gService->mEnableOfflineDevice) {
        if (cleanse)
            gService->mOfflineDevice->EvictEntries(nsnull);

        gService->mOfflineDevice->Shutdown();
    }
    gService->mEnableOfflineDevice = false;

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

    if (gService->mDiskDevice) {
        gService->mDiskDevice->SetCacheParentDirectory(gService->mObserver->DiskCacheParentDirectory());
        gService->mDiskDevice->SetCapacity(gService->mObserver->DiskCacheCapacity());

        
        nsresult rv = gService->mDiskDevice->Init();
        if (NS_FAILED(rv)) {
            NS_ERROR("nsCacheService::OnProfileChanged: Re-initializing disk device failed");
            gService->mEnableDiskDevice = false;
            
        }
    }

    if (gService->mOfflineDevice) {
        gService->mOfflineDevice->SetCacheParentDirectory(gService->mObserver->OfflineCacheParentDirectory());
        gService->mOfflineDevice->SetCapacity(gService->mObserver->OfflineCacheCapacity());

        
        nsresult rv = gService->mOfflineDevice->Init();
        if (NS_FAILED(rv)) {
            NS_ERROR("nsCacheService::OnProfileChanged: Re-initializing offline device failed");
            gService->mEnableOfflineDevice = false;
            
        }
    }

    
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
nsCacheService::SetDiskCacheEnabled(bool    enabled)
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

    if (gService->mDiskDevice) {
        gService->mDiskDevice->SetCapacity(capacity);
    }

    if (gService->mObserver)
        gService->mEnableDiskDevice = gService->mObserver->DiskCacheEnabled();
}

void
nsCacheService::SetDiskCacheMaxEntrySize(PRInt32  maxSize)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;

    if (gService->mDiskDevice) {
        gService->mDiskDevice->SetMaxEntrySize(maxSize);
    }
}

void
nsCacheService::SetMemoryCacheMaxEntrySize(PRInt32  maxSize)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock;

    if (gService->mMemoryDevice) {
        gService->mMemoryDevice->SetMaxEntrySize(maxSize);
    }
}

void
nsCacheService::SetOfflineCacheEnabled(bool    enabled)
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

    if (gService->mOfflineDevice) {
        gService->mOfflineDevice->SetCapacity(capacity);
    }

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





void
nsCacheService::CloseDescriptor(nsCacheEntryDescriptor * descriptor)
{
    
    nsCacheEntry * entry       = descriptor->CacheEntry();
    bool           stillActive = entry->RemoveDescriptor(descriptor);
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
    if (NS_IsMainThread()) {
        Telemetry::AutoTimer<Telemetry::CACHE_SERVICE_LOCK_WAIT_MAINTHREAD> timer;
        gService->mLock.Lock();
    } else {
        Telemetry::AutoTimer<Telemetry::CACHE_SERVICE_LOCK_WAIT> timer;
        gService->mLock.Lock();
    }
}

void
nsCacheService::Unlock()
{
    gService->mLock.AssertCurrentThreadOwns();

    nsTArray<nsISupports*> doomed;
    doomed.SwapElements(gService->mDoomedObjects);

    gService->mLock.Unlock();

    for (PRUint32 i = 0; i < doomed.Length(); ++i)
        doomed[i]->Release();
}

void
nsCacheService::ReleaseObject_Locked(nsISupports * obj,
                                     nsIEventTarget * target)
{
    gService->mLock.AssertCurrentThreadOwns();

    bool isCur;
    if (!target || (NS_SUCCEEDED(target->IsOnCurrentThread(&isCur)) && isCur)) {
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


PRInt32
nsCacheService::CacheCompressionLevel()
{
    PRInt32 level = gService->mObserver->CacheCompressionLevel();
    return level;
}


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
    bool                newWriter = false;
    
    CACHE_LOG_DEBUG(("ProcessPendingRequests for %sinitialized %s %salid entry %p\n",
                    (entry->IsInitialized()?"" : "Un"),
                    (entry->IsDoomed()?"DOOMED" : ""),
                    (entry->IsValid()? "V":"Inv"), entry));

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
                newWriter = true;
                CACHE_LOG_DEBUG(("  promoting request %p to 1st writer\n", request));
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
        CACHE_LOG_DEBUG(("  %sync request %p for %p\n",
                        (request->mListener?"As":"S"), request, entry));

        if (request->mListener) {

            
            PR_REMOVE_AND_INIT_LINK(request);

            if (entry->IsDoomed()) {
                rv = ProcessRequest(request, false, nsnull);
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
                
                
                
                nsCOMPtr<nsIRunnable> ev =
                    new nsProcessRequestEvent(request);
                rv = DispatchToCacheIOThread(ev);
                if (NS_FAILED(rv)) {
                    delete request; 
                }
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
        DoomEntry_Internal(array[i], true);
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

nsresult
nsCacheService::SetDiskSmartSize()
{
    nsCacheServiceAutoLock lock;

    if (!gService) return NS_ERROR_NOT_AVAILABLE;

    return gService->SetDiskSmartSize_Locked();
}

nsresult
nsCacheService::SetDiskSmartSize_Locked()
{
    nsresult rv;

    if (!mObserver->DiskCacheParentDirectory())
        return NS_ERROR_NOT_AVAILABLE;

    if (!mDiskDevice)
        return NS_ERROR_NOT_AVAILABLE;

    if (!mObserver->SmartSizeEnabled())
        return NS_ERROR_NOT_AVAILABLE;

    nsAutoString cachePath;
    rv = mObserver->DiskCacheParentDirectory()->GetPath(cachePath);
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIRunnable> event =
            new nsGetSmartSizeEvent(cachePath, mDiskDevice->getCacheSize());
        DispatchToCacheIOThread(event);
    } else {
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}
