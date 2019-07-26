




#ifndef PROFILER_MARKERS_H
#define PROFILER_MARKERS_H

#include "JSCustomObjectBuilder.h"
#include "JSObjectBuilder.h"
#include "mozilla/TimeStamp.h"
#include "nsAutoPtr.h"













class ProfilerMarkerPayload
{
public:
  


  ProfilerMarkerPayload(ProfilerBacktrace* aStack = nullptr);
  ProfilerMarkerPayload(const mozilla::TimeStamp& aStartTime,
                        const mozilla::TimeStamp& aEndTime,
                        ProfilerBacktrace* aStack = nullptr);

  


  virtual ~ProfilerMarkerPayload();

  


  template<typename Builder>
  typename Builder::Object PreparePayload(Builder& b)
  {
    return preparePayload(b);
  }

protected:
  


  template<typename Builder>
  void prepareCommonProps(const char* aMarkerType, Builder& aBuilder,
                          typename Builder::ObjectHandle aObject);

  


  virtual JSCustomObjectBuilder::Object
  preparePayload(JSCustomObjectBuilder& b) = 0;

  


  virtual JSObjectBuilder::Object
  preparePayload(JSObjectBuilder& b) = 0;

private:
  mozilla::TimeStamp  mStartTime;
  mozilla::TimeStamp  mEndTime;
  ProfilerBacktrace*  mStack;
};

class gfxASurface;
class ProfilerMarkerImagePayload : public ProfilerMarkerPayload
{
public:
  ProfilerMarkerImagePayload(gfxASurface *aImg);

protected:
  virtual JSCustomObjectBuilder::Object
  preparePayload(JSCustomObjectBuilder& b) { return preparePayloadImp(b); }
  virtual JSObjectBuilder::Object
  preparePayload(JSObjectBuilder& b) { return preparePayloadImp(b); }

private:
  template<typename Builder>
  typename Builder::Object preparePayloadImp(Builder& b);
  
  nsRefPtr<gfxASurface> mImg;
};

class IOMarkerPayload : public ProfilerMarkerPayload
{
public:
  IOMarkerPayload(const char* aSource, const mozilla::TimeStamp& aStartTime,
                  const mozilla::TimeStamp& aEndTime,
                  ProfilerBacktrace* aStack);

protected:
  virtual JSCustomObjectBuilder::Object
  preparePayload(JSCustomObjectBuilder& b) { return preparePayloadImp(b); }
  virtual JSObjectBuilder::Object
  preparePayload(JSObjectBuilder& b) { return preparePayloadImp(b); }

private:
  template<typename Builder>
  typename Builder::Object preparePayloadImp(Builder& b);
  
  const char* mSource;
};

#endif 
