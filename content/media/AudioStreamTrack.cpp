




#include "AudioStreamTrack.h"

#include "mozilla/dom/AudioStreamTrackBinding.h"

namespace mozilla {
namespace dom {

JSObject*
AudioStreamTrack::WrapObject(JSContext* aCx)
{
  return AudioStreamTrackBinding::Wrap(aCx, this);
}

}
}
