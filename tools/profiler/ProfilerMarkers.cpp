




#include "ProfilerMarkers.h"
#include "gfxASurface.h"

ProfilerMarkerImagePayload::ProfilerMarkerImagePayload(gfxASurface *aImg)
  : mImg(aImg)
{}

template<typename Builder>
typename Builder::Object
ProfilerMarkerImagePayload::preparePayloadImp(Builder& b)
{
  typename Builder::RootedObject data(b.context(), b.CreateObject());
  b.DefineProperty(data, "type", "innerHTML");
  
  
  return data;
}

template JSCustomObjectBuilder::Object
ProfilerMarkerImagePayload::preparePayloadImp<JSCustomObjectBuilder>(JSCustomObjectBuilder& b);
template JSObjectBuilder::Object
ProfilerMarkerImagePayload::preparePayloadImp<JSObjectBuilder>(JSObjectBuilder& b);

