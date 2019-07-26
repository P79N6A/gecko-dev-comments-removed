





#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
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

#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIFile.h"
#include "nsIOService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "nsVoidArray.h"
#include "nsDeleteDir.h"
#include "nsNetCID.h"
#include <math.h>  
#include "mozilla/Services.h"
#include "nsITimer.h"

#include "mozilla/net/NeckoCommon.h"
#include "mozilla/VisualEventTracer.h"
#include <algorithm>

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

#define DISK_CACHE_USE_OLD_MAX_SMART_SIZE_PREF \
    "browser.cache.disk.smart_size.use_old_max"

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
    "last-pb-context-exited",
    "suspend_process_notification",
    "resume_process_notification"
};

static const char * prefList[] = { 
    DISK_CACHE_ENABLE_PREF,
    DISK_CACHE_SMART_SIZE_ENABLED_PREF,
    DISK_CACHE_CAPACITY_PREF,
    DISK_CACHE_DIR_PREF,
    DISK_CACHE_MAX_ENTRY_SIZE_PREF,
    DISK_CACHE_USE_OLD_MAX_SMART_SIZE_PREF,
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


const int32_t DEFAULT_CACHE_SIZE = 250 * 1024;  
const int32_t MIN_CACHE_SIZE = 50 * 1024;       
#ifdef ANDROID
const int32_t MAX_CACHE_SIZE = 200 * 1024;      
const int32_t OLD_MAX_CACHE_SIZE = 200 * 1024;  
#else
const int32_t MAX_CACHE_SIZE = 350 * 1024;      
const int32_t OLD_MAX_CACHE_SIZE = 1024 * 1024; 
#endif

const int32_t PRE_GECKO_2_0_DEFAULT_CACHE_SIZE = 50 * 1024;

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
        , mShouldUseOldMaxSmartSize(false)
        , mOfflineCacheEnabled(false)
        , mOfflineCacheCapacity(0)
        , mMemoryCacheEnabled(true)
        , mMemoryCacheCapacity(-1)
        , mMemoryCacheMaxEntrySize(-1) 
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
    int32_t         DiskCacheCapacity()         { return mDiskCacheCapacity; }
    void            SetDiskCacheCapacity(int32_t);
    int32_t         DiskCacheMaxEntrySize()     { return mDiskCacheMaxEntrySize; }
    nsIFile *       DiskCacheParentDirectory()  { return mDiskCacheParentDirectory; }
    bool            SmartSizeEnabled()          { return mSmartSizeEnabled; }

    bool            ShouldUseOldMaxSmartSize()        { return mShouldUseOldMaxSmartSize; }
    void            SetUseNewMaxSmartSize(bool useNew)     { mShouldUseOldMaxSmartSize = !useNew; }

    bool            OfflineCacheEnabled();
    int32_t         OfflineCacheCapacity()         { return mOfflineCacheCapacity; }
    nsIFile *       OfflineCacheParentDirectory()  { return mOfflineCacheParentDirectory; }
    
    bool            MemoryCacheEnabled();
    int32_t         MemoryCacheCapacity();
    int32_t         MemoryCacheMaxEntrySize()     { return mMemoryCacheMaxEntrySize; }

    int32_t         CacheCompressionLevel();

    bool            SanitizeAtShutdown() { return mSanitizeOnShutdown && mClearCacheOnShutdown; }

    static uint32_t GetSmartCacheSize(const nsAString& cachePath,
                                      uint32_t currentSize,
                                      bool shouldUseOldMaxSmartSize);

    bool                    PermittedToSmartSize(nsIPrefBranch*, bool firstRun);

private:
    bool                    mHaveProfile;
    
    bool                    mDiskCacheEnabled;
    int32_t                 mDiskCacheCapacity; 
    int32_t                 mDiskCacheMaxEntrySize; 
    nsCOMPtr<nsIFile>       mDiskCacheParentDirectory;
    bool                    mSmartSizeEnabled;

    bool                    mShouldUseOldMaxSmartSize;

    bool                    mOfflineCacheEnabled;
    int32_t                 mOfflineCacheCapacity; 
    nsCOMPtr<nsIFile>       mOfflineCacheParentDirectory;
    
    bool                    mMemoryCacheEnabled;
    int32_t                 mMemoryCacheCapacity; 
    int32_t                 mMemoryCacheMaxEntrySize; 

    int32_t                 mCacheCompressionLevel;

    bool                    mSanitizeOnShutdown;
    bool                    mClearCacheOnShutdown;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsCacheProfilePrefObserver, nsIObserver)

class nsSetDiskSmartSizeCallback MOZ_FINAL : public nsITimerCallback
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Notify(nsITimer* aTimer) {
        if (nsCacheService::gService) {
            nsCacheServiceAutoLock autoLock(LOCK_TELEM(NSSETDISKSMARTSIZECALLBACK_NOTIFY));
            nsCacheService::gService->SetDiskSmartSize_Locked();
            nsCacheService::gService->mSmartSizeTimer = nullptr;
        }
        return NS_OK;
    }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSetDiskSmartSizeCallback, nsITimerCallback)



class nsSetSmartSizeEvent: public nsRunnable 
{
public:
    nsSetSmartSizeEvent(int32_t smartSize)
        : mSmartSize(smartSize) {}

    NS_IMETHOD Run() 
    {
        NS_ASSERTION(NS_IsMainThread(), 
                     "Setting smart size data off the main thread");

        
        if (!nsCacheService::IsInitialized())
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
    int32_t mSmartSize;
};



class nsGetSmartSizeEvent: public nsRunnable
{
public:
    nsGetSmartSizeEvent(const nsAString& cachePath, uint32_t currentSize,
                        bool shouldUseOldMaxSmartSize)
      : mCachePath(cachePath)
      , mCurrentSize(currentSize)
      , mShouldUseOldMaxSmartSize(shouldUseOldMaxSmartSize)
    {}
   
    
    
