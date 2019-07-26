




#include "VideoStreamTrack.h"

#include "mozilla/dom/VideoStreamTrackBinding.h"

namespace mozilla {
namespace dom {

JSObject*
VideoStreamTrack::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return VideoStreamTrackBinding::Wrap(aCx, aScope, this);
}

}
}
