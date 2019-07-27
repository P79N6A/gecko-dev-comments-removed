



























#ifndef HRTFDatabaseLoader_h
#define HRTFDatabaseLoader_h

#include "nsHashKeys.h"
#include "mozilla/RefPtr.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "HRTFDatabase.h"

template <class EntryType> class nsTHashtable;
template <class T> class nsAutoRef;

namespace WebCore {



class HRTFDatabaseLoader {
public:
    
    
    
    
    static mozilla::TemporaryRef<HRTFDatabaseLoader> createAndLoadAsynchronouslyIfNecessary(float sampleRate);

    
    void AddRef()
    {
#if defined(DEBUG) || defined(NS_BUILD_REFCNT_LOGGING)
        int count =
#endif
          ++m_refCnt;
        MOZ_ASSERT(count > 0, "invalid ref count");
        NS_LOG_ADDREF(this, count, "HRTFDatabaseLoader", sizeof(*this));
    }

    void Release()
    {
        
        
        
        int count = m_refCnt;
        MOZ_ASSERT(count > 0, "extra release");
        
        
        if (count != 1 && m_refCnt.compareExchange(count, count - 1)) {
            NS_LOG_RELEASE(this, count - 1, "HRTFDatabaseLoader");
            return;
        }

        ProxyRelease();
    }

    
    bool isLoaded() const;

    
    
    void waitForLoaderThreadCompletion();
    
    HRTFDatabase* database() { return m_hrtfDatabase.get(); }

    float databaseSampleRate() const { return m_databaseSampleRate; }

    static void shutdown();
    
    
    void load();

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    
    explicit HRTFDatabaseLoader(float sampleRate);
    ~HRTFDatabaseLoader();
    
    void ProxyRelease(); 
    void MainThreadRelease(); 
    class ProxyReleaseEvent;

    
    
    void loadAsynchronously();

    
    class LoaderByRateEntry : public nsFloatHashKey {
    public:
        explicit LoaderByRateEntry(KeyTypePointer aKey)
            : nsFloatHashKey(aKey)
            , mLoader() 
        {
        }
        HRTFDatabaseLoader* mLoader;
    };

    static PLDHashOperator shutdownEnumFunc(LoaderByRateEntry *entry,
                                            void* unused);

    
    static nsTHashtable<LoaderByRateEntry> *s_loaderMap; 

    mozilla::Atomic<int> m_refCnt;

    nsAutoRef<HRTFDatabase> m_hrtfDatabase;

    
    mozilla::Mutex m_threadLock;
    PRThread* m_databaseLoaderThread;

    float m_databaseSampleRate;
};

} 

#endif