    NS_IMETHOD Run()
    {
        uint32_t size;
        size = nsCacheProfilePrefObserver::GetSmartCacheSize(mCachePath,
                                                             mCurrentSize,
                                                             mShouldUseOldMaxSmartSize);
        NS_DispatchToMainThread(new nsSetSmartSizeEvent(size));
        return NS_OK;
    }

private:
    nsString mCachePath;
    uint32_t mCurrentSize;
    bool     mShouldUseOldMaxSmartSize;
};

class nsBlockOnCacheThreadEvent : public nsRunnable {
public:
    nsBlockOnCacheThreadEvent()
    {
    }
    NS_IMETHOD Run()
    {
        nsCacheServiceAutoLock autoLock(LOCK_TELEM(NSBLOCKONCACHETHREADEVENT_RUN));
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
nsCacheProfilePrefObserver::SetDiskCacheCapacity(int32_t capacity)
{
    mDiskCacheCapacity = std::max(0, capacity);
}


NS_IMETHODIMP
nsCacheProfilePrefObserver::Observe(nsISupports *     subject,
                                    const char *      topic,
                                    const PRUnichar * data_unicode)
{
    nsresult rv;
    NS_ConvertUTF16toUTF8 data(data_unicode);
    CACHE_LOG_ALWAYS(("Observe [topic=%s data=%s]\n", topic, data.get()));

    if (!nsCacheService::IsInitialized()) {
        if (!strcmp("resume_process_notification", topic)) {
            
            nsCacheService::GlobalInstance()->Init();
        }
        return NS_OK;
    }

    if (!strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, topic)) {
        
        nsCacheService::GlobalInstance()->Shutdown();
    } else if (!strcmp("profile-before-change", topic)) {
        
        mHaveProfile = false;

        
        nsCacheService::OnProfileShutdown(!strcmp("shutdown-cleanse",
                                                  data.get()));
        
    } else if (!strcmp("suspend_process_notification", topic)) {
        
        
        nsCacheService::GlobalInstance()->Shutdown();
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

            rv = branch->GetBoolPref(DISK_CACHE_ENABLE_PREF,
                                     &mDiskCacheEnabled);
            if (NS_FAILED(rv))  
                return rv;
            nsCacheService::SetDiskCacheEnabled(DiskCacheEnabled());

        } else if (!strcmp(DISK_CACHE_CAPACITY_PREF, data.get())) {

            int32_t capacity = 0;
            rv = branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &capacity);
            if (NS_FAILED(rv))  
                return rv;
            mDiskCacheCapacity = std::max(0, capacity);
            nsCacheService::SetDiskCacheCapacity(mDiskCacheCapacity);
       
        
        } else if (!strcmp(DISK_CACHE_SMART_SIZE_ENABLED_PREF, data.get())) {
            
            rv = branch->GetBoolPref(DISK_CACHE_SMART_SIZE_ENABLED_PREF,
                                     &mSmartSizeEnabled);
            if (NS_FAILED(rv)) 
                return rv;
            int32_t newCapacity = 0;
            if (mSmartSizeEnabled) {
                nsCacheService::SetDiskSmartSize();
            } else {
                
                rv = branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &newCapacity);
                if (NS_FAILED(rv)) 
                    return rv;
                mDiskCacheCapacity = std::max(0, newCapacity);
                nsCacheService::SetDiskCacheCapacity(mDiskCacheCapacity);
            }
        } else if (!strcmp(DISK_CACHE_USE_OLD_MAX_SMART_SIZE_PREF, data.get())) {
            rv = branch->GetBoolPref(DISK_CACHE_USE_OLD_MAX_SMART_SIZE_PREF,
                                     &mShouldUseOldMaxSmartSize);
            if (NS_FAILED(rv))
                return rv;
        } else if (!strcmp(DISK_CACHE_MAX_ENTRY_SIZE_PREF, data.get())) {
            int32_t newMaxSize;
            rv = branch->GetIntPref(DISK_CACHE_MAX_ENTRY_SIZE_PREF,
                                    &newMaxSize);
            if (NS_FAILED(rv)) 
                return rv;

            mDiskCacheMaxEntrySize = std::max(-1, newMaxSize);
            nsCacheService::SetDiskCacheMaxEntrySize(mDiskCacheMaxEntrySize);
          
#if 0            
        } else if (!strcmp(DISK_CACHE_DIR_PREF, data.get())) {
            
            
            
            
#endif            
        } else

        
        if (!strcmp(OFFLINE_CACHE_ENABLE_PREF, data.get())) {

            rv = branch->GetBoolPref(OFFLINE_CACHE_ENABLE_PREF,
                                     &mOfflineCacheEnabled);
            if (NS_FAILED(rv))  return rv;
            nsCacheService::SetOfflineCacheEnabled(OfflineCacheEnabled());

        } else if (!strcmp(OFFLINE_CACHE_CAPACITY_PREF, data.get())) {

            int32_t capacity = 0;
            rv = branch->GetIntPref(OFFLINE_CACHE_CAPACITY_PREF, &capacity);
            if (NS_FAILED(rv))  return rv;
            mOfflineCacheCapacity = std::max(0, capacity);
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
            int32_t newMaxSize;
            rv = branch->GetIntPref(MEMORY_CACHE_MAX_ENTRY_SIZE_PREF,
                                     &newMaxSize);
            if (NS_FAILED(rv)) 
                return rv;
            
            mMemoryCacheMaxEntrySize = std::max(-1, newMaxSize);
            nsCacheService::SetMemoryCacheMaxEntrySize(mMemoryCacheMaxEntrySize);
        } else if (!strcmp(CACHE_COMPRESSION_LEVEL_PREF, data.get())) {
            mCacheCompressionLevel = CACHE_COMPRESSION_LEVEL;
            (void)branch->GetIntPref(CACHE_COMPRESSION_LEVEL_PREF,
                                     &mCacheCompressionLevel);
            mCacheCompressionLevel = std::max(0, mCacheCompressionLevel);
            mCacheCompressionLevel = std::min(9, mCacheCompressionLevel);
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
    } else if (!strcmp("last-pb-context-exited", topic)) {
        nsCacheService::LeavePrivateBrowsing();
    }

    return NS_OK;
}



static uint32_t
SmartCacheSize(const uint32_t availKB, bool shouldUseOldMaxSmartSize)
{
    uint32_t maxSize = shouldUseOldMaxSmartSize ? OLD_MAX_CACHE_SIZE : MAX_CACHE_SIZE;

    if (availKB > 100 * 1024 * 1024)
        return maxSize;  

    
    
    
    uint32_t sz10MBs = 0;
    uint32_t avail10MBs = availKB / (1024*10);

    
    if (avail10MBs > 2500) {
        sz10MBs += static_cast<uint32_t>((avail10MBs - 2500)*.005);
        avail10MBs = 2500;
    }
    
    if (avail10MBs > 700) {
        sz10MBs += static_cast<uint32_t>((avail10MBs - 700)*.01);
        avail10MBs = 700;
    }
    
    if (avail10MBs > 50) {
        sz10MBs += static_cast<uint32_t>((avail10MBs - 50)*.05);
        avail10MBs = 50;
    }

#ifdef ANDROID
    
    
    

    
    sz10MBs += std::max<uint32_t>(1, static_cast<uint32_t>(avail10MBs * .2));
#else
    
    sz10MBs += std::max<uint32_t>(5, static_cast<uint32_t>(avail10MBs * .4));
#endif

    return std::min<uint32_t>(maxSize, sz10MBs * 10 * 1024);
}

 










