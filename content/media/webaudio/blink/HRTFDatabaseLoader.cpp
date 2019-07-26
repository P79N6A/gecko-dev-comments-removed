



























#include "HRTFDatabaseLoader.h"

#include "HRTFDatabase.h"

using namespace mozilla;

namespace WebCore {


nsTHashtable<HRTFDatabaseLoader::LoaderByRateEntry>*
    HRTFDatabaseLoader::s_loaderMap = nullptr;

TemporaryRef<HRTFDatabaseLoader> HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(float sampleRate)
{
    MOZ_ASSERT(NS_IsMainThread());

    RefPtr<HRTFDatabaseLoader> loader;
    
    if (!s_loaderMap) {
        s_loaderMap = new nsTHashtable<LoaderByRateEntry>();
        s_loaderMap->Init();
    }

    LoaderByRateEntry* entry = s_loaderMap->PutEntry(sampleRate);
    loader = entry->mLoader;
    if (loader) { 
        MOZ_ASSERT(sampleRate == loader->databaseSampleRate());
        return loader;
    }

    loader = new HRTFDatabaseLoader(sampleRate);
    entry->mLoader = loader;

    loader->loadAsynchronously();

    return loader;
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

    
    s_loaderMap->RemoveEntry(m_databaseSampleRate);
    if (s_loaderMap->Count() == 0) {
        delete s_loaderMap;
        s_loaderMap = nullptr;
    }
}

class HRTFDatabaseLoader::ProxyReleaseEvent MOZ_FINAL : public nsRunnable {
public:
    explicit ProxyReleaseEvent(HRTFDatabaseLoader* loader) : mLoader(loader) {}
    NS_IMETHOD Run() MOZ_OVERRIDE
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
    if (!m_hrtfDatabase.get()) {
        
        m_hrtfDatabase = HRTFDatabase::create(m_databaseSampleRate);
    }
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    MOZ_ASSERT(NS_IsMainThread());

    MutexAutoLock locker(m_threadLock);
    
    if (!m_hrtfDatabase.get() && !m_databaseLoaderThread) {
        
        m_databaseLoaderThread =
            PR_CreateThread(PR_USER_THREAD, databaseLoaderEntry, this,
                            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                            PR_JOINABLE_THREAD, 0);
    }
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

} 
