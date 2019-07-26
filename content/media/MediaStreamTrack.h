




#ifndef MEDIASTREAMTRACK_H_
#define MEDIASTREAMTRACK_H_

#include "nsDOMEventTargetHelper.h"
#include "nsID.h"
#include "StreamBuffer.h"

namespace mozilla {

class DOMMediaStream;

namespace dom {

class AudioStreamTrack;
class VideoStreamTrack;




class MediaStreamTrack : public nsDOMEventTargetHelper {
public:
  



  MediaStreamTrack(DOMMediaStream* aStream, TrackID aTrackID);
  virtual ~MediaStreamTrack();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaStreamTrack, nsDOMEventTargetHelper)

  DOMMediaStream* GetParentObject() const { return mStream; }
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE = 0;

  DOMMediaStream* GetStream() const { return mStream; }
  TrackID GetTrackID() const { return mTrackID; }
  virtual AudioStreamTrack* AsAudioStreamTrack() { return nullptr; }
  virtual VideoStreamTrack* AsVideoStreamTrack() { return nullptr; }

  
  virtual void GetKind(nsAString& aKind) = 0;
  void GetId(nsAString& aID);
  void GetLabel(nsAString& aLabel) { aLabel.Truncate(); }

  
  void NotifyEnded() { mEnded = true; }

protected:
  nsRefPtr<DOMMediaStream> mStream;
  TrackID mTrackID;
  nsID mID;
  bool mEnded;
};

}
}

#endif 