uint32_t
nsCacheProfilePrefObserver::GetSmartCacheSize(const nsAString& cachePath,
                                              uint32_t currentSize,
                                              bool shouldUseOldMaxSmartSize)
{
    
    nsresult rv;
    nsCOMPtr<nsIFile> 
        cacheDirectory (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
    if (NS_FAILED(rv) || !cacheDirectory)
        return DEFAULT_CACHE_SIZE;
    rv = cacheDirectory->InitWithPath(cachePath);
    if (NS_FAILED(rv))
        return DEFAULT_CACHE_SIZE;
    int64_t bytesAvailable;
    rv = cacheDirectory->GetDiskSpaceAvailable(&bytesAvailable);
    if (NS_FAILED(rv))
        return DEFAULT_CACHE_SIZE;

    return SmartCacheSize(static_cast<uint32_t>((bytesAvailable / 1024) +
                                                currentSize),
                          shouldUseOldMaxSmartSize);
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
            int32_t oldCapacity;
            
            
            rv = branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &oldCapacity);
            if (oldCapacity < PRE_GECKO_2_0_DEFAULT_CACHE_SIZE) {
                mSmartSizeEnabled = false;
                branch->SetBoolPref(DISK_CACHE_SMART_SIZE_ENABLED_PREF,
                                    mSmartSizeEnabled);
                return mSmartSizeEnabled;
            }
        }
        
        
        int32_t maxSize = mShouldUseOldMaxSmartSize ? OLD_MAX_CACHE_SIZE : MAX_CACHE_SIZE;
        branch->SetIntPref(DISK_CACHE_CAPACITY_PREF, maxSize);
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

    
    mDiskCacheEnabled = true;  
    (void) branch->GetBoolPref(DISK_CACHE_ENABLE_PREF, &mDiskCacheEnabled);

    mDiskCacheCapacity = DISK_CACHE_CAPACITY;
    (void)branch->GetIntPref(DISK_CACHE_CAPACITY_PREF, &mDiskCacheCapacity);
    mDiskCacheCapacity = std::max(0, mDiskCacheCapacity);

    (void) branch->GetIntPref(DISK_CACHE_MAX_ENTRY_SIZE_PREF,
                              &mDiskCacheMaxEntrySize);
    mDiskCacheMaxEntrySize = std::max(-1, mDiskCacheMaxEntrySize);
    
    (void) branch->GetComplexValue(DISK_CACHE_DIR_PREF,     
                                   NS_GET_IID(nsIFile),
                                   getter_AddRefs(mDiskCacheParentDirectory));

    (void) branch->GetBoolPref(DISK_CACHE_USE_OLD_MAX_SMART_SIZE_PREF,
                               &mShouldUseOldMaxSmartSize);
    
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
                nsCacheService::MoveOrRemoveDiskCache(profDir, directory, 
                                                      "Cache");
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

    
    mOfflineCacheEnabled = true;  
    (void) branch->GetBoolPref(OFFLINE_CACHE_ENABLE_PREF,
                              &mOfflineCacheEnabled);

    mOfflineCacheCapacity = OFFLINE_CACHE_CAPACITY;
    (void)branch->GetIntPref(OFFLINE_CACHE_CAPACITY_PREF,
                             &mOfflineCacheCapacity);
    mOfflineCacheCapacity = std::max(0, mOfflineCacheCapacity);

    (void) branch->GetComplexValue(OFFLINE_CACHE_DIR_PREF,     
                                   NS_GET_IID(nsIFile),
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
            else if (profDir) {
                nsCacheService::MoveOrRemoveDiskCache(profDir, directory, 
                                                      "OfflineCache");
            }
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
    mMemoryCacheMaxEntrySize = std::max(-1, mMemoryCacheMaxEntrySize);

    
    mCacheCompressionLevel = CACHE_COMPRESSION_LEVEL;
    (void)branch->GetIntPref(CACHE_COMPRESSION_LEVEL_PREF,
                             &mCacheCompressionLevel);
    mCacheCompressionLevel = std::max(0, mCacheCompressionLevel);
    mCacheCompressionLevel = std::min(9, mCacheCompressionLevel);

    
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

































int32_t
nsCacheProfilePrefObserver::MemoryCacheCapacity()
{
    int32_t capacity = mMemoryCacheCapacity;
    if (capacity >= 0) {
        CACHE_LOG_DEBUG(("Memory cache capacity forced to %d\n", capacity));
        return capacity;
    }

    static uint64_t bytes = PR_GetPhysicalMemorySize();
    CACHE_LOG_DEBUG(("Physical Memory size is %llu\n", bytes));

    
    
    
    if (bytes == 0)
        bytes = 32 * 1024 * 1024;

    
    
    
    if (bytes > INT64_MAX)
        bytes = INT64_MAX;

    uint64_t kbytes = bytes >> 10;

    double kBytesD = double(kbytes);

    double x = log(kBytesD)/log(2.0) - 14;
    if (x > 0) {
        capacity = (int32_t)(x * x / 3.0 + x + 2.0 / 3 + 0.1); 
        if (capacity > 32)
            capacity = 32;
        capacity   *= 1024;
    } else {
        capacity    = 0;
    }

    return capacity;
}

int32_t
nsCacheProfilePrefObserver::CacheCompressionLevel()
{
    return mCacheCompressionLevel;
}





class nsProcessRequestEvent : public nsRunnable {
public:
    nsProcessRequestEvent(nsCacheRequest *aRequest)
    {
        MOZ_EVENT_TRACER_NAME_OBJECT(aRequest, aRequest->mKey.get());
        MOZ_EVENT_TRACER_WAIT(aRequest, "net::cache::ProcessRequest");
        mRequest = aRequest;
    }

    NS_IMETHOD Run()
    {
        nsresult rv;

        NS_ASSERTION(mRequest->mListener,
                     "Sync OpenCacheEntry() posted to background thread!");

        nsCacheServiceAutoLock lock(LOCK_TELEM(NSPROCESSREQUESTEVENT_RUN));
        rv = nsCacheService::gService->ProcessRequest(mRequest,
                                                      false,
                                                      nullptr);

        
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
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSDOOMEVENT_RUN));

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
            
            mListener = nullptr;
        }

        return NS_OK;
    }

private:
    nsCString             mKey;
    nsCacheStoragePolicy  mStoragePolicy;
    nsICacheListener     *mListener;
    nsCOMPtr<nsIThread>   mThread;
};




nsCacheService *   nsCacheService::gService = nullptr;

NS_IMPL_THREADSAFE_ISUPPORTS1(nsCacheService, nsICacheService)

