




#ifndef _LOADMANAGER_H_
#define _LOADMANAGER_H_

#include "LoadMonitor.h"
#include "webrtc/common_types.h"
#include "webrtc/video_engine/include/vie_base.h"
#include "mozilla/TimeStamp.h"
#include "nsTArray.h"

extern PRLogModuleInfo *gLoadManagerLog;

namespace mozilla {

class LoadManager : public LoadNotificationCallback,
                    public webrtc::CPULoadStateCallbackInvoker,
                    public webrtc::CpuOveruseObserver
{
public:
    LoadManager(int aLoadMeasurementInterval,
                int aAveragingMeasurements,
                float aHighLoadThreshold,
                float aLowLoadThreshold);
    ~LoadManager();

    
    virtual void LoadChanged(float aSystemLoad, float aProcessLoad) MOZ_OVERRIDE;
    
    
    virtual void OveruseDetected() MOZ_OVERRIDE;
    
    virtual void NormalUsage() MOZ_OVERRIDE;
    
    virtual void AddObserver(webrtc::CPULoadStateObserver * aObserver) MOZ_OVERRIDE;
    virtual void RemoveObserver(webrtc::CPULoadStateObserver * aObserver) MOZ_OVERRIDE;

private:
    void LoadHasChanged();

    nsRefPtr<LoadMonitor> mLoadMonitor;
    float mLastSystemLoad;
    float mLoadSum;
    int   mLoadSumMeasurements;
    
    bool  mOveruseActive;
    
    int mLoadMeasurementInterval;
    int mAveragingMeasurements;
    float mHighLoadThreshold;
    float mLowLoadThreshold;

    webrtc::CPULoadState mCurrentState;

    nsTArray<webrtc::CPULoadStateObserver*> mObservers;
};

} 

#endif 
