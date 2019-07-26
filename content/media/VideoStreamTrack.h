




#ifndef VIDEOSTREAMTRACK_H_
#define VIDEOSTREAMTRACK_H_

#include "MediaStreamTrack.h"
#include "DOMMediaStream.h"

namespace mozilla {
namespace dom {

class VideoStreamTrack : public MediaStreamTrack {
public:
  VideoStreamTrack(DOMMediaStream* aStream, TrackID aTrackID)
    : MediaStreamTrack(aStream, aTrackID) {}

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual VideoStreamTrack* AsVideoStreamTrack() { return this; }

  
  virtual void GetKind(nsAString& aKind) { aKind.AssignLiteral("video"); }
};

}
}

#endif 