nsCacheService::nsCacheService()
    : mObserver(nullptr),
      mLock("nsCacheService.mLock"),
      mCondVar(mLock, "nsCacheService.mCondVar"),
      mInitialized(false),
      mClearingEntries(false),
      mEnableMemoryDevice(true),
      mEnableDiskDevice(true),
      mMemoryDevice(nullptr),
      mDiskDevice(nullptr),
      mOfflineDevice(nullptr),
      mTotalEntries(0),
      mCacheHits(0),
      mCacheMisses(0),
      mMaxKeyLength(0),
      mMaxDataSize(0),
      mMaxMetaSize(0),
      mDeactivateFailures(0),
      mDeactivatedUnboundEntries(0)
{
    NS_ASSERTION(gService==nullptr, "multiple nsCacheService instances!");
    gService = this;

    
    PR_INIT_CLIST(&mDoomedEntries);
    mCustomOfflineDevices.Init();
}

nsCacheService::~nsCacheService()
{
    if (mInitialized) 
        (void) Shutdown();

    if (mObserver) {
        mObserver->Remove();
        NS_RELEASE(mObserver);
    }

    gService = nullptr;
}


nsresult
nsCacheService::Init()
{
    
    
    if (!NS_IsMainThread()) {
        NS_ERROR("nsCacheService::Init called off the main thread");
        return NS_ERROR_NOT_SAME_THREAD;
    }

    NS_ASSERTION(!mInitialized, "nsCacheService already initialized.");
    if (mInitialized)
        return NS_ERROR_ALREADY_INITIALIZED;

    if (mozilla::net::IsNeckoChild()) {
        return NS_ERROR_UNEXPECTED;
    }

    CACHE_LOG_INIT();

    MOZ_EVENT_TRACER_NAME_OBJECT(nsCacheService::gService, "nsCacheService");

    nsresult rv = NS_NewNamedThread("Cache I/O",
                                    getter_AddRefs(mCacheIOThread));
    if (NS_FAILED(rv)) {
        NS_RUNTIMEABORT("Can't create cache IO thread");
    }

    rv = nsDeleteDir::Init();
    if (NS_FAILED(rv)) {
        NS_WARNING("Can't initialize nsDeleteDir");
    }

    
    rv = mActiveEntries.Init();
    if (NS_FAILED(rv)) return rv;
    
    
    if (!mObserver) {
      mObserver = new nsCacheProfilePrefObserver();
      NS_ADDREF(mObserver);
      mObserver->Install();
    }

    mEnableDiskDevice    = mObserver->DiskCacheEnabled();
    mEnableOfflineDevice = mObserver->OfflineCacheEnabled();
    mEnableMemoryDevice  = mObserver->MemoryCacheEnabled();

    mInitialized = true;
    return NS_OK;
}


PLDHashOperator
nsCacheService::ShutdownCustomCacheDeviceEnum(const nsAString& aProfileDir,
                                              nsRefPtr<nsOfflineCacheDevice>& aDevice,
                                              void* aUserArg)
{
    aDevice->Shutdown();
    return PL_DHASH_REMOVE;
}

void
nsCacheService::Shutdown()
{
    
    
    if (!NS_IsMainThread()) {
        NS_RUNTIMEABORT("nsCacheService::Shutdown called off the main thread");
    }

    nsCOMPtr<nsIThread> cacheIOThread;
    Telemetry::AutoTimer<Telemetry::NETWORK_DISK_CACHE_SHUTDOWN> totalTimer;

    bool shouldSanitize = false;
    nsCOMPtr<nsIFile> parentDir;

    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SHUTDOWN));
        NS_ASSERTION(mInitialized,
            "can't shutdown nsCacheService unless it has been initialized.");
        if (!mInitialized)
            return;

        mClearingEntries = true;
        DoomActiveEntries(nullptr);
    }

    CloseAllStreams();

    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SHUTDOWN));
        NS_ASSERTION(mInitialized, "Bad state");

        mInitialized = false;

        
        ClearDoomList();

        if (mSmartSizeTimer) {
            mSmartSizeTimer->Cancel();
            mSmartSizeTimer = nullptr;
        }

        
        
        (void) SyncWithCacheIOThread();

        
        parentDir = mObserver->DiskCacheParentDirectory();
        shouldSanitize = mObserver->SanitizeAtShutdown();

        
        delete mMemoryDevice;
        mMemoryDevice = nullptr;

        delete mDiskDevice;
        mDiskDevice = nullptr;

        if (mOfflineDevice)
            mOfflineDevice->Shutdown();

        NS_IF_RELEASE(mOfflineDevice);

        mCustomOfflineDevices.Enumerate(&nsCacheService::ShutdownCustomCacheDeviceEnum, nullptr);

#ifdef PR_LOGGING
        LogCacheStatistics();
#endif

        mClearingEntries = false;
        mCacheIOThread.swap(cacheIOThread);
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

    if (aOuter != nullptr)
        return NS_ERROR_NO_AGGREGATION;

    nsCacheService * cacheService = new nsCacheService();
    if (cacheService == nullptr)
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
    *result = nullptr;

    if (this == nullptr)  return NS_ERROR_NOT_AVAILABLE;

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
                                nullptr);
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

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_EVICTENTRIESFORCLIENT));
    nsresult res = NS_OK;

    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_ON_DISK) {

        if (mEnableDiskDevice) {
            nsresult rv = NS_OK;
            if (!mDiskDevice)
                rv = CreateDiskDevice();
            if (mDiskDevice)
                rv = mDiskDevice->EvictEntries(clientID);
            if (NS_FAILED(rv))
                res = rv;
        }
    }

    
    if (storagePolicy == nsICache::STORE_OFFLINE) {
        if (mEnableOfflineDevice) {
            nsresult rv = NS_OK;
            if (!mOfflineDevice)
                rv = CreateOfflineDevice();
            if (mOfflineDevice)
                rv = mOfflineDevice->EvictEntries(clientID);
            if (NS_FAILED(rv))
                res = rv;
        }
    }

    if (storagePolicy == nsICache::STORE_ANYWHERE ||
        storagePolicy == nsICache::STORE_IN_MEMORY) {
        
        if (mMemoryDevice) {
            nsresult rv = mMemoryDevice->EvictEntries(clientID);
            if (NS_FAILED(rv))
                res = rv;
        }
    }

    return res;
}


nsresult        
nsCacheService::IsStorageEnabledForPolicy(nsCacheStoragePolicy  storagePolicy,
                                          bool *              result)
{
    if (gService == nullptr) return NS_ERROR_NOT_AVAILABLE;
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_ISSTORAGEENABLEDFORPOLICY));

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
         storagePolicy == nsICache::STORE_ON_DISK)) {
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

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_VISITENTRIES));

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
    return  EvictEntriesForClient(nullptr, storagePolicy);
}

