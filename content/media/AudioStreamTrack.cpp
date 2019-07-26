




#include "AudioStreamTrack.h"

#include "mozilla/dom/AudioStreamTrackBinding.h"

namespace mozilla {
namespace dom {

JSObject*
AudioStreamTrack::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return AudioStreamTrackBinding::Wrap(aCx, aScope, this);
}

}
}
