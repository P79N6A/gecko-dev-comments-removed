




#include "AudioStreamTrack.h"

#include "mozilla/dom/AudioStreamTrackBinding.h"

namespace mozilla {
namespace dom {

JSObject*
AudioStreamTrack::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return AudioStreamTrackBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