NS_IMETHODIMP nsCacheService::GetCacheIOTarget(nsIEventTarget * *aCacheIOTarget)
{
    NS_ENSURE_ARG_POINTER(aCacheIOTarget);

    
    
    
    if (!NS_IsMainThread()) {
        Lock(LOCK_TELEM(NSCACHESERVICE_GETCACHEIOTARGET));
    }

    nsresult rv;
    if (mCacheIOThread) {
        NS_ADDREF(*aCacheIOTarget = mCacheIOThread);
        rv = NS_OK;
    } else {
        *aCacheIOTarget = nullptr;
        rv = NS_ERROR_NOT_AVAILABLE;
    }

    if (!NS_IsMainThread()) {
        Unlock();
    }

    return rv;
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
        printf("### mDiskDevice->Init() failed (0x%.8x)\n",
               static_cast<uint32_t>(rv));
        printf("###    - disabling disk cache for this session.\n");
        printf("###\n");
#endif
        mEnableDiskDevice = false;
        delete mDiskDevice;
        mDiskDevice = nullptr;
        return rv;
    }

    Telemetry::Accumulate(Telemetry::DISK_CACHE_SMART_SIZE_USING_OLD_MAX,
                          mObserver->ShouldUseOldMaxSmartSize());

    NS_ASSERTION(!mSmartSizeTimer, "Smartsize timer was already fired!");

    
    
    
    mSmartSizeTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = mSmartSizeTimer->InitWithCallback(new nsSetDiskSmartSizeCallback(),
                                               1000*60*3,
                                               nsITimer::TYPE_ONE_SHOT);
        if (NS_FAILED(rv)) {
            NS_WARNING("Failed to post smart size timer");
            mSmartSizeTimer = nullptr;
        }
    } else {
        NS_WARNING("Can't create smart size timer");
    }
    
    

    return NS_OK;
}


class nsDisableOldMaxSmartSizePrefEvent: public nsRunnable
{
public:
    nsDisableOldMaxSmartSizePrefEvent() {}

    NS_IMETHOD Run()
    {
        
        if (!nsCacheService::IsInitialized())
            return NS_ERROR_NOT_AVAILABLE;

        nsCOMPtr<nsIPrefBranch> branch = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (!branch) {
            return NS_ERROR_NOT_AVAILABLE;
        }

        nsresult rv = branch->SetBoolPref(DISK_CACHE_USE_OLD_MAX_SMART_SIZE_PREF, false);
        if (NS_FAILED(rv)) {
            NS_WARNING("Failed to disable old max smart size");
            return rv;
        }

        nsCacheService::SetDiskSmartSize();

        if (nsCacheService::gService->mObserver->PermittedToSmartSize(branch, false)) {
            rv = branch->SetIntPref(DISK_CACHE_CAPACITY_PREF, MAX_CACHE_SIZE);
            if (NS_FAILED(rv)) {
                NS_WARNING("Failed to set cache capacity pref");
            }
        }

        return NS_OK;
    }
};

void
nsCacheService::MarkStartingFresh()
{
    if (!gService->mObserver->ShouldUseOldMaxSmartSize()) {
        
        return;
    }

    gService->mObserver->SetUseNewMaxSmartSize(true);

    
    
    NS_DispatchToMainThread(new nsDisableOldMaxSmartSizePrefEvent());
}

nsresult
nsCacheService::GetOfflineDevice(nsOfflineCacheDevice **aDevice)
{
    if (!mOfflineDevice) {
        nsresult rv = CreateOfflineDevice();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_ADDREF(*aDevice = mOfflineDevice);
    return NS_OK;
}

nsresult
nsCacheService::GetCustomOfflineDevice(nsIFile *aProfileDir,
                                       int32_t aQuota,
                                       nsOfflineCacheDevice **aDevice)
{
    nsresult rv;

    nsAutoString profilePath;
    rv = aProfileDir->GetPath(profilePath);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mCustomOfflineDevices.Get(profilePath, aDevice)) {
        rv = CreateCustomOfflineDevice(aProfileDir, aQuota, aDevice);
        NS_ENSURE_SUCCESS(rv, rv);

        (*aDevice)->SetAutoShutdown();
        mCustomOfflineDevices.Put(profilePath, *aDevice);
    }

    return NS_OK;
}

