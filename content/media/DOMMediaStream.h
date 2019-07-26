




#ifndef NSDOMMEDIASTREAM_H_
#define NSDOMMEDIASTREAM_H_

#include "nsIDOMMediaStream.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"
#include "nsIDOMWindow.h"
#include "StreamBuffer.h"
#include "nsIRunnable.h"

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
  typedef uint8_t TrackTypeHints;

  DOMMediaStream();
  virtual ~DOMMediaStream();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMMediaStream)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  double CurrentTime();
  void GetAudioTracks(nsTArray<nsRefPtr<AudioStreamTrack> >& aTracks);
  void GetVideoTracks(nsTArray<nsRefPtr<VideoStreamTrack> >& aTracks);

  MediaStream* GetStream() { return mStream; }
  bool IsFinished();
  



  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  





  bool CombineWithPrincipal(nsIPrincipal* aPrincipal);

  




  void NotifyMediaStreamGraphShutdown();

  
  enum {
    HINT_CONTENTS_AUDIO = 1 << 0,
    HINT_CONTENTS_VIDEO = 1 << 1
  };
  TrackTypeHints GetHintContents() const { return mHintContents; }
  void SetHintContents(TrackTypeHints aHintContents) { mHintContents = aHintContents; }

  


  static already_AddRefed<DOMMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents);

  


  static already_AddRefed<DOMMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents = 0);

  void SetLogicalStreamStartTime(StreamTime aTime)
  {
    mLogicalStreamStartTime = aTime;
  }

  
  
  MediaStreamTrack* CreateDOMTrack(TrackID aTrackID, MediaSegment::Type aType);
  MediaStreamTrack* GetDOMTrackFor(TrackID aTrackID);

  class OnTracksAvailableCallback {
  public:
    OnTracksAvailableCallback(uint8_t aExpectedTracks = 0)
      : mExpectedTracks(aExpectedTracks) {}
    virtual ~OnTracksAvailableCallback() {}
    virtual void NotifyTracksAvailable(DOMMediaStream* aStream) = 0;
    TrackTypeHints GetExpectedTracks() { return mExpectedTracks; }
    void SetExpectedTracks(TrackTypeHints aExpectedTracks) { mExpectedTracks = aExpectedTracks; }
  private:
    TrackTypeHints mExpectedTracks;
  };
  
  
  
  
  
  
  
  
  void OnTracksAvailable(OnTracksAvailableCallback* aCallback);

protected:
  void Destroy();
  void InitSourceStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents);
  void InitTrackUnionStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents);
  void InitStreamCommon(MediaStream* aStream);
  void CheckTracksAvailable();

  class StreamListener;
  friend class StreamListener;

  
  StreamTime mLogicalStreamStartTime;

  
  nsCOMPtr<nsIDOMWindow> mWindow;

  
  
  MediaStream* mStream;
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  nsAutoTArray<nsRefPtr<MediaStreamTrack>,2> mTracks;
  nsRefPtr<StreamListener> mListener;

  nsTArray<nsAutoPtr<OnTracksAvailableCallback> > mRunOnTracksAvailable;

  
  uint8_t mHintContents;
  
  uint8_t mTrackTypesAvailable;
  bool mNotifiedOfMediaStreamGraphShutdown;
};

class DOMLocalMediaStream : public DOMMediaStream,
                            public nsIDOMLocalMediaStream
{
public:
  DOMLocalMediaStream() {}
  virtual ~DOMLocalMediaStream();

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  virtual void Stop();

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents);

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents = 0);
};

}

#endif 
