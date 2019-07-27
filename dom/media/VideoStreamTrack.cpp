




#include "VideoStreamTrack.h"

#include "mozilla/dom/VideoStreamTrackBinding.h"

namespace mozilla {
namespace dom {

JSObject*
VideoStreamTrack::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return VideoStreamTrackBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
