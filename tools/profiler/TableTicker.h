




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

namespace mozilla {
class ProfileGatherer;
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
              const char** aThreadNameFilters, uint32_t aFilterCount);
  ~TableTicker();

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

  void ToStreamAsJSON(std::ostream& stream, double aSinceTime = 0);
  virtual JSObject *ToJSObject(JSContext* aCx, double aSinceTime = 0);
  mozilla::UniquePtr<char[]> ToJSON(double aSinceTime = 0);
  virtual void ToJSObjectAsync(double aSinceTime = 0, mozilla::dom::Promise* aPromise = 0);
  void StreamMetaJSCustomObject(SpliceableJSONWriter& aWriter);
  void StreamTaskTracer(SpliceableJSONWriter& aWriter);
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

  void GetBufferInfo(uint32_t *aCurrentPosition, uint32_t *aTotalSize, uint32_t *aGeneration);

  void ProfileGathered();

protected:
  
  virtual void InplaceTick(TickSample* sample);

  
  void doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample);

  void StreamJSON(SpliceableJSONWriter& aWriter, double aSinceTime);

  
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

private:
  nsRefPtr<mozilla::ProfileGatherer> mGatherer;
};

#endif

