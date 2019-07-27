




#ifndef PROFILER_MARKERS_H
#define PROFILER_MARKERS_H

#include "JSStreamWriter.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "Units.h"    

namespace mozilla {
namespace layers {
class Layer;
} 
} 













class ProfilerMarkerPayload
{
public:
  


  explicit ProfilerMarkerPayload(ProfilerBacktrace* aStack = nullptr);
  ProfilerMarkerPayload(const mozilla::TimeStamp& aStartTime,
                        const mozilla::TimeStamp& aEndTime,
                        ProfilerBacktrace* aStack = nullptr);

  


  virtual ~ProfilerMarkerPayload();

  


  void StreamPayload(JSStreamWriter& b) {
    return streamPayload(b);
  }

  mozilla::TimeStamp GetStartTime() const { return mStartTime; }

protected:
  


  void streamCommonProps(const char* aMarkerType, JSStreamWriter& b);

  


  virtual void
  streamPayload(JSStreamWriter& b) = 0;

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

protected:
  virtual void
  streamPayload(JSStreamWriter& b) { return streamPayloadImp(b); }

private:
  void streamPayloadImp(JSStreamWriter& b);

private:
  const char *mCategory;
  TracingMetadata mMetaData;
};


#include "gfxASurface.h"
class ProfilerMarkerImagePayload : public ProfilerMarkerPayload
{
public:
  explicit ProfilerMarkerImagePayload(gfxASurface *aImg);

protected:
  virtual void
  streamPayload(JSStreamWriter& b) { return streamPayloadImp(b); }

private:
  void streamPayloadImp(JSStreamWriter& b);

  nsRefPtr<gfxASurface> mImg;
};

class IOMarkerPayload : public ProfilerMarkerPayload
{
public:
  IOMarkerPayload(const char* aSource, const char* aFilename, const mozilla::TimeStamp& aStartTime,
                  const mozilla::TimeStamp& aEndTime,
                  ProfilerBacktrace* aStack);
  ~IOMarkerPayload();

protected:
  virtual void
  streamPayload(JSStreamWriter& b) { return streamPayloadImp(b); }

private:
  void streamPayloadImp(JSStreamWriter& b);

  const char* mSource;
  char* mFilename;
};





class LayerTranslationPayload : public ProfilerMarkerPayload
{
public:
  LayerTranslationPayload(mozilla::layers::Layer* aLayer,
                          mozilla::gfx::Point aPoint);

protected:
  virtual void
  streamPayload(JSStreamWriter& b) { return streamPayloadImpl(b); }

private:
  void streamPayloadImpl(JSStreamWriter& b);
  mozilla::layers::Layer* mLayer;
  mozilla::gfx::Point mPoint;
};





class TouchDataPayload : public ProfilerMarkerPayload
{
public:
  explicit TouchDataPayload(const mozilla::ScreenIntPoint& aPoint);
  virtual ~TouchDataPayload() {}

protected:
  virtual void
  streamPayload(JSStreamWriter& b) { return streamPayloadImpl(b); }

private:
  void streamPayloadImpl(JSStreamWriter& b);
  mozilla::ScreenIntPoint mPoint;
};




class VsyncPayload : public ProfilerMarkerPayload
{
public:
  explicit VsyncPayload(mozilla::TimeStamp aVsyncTimestamp);
  virtual ~VsyncPayload() {}

protected:
  virtual void
  streamPayload(JSStreamWriter& b) { return streamPayloadImpl(b); }

private:
  void streamPayloadImpl(JSStreamWriter& b);
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

protected:
  virtual void
  streamPayload(JSStreamWriter& b) override { return streamPayloadImp(b); }

private:
  void streamPayloadImp(JSStreamWriter& b);

  mozilla::TimeStamp mCpuTimeStart;
  mozilla::TimeStamp mCpuTimeEnd;
  uint64_t mGpuTimeStart;
  uint64_t mGpuTimeEnd;
};

#endif 
