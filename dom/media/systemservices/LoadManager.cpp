




#include "LoadManager.h"
#include "LoadMonitor.h"
#include "nsString.h"
#include "mozilla/Logging.h"
#include "prtime.h"
#include "prinrval.h"
#include "prsystem.h"

#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsReadableUtils.h"
#include "nsIObserverService.h"


PRLogModuleInfo *gLoadManagerLog = nullptr;
#undef LOG
#undef LOG_ENABLED
#define LOG(args) MOZ_LOG(gLoadManagerLog, mozilla::LogLevel::Debug, args)
#define LOG_ENABLED() MOZ_LOG_TEST(gLoadManagerLog, mozilla::LogLevel::Verbose)

namespace mozilla {

 StaticRefPtr<LoadManagerSingleton> LoadManagerSingleton::sSingleton;

NS_IMPL_ISUPPORTS(LoadManagerSingleton, nsIObserver)


LoadManagerSingleton::LoadManagerSingleton(int aLoadMeasurementInterval,
                                           int aAveragingMeasurements,
                                           float aHighLoadThreshold,
                                           float aLowLoadThreshold)
  : mLock("LoadManager"),
    mCurrentState(webrtc::kLoadNormal),
    mOveruseActive(false),
    mLoadSum(0.0f),
    mLoadSumMeasurements(0),
    mLoadMeasurementInterval(aLoadMeasurementInterval),
    mAveragingMeasurements(aAveragingMeasurements),
    mHighLoadThreshold(aHighLoadThreshold),
    mLowLoadThreshold(aLowLoadThreshold)
{
  if (!gLoadManagerLog)
    gLoadManagerLog = PR_NewLogModule("LoadManager");
  LOG(("LoadManager - Initializing (%dms x %d, %f, %f)",
       mLoadMeasurementInterval, mAveragingMeasurements,
       mHighLoadThreshold, mLowLoadThreshold));
  MOZ_ASSERT(mHighLoadThreshold > mLowLoadThreshold);
  mLoadMonitor = new LoadMonitor(mLoadMeasurementInterval);
  mLoadMonitor->Init(mLoadMonitor);
  mLoadMonitor->SetLoadChangeCallback(this);
}

LoadManagerSingleton::~LoadManagerSingleton()
{
  LOG(("LoadManager: shutting down LoadMonitor"));
  MOZ_ASSERT(!mLoadMonitor, "why wasn't the LoadMonitor shut down in xpcom-shutdown?");
  if (mLoadMonitor) {
    mLoadMonitor->Shutdown();
  }
}

nsresult
LoadManagerSingleton::Observe(nsISupports* aSubject, const char* aTopic,
                     const char16_t* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Observer invoked off the main thread");
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();

  if (!strcmp(aTopic, "xpcom-shutdown")) {
    obs->RemoveObserver(this, "xpcom-shutdown");
    {
      MutexAutoLock lock(mLock);
      mObservers.Clear();
    }
    if (mLoadMonitor) {
      mLoadMonitor->Shutdown();
      mLoadMonitor = nullptr;
    }

    LOG(("Releasing LoadManager singleton and thread"));
    
    sSingleton = nullptr;
  }
  return NS_OK;
}

void
LoadManagerSingleton::LoadChanged(float aSystemLoad, float aProcesLoad)
{
  MutexAutoLock lock(mLock);
  
  mLoadSum += aSystemLoad;
  mLoadSumMeasurements++;

  if (mLoadSumMeasurements >= mAveragingMeasurements) {
    double averagedLoad = mLoadSum / (float)mLoadSumMeasurements;

    webrtc::CPULoadState oldState = mCurrentState;

    if (mOveruseActive || averagedLoad > mHighLoadThreshold) {
      LOG(("LoadManager - LoadStressed"));
      mCurrentState = webrtc::kLoadStressed;
    } else if (averagedLoad < mLowLoadThreshold) {
      LOG(("LoadManager - LoadRelaxed"));
      mCurrentState = webrtc::kLoadRelaxed;
    } else {
      LOG(("LoadManager - LoadNormal"));
      mCurrentState = webrtc::kLoadNormal;
    }

    if (oldState != mCurrentState)
      LoadHasChanged();

    mLoadSum = 0;
    mLoadSumMeasurements = 0;
  }
}

void
LoadManagerSingleton::OveruseDetected()
{
  LOG(("LoadManager - Overuse Detected"));
  MutexAutoLock lock(mLock);
  mOveruseActive = true;
  if (mCurrentState != webrtc::kLoadStressed) {
    mCurrentState = webrtc::kLoadStressed;
    LoadHasChanged();
  }
}

void
LoadManagerSingleton::NormalUsage()
{
  LOG(("LoadManager - Overuse finished"));
  MutexAutoLock lock(mLock);
  mOveruseActive = false;
}

void
LoadManagerSingleton::LoadHasChanged()
{
  mLock.AssertCurrentThreadOwns();
  LOG(("LoadManager - Signaling LoadHasChanged to %d listeners", mObservers.Length()));
  for (size_t i = 0; i < mObservers.Length(); i++) {
    mObservers.ElementAt(i)->onLoadStateChanged(mCurrentState);
  }
}

void
LoadManagerSingleton::AddObserver(webrtc::CPULoadStateObserver * aObserver)
{
  LOG(("LoadManager - Adding Observer"));
  MutexAutoLock lock(mLock);
  mObservers.AppendElement(aObserver);
  if (mObservers.Length() == 1) {
    if (!mLoadMonitor) {
      mLoadMonitor = new LoadMonitor(mLoadMeasurementInterval);
      mLoadMonitor->Init(mLoadMonitor);
      mLoadMonitor->SetLoadChangeCallback(this);
    }
  }
}

void
LoadManagerSingleton::RemoveObserver(webrtc::CPULoadStateObserver * aObserver)
{
  LOG(("LoadManager - Removing Observer"));
  MutexAutoLock lock(mLock);
  if (!mObservers.RemoveElement(aObserver)) {
    LOG(("LoadManager - Element to remove not found"));
  }
  if (mObservers.Length() == 0) {
    if (mLoadMonitor) {
      
      nsRefPtr<LoadMonitor> loadMonitor = mLoadMonitor.forget();
      MutexAutoUnlock unlock(mLock);

      loadMonitor->Shutdown();
    }
  }
}


}
