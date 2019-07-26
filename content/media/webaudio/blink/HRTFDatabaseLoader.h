



























#ifndef HRTFDatabaseLoader_h
#define HRTFDatabaseLoader_h

#include "core/platform/audio/HRTFDatabase.h"
#include "wtf/HashMap.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Threading.h"

namespace WebCore {



class HRTFDatabaseLoader : public RefCounted<HRTFDatabaseLoader> {
public:
    
    
    
    
    static PassRefPtr<HRTFDatabaseLoader> createAndLoadAsynchronouslyIfNecessary(float sampleRate);

    
    ~HRTFDatabaseLoader();
    
    
    bool isLoaded() const;

    
    void waitForLoaderThreadCompletion();
    
    HRTFDatabase* database() { return m_hrtfDatabase.get(); }

    float databaseSampleRate() const { return m_databaseSampleRate; }
    
    
    void load();

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    
    explicit HRTFDatabaseLoader(float sampleRate);
    
    
    
    void loadAsynchronously();

    
    typedef HashMap<double, HRTFDatabaseLoader*> LoaderMap;

    
    static LoaderMap* s_loaderMap; 

    OwnPtr<HRTFDatabase> m_hrtfDatabase;

    
    Mutex m_threadLock;
    ThreadIdentifier m_databaseLoaderThread;

    float m_databaseSampleRate;
};

} 

#endif 