nsresult
nsCacheService::CreateOfflineDevice()
{
    CACHE_LOG_ALWAYS(("Creating default offline device"));

    if (mOfflineDevice)        return NS_OK;
    if (!nsCacheService::IsInitialized()) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    nsresult rv = CreateCustomOfflineDevice(
        mObserver->OfflineCacheParentDirectory(),
        mObserver->OfflineCacheCapacity(),
        &mOfflineDevice);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
nsCacheService::CreateCustomOfflineDevice(nsIFile *aProfileDir,
                                          int32_t aQuota,
                                          nsOfflineCacheDevice **aDevice)
{
    NS_ENSURE_ARG(aProfileDir);

#if defined(PR_LOGGING)
    nsAutoCString profilePath;
    aProfileDir->GetNativePath(profilePath);
    CACHE_LOG_ALWAYS(("Creating custom offline device, %s, %d",
                      profilePath.BeginReading(), aQuota));
#endif

    if (!mInitialized)         return NS_ERROR_NOT_AVAILABLE;
    if (!mEnableOfflineDevice) return NS_ERROR_NOT_AVAILABLE;

    *aDevice = new nsOfflineCacheDevice;

    NS_ADDREF(*aDevice);

    
    (*aDevice)->SetCacheParentDirectory(aProfileDir);
    (*aDevice)->SetCapacity(aQuota);

    nsresult rv = (*aDevice)->Init();
    if (NS_FAILED(rv)) {
        CACHE_LOG_DEBUG(("OfflineDevice->Init() failed (0x%.8x)\n", rv));
        CACHE_LOG_DEBUG(("    - disabling offline cache for this session.\n"));

        NS_RELEASE(*aDevice);
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
    
    
    int32_t capacity = mObserver->MemoryCacheCapacity();
    CACHE_LOG_DEBUG(("Creating memory device with capacity %d\n", capacity));
    mMemoryDevice->SetCapacity(capacity);
    mMemoryDevice->SetMaxEntrySize(mObserver->MemoryCacheMaxEntrySize());

    nsresult rv = mMemoryDevice->Init();
    if (NS_FAILED(rv)) {
        NS_WARNING("Initialization of Memory Cache failed.");
        delete mMemoryDevice;
        mMemoryDevice = nullptr;
    }

    return rv;
}

nsresult
nsCacheService::RemoveCustomOfflineDevice(nsOfflineCacheDevice *aDevice)
{
    nsCOMPtr<nsIFile> profileDir = aDevice->BaseDirectory();
    if (!profileDir)
        return NS_ERROR_UNEXPECTED;

    nsAutoString profilePath;
    nsresult rv = profileDir->GetPath(profilePath);
    NS_ENSURE_SUCCESS(rv, rv);

    mCustomOfflineDevices.Remove(profilePath);
    return NS_OK;
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
     
    nsAutoCString key(*session->ClientID());
    key.Append(':');
    key.Append(clientKey);

    if (mMaxKeyLength < key.Length()) mMaxKeyLength = key.Length();

    
    *request = new nsCacheRequest(key, listener, accessRequested,
                                  blockingMode, session);

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
        mozilla::eventtracer::AutoEventTracer tracer(
            static_cast<nsIRunnable*>(this),
            eventtracer::eExec,
            eventtracer::eDone,
            "net::cache::OnCacheEntryAvailable");

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
    request->mListener = nullptr;

    nsCOMPtr<nsIRunnable> ev =
            new nsCacheListenerEvent(listener, descriptor,
                                     accessGranted, status);
    if (!ev) {
        
        
        
        return NS_ERROR_OUT_OF_MEMORY;
    }

    MOZ_EVENT_TRACER_NAME_OBJECT(ev.get(), request->mKey.get());
    MOZ_EVENT_TRACER_WAIT(ev.get(), "net::cache::OnCacheEntryAvailable");
    return request->mThread->Dispatch(ev, NS_DISPATCH_NORMAL);
}


nsresult
nsCacheService::ProcessRequest(nsCacheRequest *           request,
                               bool                       calledFromOpenCacheEntry,
                               nsICacheEntryDescriptor ** result)
{
    mozilla::eventtracer::AutoEventTracer tracer(
        request,
        eventtracer::eExec,
        eventtracer::eDone,
        "net::cache::ProcessRequest");

    
    nsresult           rv;
    nsCacheEntry *     entry = nullptr;
    nsCacheEntry *     doomedEntry = nullptr;
    nsCacheAccessMode  accessGranted = nsICache::ACCESS_NONE;
    if (result) *result = nullptr;

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
                Lock(LOCK_TELEM(NSCACHESERVICE_PROCESSREQUEST));
            }

            PR_REMOVE_AND_INIT_LINK(request);
            if (NS_FAILED(rv)) break;   
            
        }
        if (rv != NS_ERROR_CACHE_ENTRY_DOOMED)  break;

        if (entry->IsNotInUse()) {
            
            DeactivateEntry(entry);
        }
        
    }

    if (NS_SUCCEEDED(rv) && request->mProfileDir) {
        
        if (entry->StoragePolicy() != nsICache::STORE_OFFLINE) {
            
            rv = NS_ERROR_FAILURE;
        } else {
            nsRefPtr<nsOfflineCacheDevice> customCacheDevice;
            rv = GetCustomOfflineDevice(request->mProfileDir, -1,
                                        getter_AddRefs(customCacheDevice));
            if (NS_SUCCEEDED(rv))
                entry->SetCustomCacheDevice(customCacheDevice);
        }
    }

    nsICacheEntryDescriptor *descriptor = nullptr;
    
    if (NS_SUCCEEDED(rv))
        rv = entry->CreateDescriptor(request, accessGranted, &descriptor);

    
    
    
    
    
    
    
    
    
    
    
    if (doomedEntry) {
        (void) ProcessPendingRequests(doomedEntry);
        if (doomedEntry->IsNotInUse())
            DeactivateEntry(doomedEntry);
        doomedEntry = nullptr;
    }

    if (request->mListener) {  
    
        if (NS_FAILED(rv) && calledFromOpenCacheEntry && request->IsBlocking())
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
        *result = nullptr;

    if (!gService->mInitialized)
        return NS_ERROR_NOT_INITIALIZED;

    nsCacheRequest * request = nullptr;

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

        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_OPENCACHEENTRY));
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
    if (!mInitialized || mClearingEntries)
        return NS_ERROR_NOT_AVAILABLE;

    mozilla::eventtracer::AutoEventTracer tracer(
        request,
        eventtracer::eExec,
        eventtracer::eDone,
        "net::cache::ActivateEntry");

    nsresult        rv = NS_OK;

    NS_ASSERTION(request != nullptr, "ActivateEntry called with no request");
    if (result) *result = nullptr;
    if (doomedEntry) *doomedEntry = nullptr;
    if ((!request) || (!result) || (!doomedEntry))
        return NS_ERROR_NULL_POINTER;

    
    if (!mEnableMemoryDevice && !request->IsStreamBased())
        return NS_ERROR_FAILURE;
    if (!IsStorageEnabledForPolicy_Locked(request->StoragePolicy()))
        return NS_ERROR_FAILURE;

    
    nsCacheEntry *entry = mActiveEntries.GetEntry(&(request->mKey));
    CACHE_LOG_DEBUG(("Active entry for request %p is %p\n", request, entry));

    if (!entry) {
        
        bool collision = false;
        entry = SearchCacheDevices(&(request->mKey), request->StoragePolicy(), &collision);
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
        entry = nullptr;
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

        if (request->IsPrivate())
            entry->MarkPrivate();
        
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
    *result = nullptr;
    delete entry;
    return rv;
}


nsCacheEntry *
nsCacheService::SearchCacheDevices(nsCString * key, nsCacheStoragePolicy policy, bool *collision)
{
    Telemetry::AutoTimer<Telemetry::CACHE_DEVICE_SEARCH_2> timer;
    nsCacheEntry * entry = nullptr;

    MOZ_EVENT_TRACER_NAME_OBJECT(key, key->BeginReading());
    eventtracer::AutoEventTracer searchCacheDevices(
        key,
        eventtracer::eExec,
        eventtracer::eDone,
        "net::cache::SearchCacheDevices");

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
                    return nullptr;
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
                    return nullptr;
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

    int64_t predictedDataSize = entry->PredictedDataSize();
    if (entry->IsStreamData() && entry->IsAllowedOnDisk() && mEnableDiskDevice) {
        
        if (!mDiskDevice) {
            (void)CreateDiskDevice();  
        }

        if (mDiskDevice) {
            
            if (predictedDataSize != -1 &&
                mDiskDevice->EntryIsTooBig(predictedDataSize)) {
                DebugOnly<nsresult> rv = nsCacheService::DoomEntry(entry);
                NS_ASSERTION(NS_SUCCEEDED(rv),"DoomEntry() failed.");
                return nullptr;
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
                return nullptr;
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

        device = entry->CustomCacheDevice()
               ? entry->CustomCacheDevice()
               : mOfflineDevice;

        if (device) {
            entry->MarkBinding();
            nsresult rv = device->BindEntry(entry);
            entry->ClearBinding();
            if (NS_FAILED(rv))
                device = nullptr;
        }
    }

    if (device) 
        entry->SetCacheDevice(device);
    return device;
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
    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_ONPROFILESHUTDOWN));
        gService->mClearingEntries = true;
        gService->DoomActiveEntries(nullptr);
    }

    gService->CloseAllStreams();

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_ONPROFILESHUTDOWN));
    gService->ClearDoomList();

    
    
    (void) SyncWithCacheIOThread();

    if (gService->mDiskDevice && gService->mEnableDiskDevice) {
        if (cleanse)
            gService->mDiskDevice->EvictEntries(nullptr);

        gService->mDiskDevice->Shutdown();
    }
    gService->mEnableDiskDevice = false;

    if (gService->mOfflineDevice && gService->mEnableOfflineDevice) {
        if (cleanse)
            gService->mOfflineDevice->EvictEntries(nullptr);

        gService->mOfflineDevice->Shutdown();
    }
    gService->mCustomOfflineDevices.Enumerate(
        &nsCacheService::ShutdownCustomCacheDeviceEnum, nullptr);

    gService->mEnableOfflineDevice = false;

    if (gService->mMemoryDevice) {
        
        gService->mMemoryDevice->EvictEntries(nullptr);
    }

    gService->mClearingEntries = false;
}


