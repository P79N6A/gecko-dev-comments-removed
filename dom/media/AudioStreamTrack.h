




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

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual AudioStreamTrack* AsAudioStreamTrack() { return this; }

  
  virtual void GetKind(nsAString& aKind) { aKind.AssignLiteral("audio"); }
};

}
}

#endif 
