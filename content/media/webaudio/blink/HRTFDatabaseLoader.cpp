



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/HRTFDatabaseLoader.h"

#include "core/platform/PlatformMemoryInstrumentation.h"
#include "core/platform/audio/HRTFDatabase.h"
#include "wtf/MainThread.h"
#include "wtf/MemoryInstrumentationHashMap.h"

namespace WebCore {


HRTFDatabaseLoader::LoaderMap* HRTFDatabaseLoader::s_loaderMap = 0;

PassRefPtr<HRTFDatabaseLoader> HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(float sampleRate)
{
    ASSERT(isMainThread());

    RefPtr<HRTFDatabaseLoader> loader;
    
    if (!s_loaderMap)
        s_loaderMap = adoptPtr(new LoaderMap()).leakPtr();

    loader = s_loaderMap->get(sampleRate);
    if (loader) {
        ASSERT(sampleRate == loader->databaseSampleRate());
        return loader;
    }

    loader = adoptRef(new HRTFDatabaseLoader(sampleRate));
    s_loaderMap->add(sampleRate, loader.get());

    loader->loadAsynchronously();

    return loader;
}

HRTFDatabaseLoader::HRTFDatabaseLoader(float sampleRate)
    : m_databaseLoaderThread(0)
    , m_databaseSampleRate(sampleRate)
{
    ASSERT(isMainThread());
}

HRTFDatabaseLoader::~HRTFDatabaseLoader()
{
    ASSERT(isMainThread());

    waitForLoaderThreadCompletion();
    m_hrtfDatabase.clear();

    
    if (s_loaderMap)
        s_loaderMap->remove(m_databaseSampleRate);
}


static void databaseLoaderEntry(void* threadData)
{
    HRTFDatabaseLoader* loader = reinterpret_cast<HRTFDatabaseLoader*>(threadData);
    ASSERT(loader);
    loader->load();
}

void HRTFDatabaseLoader::load()
{
    ASSERT(!isMainThread());
    if (!m_hrtfDatabase.get()) {
        
        m_hrtfDatabase = HRTFDatabase::create(m_databaseSampleRate);
    }
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    ASSERT(isMainThread());

    MutexLocker locker(m_threadLock);
    
    if (!m_hrtfDatabase.get() && !m_databaseLoaderThread) {
        
        m_databaseLoaderThread = createThread(databaseLoaderEntry, this, "HRTF database loader");
    }
}

bool HRTFDatabaseLoader::isLoaded() const
{
    return m_hrtfDatabase.get();
}

void HRTFDatabaseLoader::waitForLoaderThreadCompletion()
{
    MutexLocker locker(m_threadLock);
    
    
    if (m_databaseLoaderThread)
        waitForThreadCompletion(m_databaseLoaderThread);
    m_databaseLoaderThread = 0;
}

void HRTFDatabaseLoader::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, PlatformMemoryTypes::AudioSharedData);
    info.addMember(m_hrtfDatabase, "hrtfDatabase");
    info.addMember(s_loaderMap, "loaderMap", WTF::RetainingPointer);
}

} 

#endif 
