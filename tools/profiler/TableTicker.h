




#ifndef TableTicker_h
#define TableTicker_h

#include "platform.h"
#include "ProfileEntry.h"
#include "mozilla/Mutex.h"
#include "IntelPowerGadget.h"
#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
#endif

static bool
hasFeature(const char** aFeatures, uint32_t aFeatureCount, const char* aFeature) {
  for(size_t i = 0; i < aFeatureCount; i++) {
    if (strcmp(aFeatures[i], aFeature) == 0)
      return true;
  }
  return false;
}

static bool
threadSelected(ThreadInfo* aInfo, char** aThreadNameFilters, uint32_t aFeatureCount) {
  if (aFeatureCount == 0) {
    return true;
  }

  for (uint32_t i = 0; i < aFeatureCount; ++i) {
    const char* filterPrefix = aThreadNameFilters[i];
    if (strncmp(aInfo->Name(), filterPrefix, strlen(filterPrefix)) == 0) {
      return true;
    }
  }

  return false;
}

extern mozilla::TimeStamp sLastTracerEvent;
extern int sFrameNumber;
extern int sLastFrameNumber;
extern unsigned int sCurrentEventGeneration;
extern unsigned int sLastSampledEventGeneration;

class BreakpadSampler;

class TableTicker: public Sampler {
 public:
  TableTicker(double aInterval, int aEntrySize,
              const char** aFeatures, uint32_t aFeatureCount,
              const char** aThreadNameFilters, uint32_t aFilterCount)
    : Sampler(aInterval, true, aEntrySize)
    , mPrimaryThreadProfile(nullptr)
    , mSaveRequested(false)
    , mUnwinderThread(false)
    , mFilterCount(aFilterCount)
#if defined(XP_WIN)
    , mIntelPowerGadget(nullptr)
#endif
  {
    mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");

    
    mJankOnly = hasFeature(aFeatures, aFeatureCount, "jank");
    mProfileJS = hasFeature(aFeatures, aFeatureCount, "js");
    mProfileJava = hasFeature(aFeatures, aFeatureCount, "java");
    mProfileGPU = hasFeature(aFeatures, aFeatureCount, "gpu");
    mProfilePower = hasFeature(aFeatures, aFeatureCount, "power");
    
    
    mProfileThreads = hasFeature(aFeatures, aFeatureCount, "threads") || aFilterCount > 0;
    mUnwinderThread = hasFeature(aFeatures, aFeatureCount, "unwinder") || sps_version2();
    mAddLeafAddresses = hasFeature(aFeatures, aFeatureCount, "leaf");
    mPrivacyMode = hasFeature(aFeatures, aFeatureCount, "privacy");
    mAddMainThreadIO = hasFeature(aFeatures, aFeatureCount, "mainthreadio");
    mProfileMemory = hasFeature(aFeatures, aFeatureCount, "memory");
    mTaskTracer = hasFeature(aFeatures, aFeatureCount, "tasktracer");
    mLayersDump = hasFeature(aFeatures, aFeatureCount, "layersdump");

#if defined(XP_WIN)
    if (mProfilePower) {
      mIntelPowerGadget = new IntelPowerGadget();
      mProfilePower = mIntelPowerGadget->Init();
    }
#endif

    
    mThreadNameFilters = new char*[aFilterCount];
    for (uint32_t i = 0; i < aFilterCount; ++i) {
      mThreadNameFilters[i] = strdup(aThreadNameFilters[i]);
    }

    sStartTime = mozilla::TimeStamp::Now();

    {
      mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

      
      for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
        ThreadInfo* info = sRegisteredThreads->at(i);

        RegisterThread(info);
      }

      SetActiveSampler(this);
    }

#ifdef MOZ_TASK_TRACER
    if (mTaskTracer) {
      mozilla::tasktracer::StartLogging(sStartTime);
    }
#endif
  }

  ~TableTicker() {
    if (IsActive())
      Stop();

    SetActiveSampler(nullptr);

    
    {
      mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

      for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
        ThreadInfo* info = sRegisteredThreads->at(i);
        ThreadProfile* profile = info->Profile();
        if (profile) {
          delete profile;
          info->SetProfile(nullptr);
        }
        
        
        if (info->IsPendingDelete()) {
          delete info;
          sRegisteredThreads->erase(sRegisteredThreads->begin() + i);
          i--;
        }
      }
    }
#if defined(XP_WIN)
    delete mIntelPowerGadget;
#endif
  }

  void RegisterThread(ThreadInfo* aInfo) {
    if (!aInfo->IsMainThread() && !mProfileThreads) {
      return;
    }

    if (!threadSelected(aInfo, mThreadNameFilters, mFilterCount)) {
      return;
    }

    ThreadProfile* profile = new ThreadProfile(aInfo, EntrySize());
    aInfo->SetProfile(profile);
  }

  
  virtual void Tick(TickSample* sample);

  
  virtual SyncProfile* GetBacktrace();

  
  virtual void RequestSave()
  {
    mSaveRequested = true;
#ifdef MOZ_TASK_TRACER
    if (mTaskTracer) {
      mozilla::tasktracer::StopLogging();
    }
#endif
  }

  virtual void HandleSaveRequest();

  ThreadProfile* GetPrimaryThreadProfile()
  {
    if (!mPrimaryThreadProfile) {
      mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

      for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
        ThreadInfo* info = sRegisteredThreads->at(i);
        if (info->IsMainThread() && !info->IsPendingDelete()) {
          mPrimaryThreadProfile = info->Profile();
          break;
        }
      }
    }

    return mPrimaryThreadProfile;
  }

  void ToStreamAsJSON(std::ostream& stream);
  virtual JSObject *ToJSObject(JSContext *aCx);
  void StreamMetaJSCustomObject(JSStreamWriter& b);
  void StreamTaskTracer(JSStreamWriter& b);
  bool HasUnwinderThread() const { return mUnwinderThread; }
  bool ProfileJS() const { return mProfileJS; }
  bool ProfileJava() const { return mProfileJava; }
  bool ProfileGPU() const { return mProfileGPU; }
  bool ProfilePower() const { return mProfilePower; }
  bool ProfileThreads() const { return mProfileThreads; }
  bool InPrivacyMode() const { return mPrivacyMode; }
  bool AddMainThreadIO() const { return mAddMainThreadIO; }
  bool ProfileMemory() const { return mProfileMemory; }
  bool TaskTracer() const { return mTaskTracer; }
  bool LayersDump() const { return mLayersDump; }

protected:
  
  virtual void UnwinderTick(TickSample* sample);

  
  virtual void InplaceTick(TickSample* sample);

  
  void doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample);

  void StreamJSObject(JSStreamWriter& b);

  
  ThreadProfile* mPrimaryThreadProfile;
  bool mSaveRequested;
  bool mAddLeafAddresses;
  bool mUseStackWalk;
  bool mJankOnly;
  bool mProfileJS;
  bool mProfileGPU;
  bool mProfileThreads;
  bool mUnwinderThread;
  bool mProfileJava;
  bool mProfilePower;
  bool mLayersDump;

  
  
  char** mThreadNameFilters;
  uint32_t mFilterCount;
  bool mPrivacyMode;
  bool mAddMainThreadIO;
  bool mProfileMemory;
  bool mTaskTracer;
#if defined(XP_WIN)
  IntelPowerGadget* mIntelPowerGadget;
#endif
};

#endif

