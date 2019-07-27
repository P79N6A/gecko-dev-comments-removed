




#ifndef _LOADMANAGER_H_
#define _LOADMANAGER_H_

#include "LoadMonitor.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Services.h"
#include "nsTArray.h"
#include "nsIObserver.h"

#include "webrtc/common_types.h"
#include "webrtc/video_engine/include/vie_base.h"

extern PRLogModuleInfo *gLoadManagerLog;

namespace mozilla {

class LoadManagerSingleton : public LoadNotificationCallback,
                             public webrtc::CPULoadStateCallbackInvoker,
                             public webrtc::CpuOveruseObserver,
                             public nsIObserver

{
public:
    static LoadManagerSingleton* Get();

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIOBSERVER

    
    virtual void LoadChanged(float aSystemLoad, float aProcessLoad) override;
    
    
    virtual void OveruseDetected() override;
    
    virtual void NormalUsage() override;
    
    virtual void AddObserver(webrtc::CPULoadStateObserver * aObserver) override;
    virtual void RemoveObserver(webrtc::CPULoadStateObserver * aObserver) override;

private:
    LoadManagerSingleton(int aLoadMeasurementInterval,
                         int aAveragingMeasurements,
                         float aHighLoadThreshold,
                         float aLowLoadThreshold);
    ~LoadManagerSingleton();

    void LoadHasChanged();

    nsRefPtr<LoadMonitor> mLoadMonitor;

    
    
    Mutex mLock;
    nsTArray<webrtc::CPULoadStateObserver*> mObservers;
    webrtc::CPULoadState mCurrentState;
    
    bool  mOveruseActive;
    float mLoadSum;
    int   mLoadSumMeasurements;
    
    int mLoadMeasurementInterval;
    int mAveragingMeasurements;
    float mHighLoadThreshold;
    float mLowLoadThreshold;

    static StaticRefPtr<LoadManagerSingleton> sSingleton;
};

class LoadManager final : public webrtc::CPULoadStateCallbackInvoker,
                              public webrtc::CpuOveruseObserver
{
public:
    explicit LoadManager(LoadManagerSingleton* aManager)
        : mManager(aManager)
    {}
    ~LoadManager() {}

    void AddObserver(webrtc::CPULoadStateObserver * aObserver) override
    {
        mManager->AddObserver(aObserver);
    }
    void RemoveObserver(webrtc::CPULoadStateObserver * aObserver) override
    {
        mManager->RemoveObserver(aObserver);
    }
    void OveruseDetected() override
    {
        mManager->OveruseDetected();
    }
    void NormalUsage() override
    {
        mManager->NormalUsage();
    }

private:
    LoadManagerSingleton* mManager;
};

} 

#endif 
