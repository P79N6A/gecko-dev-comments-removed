




#ifndef PROFILER_MARKERS_H
#define PROFILER_MARKERS_H

#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace layers {
class Layer;
} 
} 

class SpliceableJSONWriter;
class UniqueStacks;













class ProfilerMarkerPayload
{
public:
  


  explicit ProfilerMarkerPayload(ProfilerBacktrace* aStack = nullptr);
  ProfilerMarkerPayload(const mozilla::TimeStamp& aStartTime,
                        const mozilla::TimeStamp& aEndTime,
                        ProfilerBacktrace* aStack = nullptr);

  


  virtual ~ProfilerMarkerPayload();

  


  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) = 0;

  mozilla::TimeStamp GetStartTime() const { return mStartTime; }

protected:
  


  void streamCommonProps(const char* aMarkerType, SpliceableJSONWriter& aWriter,
                         UniqueStacks& aUniqueStacks);

  void SetStack(ProfilerBacktrace* aStack) { mStack = aStack; }

private:
  mozilla::TimeStamp  mStartTime;
  mozilla::TimeStamp  mEndTime;
  ProfilerBacktrace*  mStack;
};

class ProfilerMarkerTracing : public ProfilerMarkerPayload
{
public:
  ProfilerMarkerTracing(const char* aCategory, TracingMetadata aMetaData);
  ProfilerMarkerTracing(const char* aCategory, TracingMetadata aMetaData, ProfilerBacktrace* aCause);

  const char *GetCategory() const { return mCategory; }
  TracingMetadata GetMetaData() const { return mMetaData; }

  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) override;

private:
  const char *mCategory;
  TracingMetadata mMetaData;
};


#ifndef SPS_STANDALONE
#include "gfxASurface.h"
class ProfilerMarkerImagePayload : public ProfilerMarkerPayload
{
public:
  explicit ProfilerMarkerImagePayload(gfxASurface *aImg);

  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) override;

private:
  nsRefPtr<gfxASurface> mImg;
};

class IOMarkerPayload : public ProfilerMarkerPayload
{
public:
  IOMarkerPayload(const char* aSource, const char* aFilename, const mozilla::TimeStamp& aStartTime,
                  const mozilla::TimeStamp& aEndTime,
                  ProfilerBacktrace* aStack);
  ~IOMarkerPayload();

  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) override;

private:
  const char* mSource;
  char* mFilename;
};





class LayerTranslationPayload : public ProfilerMarkerPayload
{
public:
  LayerTranslationPayload(mozilla::layers::Layer* aLayer,
                          mozilla::gfx::Point aPoint);

  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) override;

private:
  mozilla::layers::Layer* mLayer;
  mozilla::gfx::Point mPoint;
};

#include "Units.h"    





class TouchDataPayload : public ProfilerMarkerPayload
{
public:
  explicit TouchDataPayload(const mozilla::ScreenIntPoint& aPoint);
  virtual ~TouchDataPayload() {}

  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) override;

private:
  mozilla::ScreenIntPoint mPoint;
};




class VsyncPayload : public ProfilerMarkerPayload
{
public:
  explicit VsyncPayload(mozilla::TimeStamp aVsyncTimestamp);
  virtual ~VsyncPayload() {}

  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) override;

private:
  mozilla::TimeStamp mVsyncTimestamp;
};

class GPUMarkerPayload : public ProfilerMarkerPayload
{
public:
  GPUMarkerPayload(const mozilla::TimeStamp& aCpuTimeStart,
                   const mozilla::TimeStamp& aCpuTimeEnd,
                   uint64_t aGpuTimeStart,
                   uint64_t aGpuTimeEnd);
  ~GPUMarkerPayload() {}

  virtual void StreamPayload(SpliceableJSONWriter& aWriter,
                             UniqueStacks& aUniqueStacks) override;

private:
  mozilla::TimeStamp mCpuTimeStart;
  mozilla::TimeStamp mCpuTimeEnd;
  uint64_t mGpuTimeStart;
  uint64_t mGpuTimeEnd;
};
#endif

#endif 
