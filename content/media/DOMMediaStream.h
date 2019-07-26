




#ifndef NSDOMMEDIASTREAM_H_
#define NSDOMMEDIASTREAM_H_

#include "nsIDOMMediaStream.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"
#include "nsIDOMWindow.h"
#include "StreamBuffer.h"

class nsXPCClassInfo;




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif


#ifdef CurrentTime
#undef CurrentTime
#endif

namespace mozilla {

class MediaStream;

namespace dom {
class MediaStreamTrack;
class AudioStreamTrack;
class VideoStreamTrack;
}




class DOMMediaStream : public nsIDOMMediaStream,
                       public nsWrapperCache
{
  friend class DOMLocalMediaStream;
  typedef dom::MediaStreamTrack MediaStreamTrack;
  typedef dom::AudioStreamTrack AudioStreamTrack;
  typedef dom::VideoStreamTrack VideoStreamTrack;

public:
  DOMMediaStream();
  virtual ~DOMMediaStream();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMMediaStream)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  
  double CurrentTime();
  void GetAudioTracks(nsTArray<nsRefPtr<AudioStreamTrack> >& aTracks);
  void GetVideoTracks(nsTArray<nsRefPtr<VideoStreamTrack> >& aTracks);

  MediaStream* GetStream() { return mStream; }
  bool IsFinished();
  



  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  





  bool CombineWithPrincipal(nsIPrincipal* aPrincipal);

  


  static already_AddRefed<DOMMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents);

  
  
  enum {
    HINT_CONTENTS_AUDIO = 0x00000001U,
    HINT_CONTENTS_VIDEO = 0x00000002U
  };
  uint32_t GetHintContents() const { return mHintContents; }
  void SetHintContents(uint32_t aHintContents) { mHintContents = aHintContents; }

  


  static already_AddRefed<DOMMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents = 0);

  
  MediaStreamTrack* CreateDOMTrack(TrackID aTrackID, MediaSegment::Type aType);
  MediaStreamTrack* GetDOMTrackFor(TrackID aTrackID);

protected:
  void InitSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents);
  void InitTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents);
  void InitStreamCommon(MediaStream* aStream);

  class StreamListener;
  friend class StreamListener;

  
  nsCOMPtr<nsIDOMWindow> mWindow;

  
  
  MediaStream* mStream;
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  nsAutoTArray<nsRefPtr<MediaStreamTrack>,2> mTracks;
  nsRefPtr<StreamListener> mListener;

  
  
  uint32_t mHintContents;
};

class DOMLocalMediaStream : public DOMMediaStream,
                            public nsIDOMLocalMediaStream
{
public:
  DOMLocalMediaStream() {}
  virtual ~DOMLocalMediaStream();

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  virtual void Stop();

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents);

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents = 0);
};

}

#endif 
