




#ifndef NSDOMMEDIASTREAM_H_
#define NSDOMMEDIASTREAM_H_

#include "nsIDOMMediaStream.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "StreamBuffer.h"
#include "nsIDOMWindow.h"
#include "nsIPrincipal.h"
#include "mozilla/PeerIdentity.h"

class nsXPCClassInfo;




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif


#ifdef CurrentTime
#undef CurrentTime
#endif

namespace mozilla {

class DOMLocalMediaStream;
class MediaStream;
class MediaEngineSource;

namespace dom {
class AudioNode;
class MediaStreamTrack;
class AudioStreamTrack;
class VideoStreamTrack;
class AudioTrack;
class VideoTrack;
class AudioTrackList;
class VideoTrackList;
class MediaTrackListListener;
}

class MediaStreamDirectListener;




class DOMMediaStream : public nsIDOMMediaStream,
                       public nsWrapperCache
{
  friend class DOMLocalMediaStream;
  typedef dom::MediaStreamTrack MediaStreamTrack;
  typedef dom::AudioStreamTrack AudioStreamTrack;
  typedef dom::VideoStreamTrack VideoStreamTrack;
  typedef dom::AudioTrack AudioTrack;
  typedef dom::VideoTrack VideoTrack;
  typedef dom::AudioTrackList AudioTrackList;
  typedef dom::VideoTrackList VideoTrackList;
  typedef dom::MediaTrackListListener MediaTrackListListener;

public:
  typedef uint8_t TrackTypeHints;

  DOMMediaStream();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMMediaStream)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  double CurrentTime();

  void GetAudioTracks(nsTArray<nsRefPtr<AudioStreamTrack> >& aTracks);
  void GetVideoTracks(nsTArray<nsRefPtr<VideoStreamTrack> >& aTracks);
  void GetTracks(nsTArray<nsRefPtr<MediaStreamTrack> >& aTracks);
  bool HasTrack(const MediaStreamTrack& aTrack) const;

  MediaStream* GetStream() const { return mStream; }

  




  virtual bool AddDirectListener(MediaStreamDirectListener *aListener) { return false; }
  virtual void RemoveDirectListener(MediaStreamDirectListener *aListener) {}

  



  virtual void SetTrackEnabled(TrackID aTrackID, bool aEnabled);

  virtual void StopTrack(TrackID aTrackID);

  virtual DOMLocalMediaStream* AsDOMLocalMediaStream() { return nullptr; }

  bool IsFinished();
  



  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  




  PeerIdentity* GetPeerIdentity() const { return mPeerIdentity; }
  void SetPeerIdentity(PeerIdentity* aPeerIdentity)
  {
    mPeerIdentity = aPeerIdentity;
  }

  





  bool CombineWithPrincipal(nsIPrincipal* aPrincipal);

  




  void SetPrincipal(nsIPrincipal* aPrincipal);

  



  class PrincipalChangeObserver
  {
  public:
    virtual void PrincipalChanged(DOMMediaStream* aMediaStream) = 0;
  };
  bool AddPrincipalChangeObserver(PrincipalChangeObserver* aObserver);
  bool RemovePrincipalChangeObserver(PrincipalChangeObserver* aObserver);

  




  void NotifyMediaStreamGraphShutdown();
  


  void NotifyStreamStateChanged();

  
  enum {
    HINT_CONTENTS_AUDIO = 1 << 0,
    HINT_CONTENTS_VIDEO = 1 << 1,
    HINT_CONTENTS_UNKNOWN = 1 << 2
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
    explicit OnTracksAvailableCallback(uint8_t aExpectedTracks = 0)
      : mExpectedTracks(aExpectedTracks) {}
    virtual ~OnTracksAvailableCallback() {}
    virtual void NotifyTracksAvailable(DOMMediaStream* aStream) = 0;
    TrackTypeHints GetExpectedTracks() { return mExpectedTracks; }
    void SetExpectedTracks(TrackTypeHints aExpectedTracks) { mExpectedTracks = aExpectedTracks; }
  private:
    TrackTypeHints mExpectedTracks;
  };
  
  
  
  
  
  
  
  
  
  void OnTracksAvailable(OnTracksAvailableCallback* aCallback);

  



  void AddConsumerToKeepAlive(nsISupports* aConsumer)
  {
    if (!IsFinished() && !mNotifiedOfMediaStreamGraphShutdown) {
      mConsumersToKeepAlive.AppendElement(aConsumer);
    }
  }

  




  void ConstructMediaTracks(AudioTrackList* aAudioTrackList,
                            VideoTrackList* aVideoTrackList);

  virtual void NotifyMediaStreamTrackCreated(MediaStreamTrack* aTrack);

  virtual void NotifyMediaStreamTrackEnded(MediaStreamTrack* aTrack);

protected:
  virtual ~DOMMediaStream();

  void Destroy();
  void InitSourceStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents);
  void InitTrackUnionStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents);
  void InitStreamCommon(MediaStream* aStream);
  already_AddRefed<AudioTrack> CreateAudioTrack(AudioStreamTrack* aStreamTrack);
  already_AddRefed<VideoTrack> CreateVideoTrack(VideoStreamTrack* aStreamTrack);

  void CheckTracksAvailable();

  class StreamListener;
  friend class StreamListener;

  
  StreamTime mLogicalStreamStartTime;

  
  nsCOMPtr<nsIDOMWindow> mWindow;

  
  
  MediaStream* mStream;

  nsAutoTArray<nsRefPtr<MediaStreamTrack>,2> mTracks;
  nsRefPtr<StreamListener> mListener;

  nsTArray<nsAutoPtr<OnTracksAvailableCallback> > mRunOnTracksAvailable;

  
  nsTArray<nsCOMPtr<nsISupports> > mConsumersToKeepAlive;

  
  uint8_t mHintContents;
  
  uint8_t mTrackTypesAvailable;
  bool mNotifiedOfMediaStreamGraphShutdown;

  
  
  nsTArray<MediaTrackListListener> mMediaTrackListListeners;

private:
  void NotifyPrincipalChanged();

  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsTArray<PrincipalChangeObserver*> mPrincipalChangeObservers;
  
  
  nsAutoPtr<PeerIdentity> mPeerIdentity;
};

class DOMLocalMediaStream : public DOMMediaStream,
                            public nsIDOMLocalMediaStream
{
public:
  DOMLocalMediaStream() {}

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual void Stop();

  virtual MediaEngineSource* GetMediaEngine(TrackID aTrackID) { return nullptr; }

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents);

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow, TrackTypeHints aHintContents = 0);

protected:
  virtual ~DOMLocalMediaStream();
};

class DOMAudioNodeMediaStream : public DOMMediaStream
{
  typedef dom::AudioNode AudioNode;
public:
  explicit DOMAudioNodeMediaStream(AudioNode* aNode);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMAudioNodeMediaStream, DOMMediaStream)

  


  static already_AddRefed<DOMAudioNodeMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow,
                         AudioNode* aNode,
                         TrackTypeHints aHintContents = 0);

protected:
  ~DOMAudioNodeMediaStream();

private:
  
  
  nsRefPtr<AudioNode> mStreamNode;
};

}

#endif 
