




#ifndef PROFILER_MARKERS_H
#define PROFILER_MARKERS_H

#include "JSCustomObjectBuilder.h"
#include "JSObjectBuilder.h"
#include "nsAutoPtr.h"













class ProfilerMarkerPayload {
public:
  ProfilerMarkerPayload() {}

  


  virtual ~ProfilerMarkerPayload() {}

  


  template<typename Builder>
  typename Builder::Object PreparePayload(Builder& b)
  {
    return preparePayload(b);
  }

protected:
  


  virtual JSCustomObjectBuilder::Object
  preparePayload(JSCustomObjectBuilder& b) = 0;

  


  virtual JSObjectBuilder::Object
  preparePayload(JSObjectBuilder& b) = 0;
};

class gfxASurface;
class ProfilerMarkerImagePayload : public ProfilerMarkerPayload {
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

#endif 
