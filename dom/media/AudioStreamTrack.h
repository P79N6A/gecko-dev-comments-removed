




#ifndef AUDIOSTREAMTRACK_H_
#define AUDIOSTREAMTRACK_H_

#include "MediaStreamTrack.h"
#include "DOMMediaStream.h"

namespace mozilla {
namespace dom {

class AudioStreamTrack : public MediaStreamTrack {
public:
  AudioStreamTrack(DOMMediaStream* aStream, TrackID aTrackID)
    : MediaStreamTrack(aStream, aTrackID) {}

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  virtual AudioStreamTrack* AsAudioStreamTrack() MOZ_OVERRIDE { return this; }

  
  virtual void GetKind(nsAString& aKind) MOZ_OVERRIDE { aKind.AssignLiteral("audio"); }
};

}
}

#endif 
