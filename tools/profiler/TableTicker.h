




#ifndef TableTicker_h
#define TableTicker_h

#include "platform.h"
#include "ProfileEntry.h"
#include "mozilla/Mutex.h"
#include "mozilla/Vector.h"
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

typedef mozilla::Vector<std::string> ThreadNameFilterList;

static bool
threadSelected(ThreadInfo* aInfo, const ThreadNameFilterList &aThreadNameFilters) {
  if (aThreadNameFilters.empty()) {
    return true;
  }

  for (uint32_t i = 0; i < aThreadNameFilters.length(); ++i) {
    if (aThreadNameFilters[i] == aInfo->Name()) {
      return true;
    }
  }

  return false;
}

extern mozilla::TimeStamp sLastTracerEvent;
extern int sFrameNumber;
extern int sLastFrameNumber;

class TableTicker: public Sampler {
 public:
  TableTicker(double aInterval, int aEntrySize,
              const char** aFeatures, uint32_t aFeatureCount,
              const char** aThreadNameFilters, uint32_t aFilterCount)
    : Sampler(aInterval, true, aEntrySize)
    , mPrimaryThreadProfile(nullptr)
    , mBuffer(new ProfileBuffer(aEntrySize))
    , mSaveRequested(false)
#if defined(XP_WIN)
    , mIntelPowerGadget(nullptr)
#endif
  {
    mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");

    mProfileJS = hasFeature(aFeatures, aFeatureCount, "js");
    mProfileJava = hasFeature(aFeatures, aFeatureCount, "java");
    mProfileGPU = hasFeature(aFeatures, aFeatureCount, "gpu");
    mProfilePower = hasFeature(aFeatures, aFeatureCount, "power");
    
    
    mProfileThreads = hasFeature(aFeatures, aFeatureCount, "threads") || aFilterCount > 0;
    mAddLeafAddresses = hasFeature(aFeatures, aFeatureCount, "leaf");
    mPrivacyMode = hasFeature(aFeatures, aFeatureCount, "privacy");
    mAddMainThreadIO = hasFeature(aFeatures, aFeatureCount, "mainthreadio");
    mProfileMemory = hasFeature(aFeatures, aFeatureCount, "memory");
    mTaskTracer = hasFeature(aFeatures, aFeatureCount, "tasktracer");
    mLayersDump = hasFeature(aFeatures, aFeatureCount, "layersdump");
    mDisplayListDump = hasFeature(aFeatures, aFeatureCount, "displaylistdump");
    mProfileRestyle = hasFeature(aFeatures, aFeatureCount, "restyle");

#if defined(XP_WIN)
    if (mProfilePower) {
      mIntelPowerGadget = new IntelPowerGadget();
      mProfilePower = mIntelPowerGadget->Init();
    }
#endif

    
    MOZ_ALWAYS_TRUE(mThreadNameFilters.resize(aFilterCount));
    for (uint32_t i = 0; i < aFilterCount; ++i) {
      mThreadNameFilters[i] = aThreadNameFilters[i];
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
      mozilla::tasktracer::StartLogging();
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

    if (!threadSelected(aInfo, mThreadNameFilters)) {
      return;
    }

    ThreadProfile* profile = new ThreadProfile(aInfo, mBuffer);
    aInfo->SetProfile(profile);
  }

  
  virtual void Tick(TickSample* sample) override;

  
  virtual SyncProfile* GetBacktrace() override;

  
  virtual void RequestSave() override
  {
    mSaveRequested = true;
#ifdef MOZ_TASK_TRACER
    if (mTaskTracer) {
      mozilla::tasktracer::StopLogging();
    }
#endif
  }

  virtual void HandleSaveRequest() override;
  virtual void DeleteExpiredMarkers() override;

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
  void FlushOnJSShutdown(JSRuntime* aRuntime);
  bool ProfileJS() const { return mProfileJS; }
  bool ProfileJava() const { return mProfileJava; }
  bool ProfileGPU() const { return mProfileGPU; }
  bool ProfilePower() const { return mProfilePower; }
  bool ProfileThreads() const override { return mProfileThreads; }
  bool InPrivacyMode() const { return mPrivacyMode; }
  bool AddMainThreadIO() const { return mAddMainThreadIO; }
  bool ProfileMemory() const { return mProfileMemory; }
  bool TaskTracer() const { return mTaskTracer; }
  bool LayersDump() const { return mLayersDump; }
  bool DisplayListDump() const { return mDisplayListDump; }
  bool ProfileRestyle() const { return mProfileRestyle; }

protected:
  
  virtual void InplaceTick(TickSample* sample);

  
  void doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample);

  void StreamJSObject(JSStreamWriter& b);

  
  ThreadProfile* mPrimaryThreadProfile;
  nsRefPtr<ProfileBuffer> mBuffer;
  bool mSaveRequested;
  bool mAddLeafAddresses;
  bool mUseStackWalk;
  bool mProfileJS;
  bool mProfileGPU;
  bool mProfileThreads;
  bool mProfileJava;
  bool mProfilePower;
  bool mLayersDump;
  bool mDisplayListDump;
  bool mProfileRestyle;

  
  
  ThreadNameFilterList mThreadNameFilters;
  bool mPrivacyMode;
  bool mAddMainThreadIO;
  bool mProfileMemory;
  bool mTaskTracer;
#if defined(XP_WIN)
  IntelPowerGadget* mIntelPowerGadget;
#endif
};

#endif

