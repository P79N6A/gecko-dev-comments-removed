




#include "platform.h"
#include "ProfileEntry.h"
#include "mozilla/Mutex.h"
#include "IntelPowerGadget.h"

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

extern TimeStamp sLastTracerEvent;
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
    mProfilePower = hasFeature(aFeatures, aFeatureCount, "power");
    mProfileThreads = hasFeature(aFeatures, aFeatureCount, "threads");
    mUnwinderThread = hasFeature(aFeatures, aFeatureCount, "unwinder") || sps_version2();
    mAddLeafAddresses = hasFeature(aFeatures, aFeatureCount, "leaf");
    mPrivacyMode = hasFeature(aFeatures, aFeatureCount, "privacy");
    mAddMainThreadIO = hasFeature(aFeatures, aFeatureCount, "mainthreadio");

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

    sStartTime = TimeStamp::Now();

    {
      mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

      
      for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
        ThreadInfo* info = sRegisteredThreads->at(i);

        RegisterThread(info);
      }

      SetActiveSampler(this);
    }
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

    ThreadProfile* profile = new ThreadProfile(aInfo->Name(),
                                               EntrySize(),
                                               aInfo->Stack(),
                                               aInfo->ThreadId(),
                                               aInfo->GetPlatformData(),
                                               aInfo->IsMainThread(),
                                               aInfo->StackTop());
    aInfo->SetProfile(profile);
  }

  
  virtual void Tick(TickSample* sample);

  
  virtual SyncProfile* GetBacktrace();

  
  virtual void RequestSave()
  {
    mSaveRequested = true;
  }

  virtual void HandleSaveRequest();

  ThreadProfile* GetPrimaryThreadProfile()
  {
    if (!mPrimaryThreadProfile) {
      mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

      for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
        ThreadInfo* info = sRegisteredThreads->at(i);
        if (info->IsMainThread()) {
          mPrimaryThreadProfile = info->Profile();
          break;
        }
      }
    }

    return mPrimaryThreadProfile;
  }

  void ToStreamAsJSON(std::ostream& stream);
  virtual JSObject *ToJSObject(JSContext *aCx);
  template <typename Builder> typename Builder::Object GetMetaJSCustomObject(Builder& b);

  bool HasUnwinderThread() const { return mUnwinderThread; }
  bool ProfileJS() const { return mProfileJS; }
  bool ProfileJava() const { return mProfileJava; }
  bool ProfilePower() const { return mProfilePower; }
  bool ProfileThreads() const { return mProfileThreads; }
  bool InPrivacyMode() const { return mPrivacyMode; }
  bool AddMainThreadIO() const { return mAddMainThreadIO; }

protected:
  
  virtual void UnwinderTick(TickSample* sample);

  
  virtual void InplaceTick(TickSample* sample);

  
  void doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample);

  template <typename Builder> void BuildJSObject(Builder& b, typename Builder::ObjectHandle profile);

  
  ThreadProfile* mPrimaryThreadProfile;
  bool mSaveRequested;
  bool mAddLeafAddresses;
  bool mUseStackWalk;
  bool mJankOnly;
  bool mProfileJS;
  bool mProfileThreads;
  bool mUnwinderThread;
  bool mProfileJava;
  bool mProfilePower;

  
  
  char** mThreadNameFilters;
  uint32_t mFilterCount;
  bool mPrivacyMode;
  bool mAddMainThreadIO;
#if defined(XP_WIN)
  IntelPowerGadget* mIntelPowerGadget;
#endif
};

