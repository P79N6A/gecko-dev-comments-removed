




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

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  virtual VideoStreamTrack* AsVideoStreamTrack() MOZ_OVERRIDE { return this; }

  
  virtual void GetKind(nsAString& aKind) MOZ_OVERRIDE { aKind.AssignLiteral("video"); }
};

}
}

#endif 
