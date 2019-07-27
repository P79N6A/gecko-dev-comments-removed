




#include "VideoStreamTrack.h"

#include "mozilla/dom/VideoStreamTrackBinding.h"

namespace mozilla {
namespace dom {

JSObject*
VideoStreamTrack::WrapObject(JSContext* aCx)
{
  return VideoStreamTrackBinding::Wrap(aCx, this);
}

}
}