void
nsCacheService::OnProfileChanged()
{
    if (!gService)  return;

    CACHE_LOG_DEBUG(("nsCacheService::OnProfileChanged"));
 
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_ONPROFILECHANGED));
    
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
            
            int32_t capacity = gService->mObserver->MemoryCacheCapacity();
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
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETDISKCACHEENABLED));
    gService->mEnableDiskDevice = enabled;
}


void
nsCacheService::SetDiskCacheCapacity(int32_t  capacity)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETDISKCACHECAPACITY));

    if (gService->mDiskDevice) {
        gService->mDiskDevice->SetCapacity(capacity);
    }

    gService->mEnableDiskDevice = gService->mObserver->DiskCacheEnabled();
}

void
nsCacheService::SetDiskCacheMaxEntrySize(int32_t  maxSize)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETDISKCACHEMAXENTRYSIZE));

    if (gService->mDiskDevice) {
        gService->mDiskDevice->SetMaxEntrySize(maxSize);
    }
}

void
nsCacheService::SetMemoryCacheMaxEntrySize(int32_t  maxSize)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETMEMORYCACHEMAXENTRYSIZE));

    if (gService->mMemoryDevice) {
        gService->mMemoryDevice->SetMaxEntrySize(maxSize);
    }
}

void
nsCacheService::SetOfflineCacheEnabled(bool    enabled)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETOFFLINECACHEENABLED));
    gService->mEnableOfflineDevice = enabled;
}

void
nsCacheService::SetOfflineCacheCapacity(int32_t  capacity)
{
    if (!gService)  return;
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETOFFLINECACHECAPACITY));

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

    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETMEMORYCACHE));

    gService->mEnableMemoryDevice = gService->mObserver->MemoryCacheEnabled();

    if (gService->mEnableMemoryDevice) {
        if (gService->mMemoryDevice) {
            int32_t capacity = gService->mObserver->MemoryCacheCapacity();
            
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
    
    nsCacheEntry * entry = descriptor->CacheEntry();
    bool doomEntry;
    bool stillActive = entry->RemoveDescriptor(descriptor, &doomEntry);

    if (!entry->IsValid()) {
        gService->ProcessPendingRequests(entry);
    }

    if (doomEntry) {
        gService->DoomEntry_Internal(entry, true);
        return;
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
                                        uint32_t           offset,
                                        nsIInputStream  ** result)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;

    return device->OpenInputStreamForEntry(entry, mode, offset, result);
}

nsresult
nsCacheService::OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                         nsCacheAccessMode  mode,
                                         uint32_t           offset,
                                         nsIOutputStream ** result)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;

    return device->OpenOutputStreamForEntry(entry, mode, offset, result);
}


nsresult
nsCacheService::OnDataSizeChange(nsCacheEntry * entry, int32_t deltaSize)
{
    nsCacheDevice * device = gService->EnsureEntryHasDevice(entry);
    if (!device)  return  NS_ERROR_UNEXPECTED;

    return device->OnDataSizeChange(entry, deltaSize);
}

void
nsCacheService::Lock(mozilla::Telemetry::ID mainThreadLockerID)
{
    mozilla::Telemetry::ID lockerID;
    mozilla::Telemetry::ID generalID;

    if (NS_IsMainThread()) {
        lockerID = mainThreadLockerID;
        generalID = mozilla::Telemetry::CACHE_SERVICE_LOCK_WAIT_MAINTHREAD_2;
    } else {
        lockerID = mozilla::Telemetry::HistogramCount;
        generalID = mozilla::Telemetry::CACHE_SERVICE_LOCK_WAIT_2;
    }

    TimeStamp start(TimeStamp::Now());
    MOZ_EVENT_TRACER_WAIT(nsCacheService::gService, "net::cache::lock");

    gService->mLock.Lock();

    TimeStamp stop(TimeStamp::Now());
    MOZ_EVENT_TRACER_EXEC(nsCacheService::gService, "net::cache::lock");

    
    
    if (lockerID != mozilla::Telemetry::HistogramCount) {
        mozilla::Telemetry::AccumulateTimeDelta(lockerID, start, stop);
    }
    mozilla::Telemetry::AccumulateTimeDelta(generalID, start, stop);
}

