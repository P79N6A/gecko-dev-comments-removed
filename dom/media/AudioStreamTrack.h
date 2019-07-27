




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

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual AudioStreamTrack* AsAudioStreamTrack() override { return this; }

  
  virtual void GetKind(nsAString& aKind) override { aKind.AssignLiteral("audio"); }
};

} 
} 

#endif 
