




#include "platform.h"
#include "ProfileEntry.h"

static bool
hasFeature(const char** aFeatures, uint32_t aFeatureCount, const char* aFeature) {
  for(size_t i = 0; i < aFeatureCount; i++) {
    if (strcmp(aFeatures[i], aFeature) == 0)
      return true;
  }
  return false;
}

void read_env_vars();
typedef  enum { UnwINVALID, UnwNATIVE, UnwPSEUDO, UnwCOMBINED }  UnwMode;
extern UnwMode sUnwindMode;
extern int     sUnwindInterval;

extern TimeStamp sLastTracerEvent;
extern int sFrameNumber;
extern int sLastFrameNumber;
extern unsigned int sCurrentEventGeneration;
extern unsigned int sLastSampledEventGeneration;

class BreakpadSampler;

class TableTicker: public Sampler {
 public:
  TableTicker(int aInterval, int aEntrySize, PseudoStack *aStack,
              const char** aFeatures, uint32_t aFeatureCount)
    : Sampler(aInterval, true)
    , mPrimaryThreadProfile(aEntrySize, aStack)
    , mStartTime(TimeStamp::Now())
    , mSaveRequested(false)
  {
    mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");

    
    mJankOnly = hasFeature(aFeatures, aFeatureCount, "jank");
    mProfileJS = hasFeature(aFeatures, aFeatureCount, "js");
    mAddLeafAddresses = hasFeature(aFeatures, aFeatureCount, "leaf");
    mPrimaryThreadProfile.addTag(ProfileEntry('m', "Start"));
  }

  ~TableTicker() { if (IsActive()) Stop(); }

  virtual void SampleStack(TickSample* sample) {}

  
  virtual void Tick(TickSample* sample);

  
  virtual void RequestSave()
  {
    mSaveRequested = true;
  }

  virtual void HandleSaveRequest();

  ThreadProfile* GetPrimaryThreadProfile()
  {
    return &mPrimaryThreadProfile;
  }

  void ToStreamAsJSON(std::ostream& stream);
  virtual JSObject *ToJSObject(JSContext *aCx);
  JSCustomObject *GetMetaJSCustomObject(JSAObjectBuilder& b);

  const bool ProfileJS() { return mProfileJS; }

  virtual BreakpadSampler* AsBreakpadSampler() { return nullptr; }

protected:
  
  void doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample);

  void BuildJSObject(JSAObjectBuilder& b, JSCustomObject* profile);

  
  ThreadProfile mPrimaryThreadProfile;
  TimeStamp mStartTime;
  bool mSaveRequested;
  bool mAddLeafAddresses;
  bool mUseStackWalk;
  bool mJankOnly;
  bool mProfileJS;
};

class BreakpadSampler: public TableTicker {
 public:
  BreakpadSampler(int aInterval, int aEntrySize, PseudoStack *aStack,
              const char** aFeatures, uint32_t aFeatureCount)
    : TableTicker(aInterval, aEntrySize, aStack, aFeatures, aFeatureCount)
  {}

  
  virtual void Tick(TickSample* sample);

  virtual BreakpadSampler* AsBreakpadSampler() { return this; }
};