void
nsCacheService::Unlock()
{
    gService->mLock.AssertCurrentThreadOwns();

    nsTArray<nsISupports*> doomed;
    doomed.SwapElements(gService->mDoomedObjects);

    gService->mLock.Unlock();

    MOZ_EVENT_TRACER_DONE(nsCacheService::gService, "net::cache::lock");

    for (uint32_t i = 0; i < doomed.Length(); ++i)
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


int32_t
nsCacheService::CacheCompressionLevel()
{
    int32_t level = gService->mObserver->CacheCompressionLevel();
    return level;
}


void
nsCacheService::DeactivateEntry(nsCacheEntry * entry)
{
    CACHE_LOG_DEBUG(("Deactivating entry %p\n", entry));
    nsresult  rv = NS_OK;
    NS_ASSERTION(entry->IsNotInUse(), "### deactivating an entry while in use!");
    nsCacheDevice * device = nullptr;

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
    mozilla::eventtracer::AutoEventTracer tracer(
        entry,
        eventtracer::eExec,
        eventtracer::eDone,
        "net::cache::ProcessPendingRequests");

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
                rv = ProcessRequest(request, false, nullptr);
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
                

                
                nsICacheEntryDescriptor *descriptor = nullptr;
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

bool
nsCacheService::IsDoomListEmpty()
{
    nsCacheEntry * entry = (nsCacheEntry *)PR_LIST_HEAD(&mDoomedEntries);
    return &mDoomedEntries == entry;
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

PLDHashOperator
nsCacheService::GetActiveEntries(PLDHashTable *    table,
                                 PLDHashEntryHdr * hdr,
                                 uint32_t          number,
                                 void *            arg)
{
    static_cast<nsVoidArray *>(arg)->AppendElement(
        ((nsCacheEntryHashTableEntry *)hdr)->cacheEntry);
    return PL_DHASH_NEXT;
}

struct ActiveEntryArgs
{
    nsTArray<nsCacheEntry*>* mActiveArray;
    nsCacheService::DoomCheckFn mCheckFn;
};

void
nsCacheService::DoomActiveEntries(DoomCheckFn check)
{
    nsAutoTArray<nsCacheEntry*, 8> array;
    ActiveEntryArgs args = { &array, check };

    mActiveEntries.VisitEntries(RemoveActiveEntry, &args);

    uint32_t count = array.Length();
    for (uint32_t i=0; i < count; ++i)
        DoomEntry_Internal(array[i], true);
}

PLDHashOperator
nsCacheService::RemoveActiveEntry(PLDHashTable *    table,
                                  PLDHashEntryHdr * hdr,
                                  uint32_t          number,
                                  void *            arg)
{
    nsCacheEntry * entry = ((nsCacheEntryHashTableEntry *)hdr)->cacheEntry;
    NS_ASSERTION(entry, "### active entry = nullptr!");

    ActiveEntryArgs* args = static_cast<ActiveEntryArgs*>(arg);
    if (args->mCheckFn && !args->mCheckFn(entry))
        return PL_DHASH_NEXT;

    NS_ASSERTION(args->mActiveArray, "### array = nullptr!");
    args->mActiveArray->AppendElement(entry);

    
    entry->MarkInactive();
    return PL_DHASH_REMOVE; 
}


void
nsCacheService::CloseAllStreams()
{
    nsTArray<nsRefPtr<nsCacheEntryDescriptor::nsInputStreamWrapper> > inputs;
    nsTArray<nsRefPtr<nsCacheEntryDescriptor::nsOutputStreamWrapper> > outputs;

    {
        nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_CLOSEALLSTREAMS));

        nsVoidArray entries;

#if DEBUG
        
        mActiveEntries.VisitEntries(GetActiveEntries, &entries);
        NS_ASSERTION(entries.Count() == 0, "Bad state");
#endif

        
        nsCacheEntry * entry = (nsCacheEntry *)PR_LIST_HEAD(&mDoomedEntries);
        while (entry != &mDoomedEntries) {
            nsCacheEntry * next = (nsCacheEntry *)PR_NEXT_LINK(entry);
            entries.AppendElement(entry);
            entry = next;
        }

        
        for (int32_t i = 0 ; i < entries.Count() ; i++) {
            entry = static_cast<nsCacheEntry *>(entries.ElementAt(i));

            nsTArray<nsRefPtr<nsCacheEntryDescriptor> > descs;
            entry->GetDescriptors(descs);

            for (uint32_t j = 0 ; j < descs.Length() ; j++) {
                if (descs[j]->mOutputWrapper)
                    outputs.AppendElement(descs[j]->mOutputWrapper);

                for (int32_t k = 0 ; k < descs[j]->mInputWrappers.Count() ; k++)
                    inputs.AppendElement(static_cast<
                        nsCacheEntryDescriptor::nsInputStreamWrapper *>(
                        descs[j]->mInputWrappers[k]));
            }
        }
    }

    uint32_t i;
    for (i = 0 ; i < inputs.Length() ; i++)
        inputs[i]->Close();

    for (i = 0 ; i < outputs.Length() ; i++)
        outputs[i]->Close();
}


bool
nsCacheService::GetClearingEntries()
{
    AssertOwnsLock();
    return gService->mClearingEntries;
}


#if defined(PR_LOGGING)
void
nsCacheService::LogCacheStatistics()
{
    uint32_t hitPercentage = (uint32_t)((((double)mCacheHits) /
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

nsresult
nsCacheService::SetDiskSmartSize()
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_SETDISKSMARTSIZE));

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
            new nsGetSmartSizeEvent(cachePath, mDiskDevice->getCacheSize(),
                                    mObserver->ShouldUseOldMaxSmartSize());
        DispatchToCacheIOThread(event);
    } else {
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

void
nsCacheService::MoveOrRemoveDiskCache(nsIFile *aOldCacheDir, 
                                      nsIFile *aNewCacheDir,
                                      const char *aCacheSubdir)
{
    bool same;
    if (NS_FAILED(aOldCacheDir->Equals(aNewCacheDir, &same)) || same)
        return;

    nsCOMPtr<nsIFile> aOldCacheSubdir;
    aOldCacheDir->Clone(getter_AddRefs(aOldCacheSubdir));

    nsresult rv = aOldCacheSubdir->AppendNative(
        nsDependentCString(aCacheSubdir));
    if (NS_FAILED(rv))
        return;

    bool exists;
    if (NS_FAILED(aOldCacheSubdir->Exists(&exists)) || !exists)
        return;

    nsCOMPtr<nsIFile> aNewCacheSubdir;
    aNewCacheDir->Clone(getter_AddRefs(aNewCacheSubdir));

    rv = aNewCacheSubdir->AppendNative(nsDependentCString(aCacheSubdir));
    if (NS_FAILED(rv))
        return;
    
    nsAutoCString newPath;
    rv = aNewCacheSubdir->GetNativePath(newPath);
    if (NS_FAILED(rv))
        return;
        
    if (NS_SUCCEEDED(aNewCacheSubdir->Exists(&exists)) && !exists) {
        
        
        rv = aNewCacheSubdir->Create(nsIFile::DIRECTORY_TYPE, 0777); 
        if (NS_SUCCEEDED(rv)) {
            nsAutoCString oldPath;
            rv = aOldCacheSubdir->GetNativePath(oldPath);
            if (NS_FAILED(rv))
                return;
            if(rename(oldPath.get(), newPath.get()) == 0)
                return;
        }
    }
    
    
    nsDeleteDir::DeleteDir(aOldCacheSubdir, false, 60000);
}

static bool
IsEntryPrivate(nsCacheEntry* entry)
{
    return entry->IsPrivate();
}

void
nsCacheService::LeavePrivateBrowsing()
{
    nsCacheServiceAutoLock lock(LOCK_TELEM(NSCACHESERVICE_LEAVEPRIVATEBROWSING));

    gService->DoomActiveEntries(IsEntryPrivate);

    if (gService->mMemoryDevice) {
        
        gService->mMemoryDevice->EvictPrivateEntries();
    }
}
