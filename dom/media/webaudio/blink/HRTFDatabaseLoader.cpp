



























#include "HRTFDatabaseLoader.h"
#include "HRTFDatabase.h"

using namespace mozilla;

namespace WebCore {


nsTHashtable<HRTFDatabaseLoader::LoaderByRateEntry>*
    HRTFDatabaseLoader::s_loaderMap = nullptr;

size_t HRTFDatabaseLoader::sizeOfLoaders(mozilla::MallocSizeOf aMallocSizeOf)
{
    return s_loaderMap ? s_loaderMap->SizeOfIncludingThis(aMallocSizeOf) : 0;
}

already_AddRefed<HRTFDatabaseLoader> HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(float sampleRate)
{
    MOZ_ASSERT(NS_IsMainThread());

    RefPtr<HRTFDatabaseLoader> loader;
    
    if (!s_loaderMap) {
        s_loaderMap = new nsTHashtable<LoaderByRateEntry>();
    }

    LoaderByRateEntry* entry = s_loaderMap->PutEntry(sampleRate);
    loader = entry->mLoader;
    if (loader) { 
        MOZ_ASSERT(sampleRate == loader->databaseSampleRate());
        return loader.forget();
    }

    loader = new HRTFDatabaseLoader(sampleRate);
    entry->mLoader = loader;

    loader->loadAsynchronously();

    return loader.forget();
}

HRTFDatabaseLoader::HRTFDatabaseLoader(float sampleRate)
    : m_refCnt(0)
    , m_threadLock("HRTFDatabaseLoader")
    , m_databaseLoaderThread(nullptr)
    , m_databaseSampleRate(sampleRate)
{
    MOZ_ASSERT(NS_IsMainThread());
}

HRTFDatabaseLoader::~HRTFDatabaseLoader()
{
    MOZ_ASSERT(NS_IsMainThread());

    waitForLoaderThreadCompletion();
    m_hrtfDatabase.reset();

    if (s_loaderMap) {
        
        s_loaderMap->RemoveEntry(m_databaseSampleRate);
        if (s_loaderMap->Count() == 0) {
            delete s_loaderMap;
            s_loaderMap = nullptr;
        }
    }
}

size_t HRTFDatabaseLoader::sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = aMallocSizeOf(this);

    
    const_cast<HRTFDatabaseLoader*>(this)->waitForLoaderThreadCompletion();

    if (m_hrtfDatabase) {
        amount += m_hrtfDatabase->sizeOfIncludingThis(aMallocSizeOf);
    }

    return amount;
}

class HRTFDatabaseLoader::ProxyReleaseEvent final : public nsRunnable {
public:
    explicit ProxyReleaseEvent(HRTFDatabaseLoader* loader) : mLoader(loader) {}
    NS_IMETHOD Run() override
    {
        mLoader->MainThreadRelease();
        return NS_OK;
    }
private:
    HRTFDatabaseLoader* mLoader;
};

void HRTFDatabaseLoader::ProxyRelease()
{
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    if (MOZ_LIKELY(mainThread)) {
        nsRefPtr<ProxyReleaseEvent> event = new ProxyReleaseEvent(this);
        DebugOnly<nsresult> rv =
            mainThread->Dispatch(event, NS_DISPATCH_NORMAL);
        MOZ_ASSERT(NS_SUCCEEDED(rv), "Failed to dispatch release event");
    } else {
        
        MOZ_ASSERT(NS_IsMainThread(),
                   "Main thread is not available for dispatch.");
        MainThreadRelease();
    }
}

void HRTFDatabaseLoader::MainThreadRelease()
{
    MOZ_ASSERT(NS_IsMainThread());
    int count = --m_refCnt;
    MOZ_ASSERT(count >= 0, "extra release");
    NS_LOG_RELEASE(this, count, "HRTFDatabaseLoader");
    if (count == 0) {
        
        
        delete this;
    }
}


static void databaseLoaderEntry(void* threadData)
{
    PR_SetCurrentThreadName("HRTFDatabaseLdr");

    HRTFDatabaseLoader* loader = reinterpret_cast<HRTFDatabaseLoader*>(threadData);
    MOZ_ASSERT(loader);
    loader->load();
}

void HRTFDatabaseLoader::load()
{
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(!m_hrtfDatabase.get(), "Called twice");
    
    m_hrtfDatabase = HRTFDatabase::create(m_databaseSampleRate);
    
    Release();
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(m_refCnt, "Must not be called before a reference is added");

    
    
    AddRef();

    MutexAutoLock locker(m_threadLock);
    
    MOZ_ASSERT(!m_hrtfDatabase.get() && !m_databaseLoaderThread,
               "Called twice");
    
    m_databaseLoaderThread =
        PR_CreateThread(PR_USER_THREAD, databaseLoaderEntry, this,
                        PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                        PR_JOINABLE_THREAD, 0);
}

bool HRTFDatabaseLoader::isLoaded() const
{
    return m_hrtfDatabase.get();
}

void HRTFDatabaseLoader::waitForLoaderThreadCompletion()
{
    MutexAutoLock locker(m_threadLock);
    
    
    if (m_databaseLoaderThread) {
        DebugOnly<PRStatus> status = PR_JoinThread(m_databaseLoaderThread);
        MOZ_ASSERT(status == PR_SUCCESS, "PR_JoinThread failed");
    }
    m_databaseLoaderThread = nullptr;
}

PLDHashOperator
HRTFDatabaseLoader::shutdownEnumFunc(LoaderByRateEntry *entry, void* unused)
{
    
    entry->mLoader->waitForLoaderThreadCompletion();
    return PLDHashOperator::PL_DHASH_NEXT;
}

void HRTFDatabaseLoader::shutdown()
{
    MOZ_ASSERT(NS_IsMainThread());
    if (s_loaderMap) {
        
        
        nsTHashtable<LoaderByRateEntry>* loaderMap = s_loaderMap;
        s_loaderMap = nullptr;
        loaderMap->EnumerateEntries(shutdownEnumFunc, nullptr);
        delete loaderMap;
    }
}
} 
