



























#ifndef HRTFDatabaseLoader_h
#define HRTFDatabaseLoader_h

#include "HRTFDatabase.h"
#include "nsTHashtable.h"
#include "mozilla/RefPtr.h"
#include "nsIThread.h"
#include "mozilla/Mutex.h"

namespace WebCore {



class HRTFDatabaseLoader : public mozilla::RefCounted<HRTFDatabaseLoader> {
public:
    
    
    
    
    static mozilla::TemporaryRef<HRTFDatabaseLoader> createAndLoadAsynchronouslyIfNecessary(float sampleRate);

    
    ~HRTFDatabaseLoader();
    
    
    bool isLoaded() const;

    
    
    void waitForLoaderThreadCompletion();
    
    HRTFDatabase* database() { return m_hrtfDatabase.get(); }

    float databaseSampleRate() const { return m_databaseSampleRate; }
    
    
    void load();

private:
    
    explicit HRTFDatabaseLoader(float sampleRate);
    
    
    
    void loadAsynchronously();

    
    class LoaderByRateEntry : public nsFloatHashKey {
    public:
        LoaderByRateEntry(KeyTypePointer aKey)
            : nsFloatHashKey(aKey)
            , mLoader() 
        {
        }
        HRTFDatabaseLoader* mLoader;
    };
    
    static nsTHashtable<LoaderByRateEntry> *s_loaderMap; 

    nsAutoRef<HRTFDatabase> m_hrtfDatabase;

    
    mozilla::Mutex m_threadLock;
    PRThread* m_databaseLoaderThread;

    float m_databaseSampleRate;
};

} 

#endif 
