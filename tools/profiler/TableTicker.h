




#include "platform.h"
#include "ProfileEntry.h"
#include "mozilla/Mutex.h"

static bool
hasFeature(const char** aFeatures, uint32_t aFeatureCount, const char* aFeature) {
  for(size_t i = 0; i < aFeatureCount; i++) {
    if (strcmp(aFeatures[i], aFeature) == 0)
      return true;
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
  TableTicker(int aInterval, int aEntrySize,
              const char** aFeatures, uint32_t aFeatureCount)
    : Sampler(aInterval, true, aEntrySize)
    , mPrimaryThreadProfile(nullptr)
    , mStartTime(TimeStamp::Now())
    , mSaveRequested(false)
  {
    mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");

    
    mJankOnly = hasFeature(aFeatures, aFeatureCount, "jank");
    mProfileJS = hasFeature(aFeatures, aFeatureCount, "js");
    mProfileThreads = hasFeature(aFeatures, aFeatureCount, "threads");
    mAddLeafAddresses = hasFeature(aFeatures, aFeatureCount, "leaf");

    {
      mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

      
      for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
        ThreadInfo* info = sRegisteredThreads->at(i);

        if (!info->IsMainThread() && !mProfileThreads)
          continue;

        ThreadProfile* profile = new ThreadProfile(info->Name(),
                                                   aEntrySize,
                                                   info->Stack(),
                                                   info->ThreadId(),
                                                   info->GetPlatformData(),
                                                   info->IsMainThread());
        profile->addTag(ProfileEntry('m', "Start"));

        info->SetProfile(profile);
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
  }

  
  virtual void Tick(TickSample* sample);

  
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
  JSCustomObject *GetMetaJSCustomObject(JSAObjectBuilder& b);

  bool ProfileJS() const { return mProfileJS; }
  bool ProfileThreads() const { return mProfileThreads; }

  virtual BreakpadSampler* AsBreakpadSampler() { return nullptr; }

protected:
  
  void doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample);

  void BuildJSObject(JSAObjectBuilder& b, JSCustomObject* profile);

  
  ThreadProfile* mPrimaryThreadProfile;
  TimeStamp mStartTime;
  bool mSaveRequested;
  bool mAddLeafAddresses;
  bool mUseStackWalk;
  bool mJankOnly;
  bool mProfileJS;
  bool mProfileThreads;
};

class BreakpadSampler: public TableTicker {
 public:
  BreakpadSampler(int aInterval, int aEntrySize,
              const char** aFeatures, uint32_t aFeatureCount)
    : TableTicker(aInterval, aEntrySize, aFeatures, aFeatureCount)
  {}

  
  virtual void Tick(TickSample* sample);

  virtual BreakpadSampler* AsBreakpadSampler() { return this; }
};

