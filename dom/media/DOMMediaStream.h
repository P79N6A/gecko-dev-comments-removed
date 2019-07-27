




#ifndef NSDOMMEDIASTREAM_H_
#define NSDOMMEDIASTREAM_H_

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "StreamBuffer.h"
#include "nsIDOMWindow.h"
#include "nsIPrincipal.h"
#include "mozilla/PeerIdentity.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/CORSMode.h"




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
class MediaStreamGraph;

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

#define NS_DOMMEDIASTREAM_IID \
{ 0x8cb65468, 0x66c0, 0x444e, \
  { 0x89, 0x9f, 0x89, 0x1d, 0x9e, 0xd2, 0xbe, 0x7c } }




class DOMMediaStream : public DOMEventTargetHelper
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

  NS_DECL_ISUPPORTS_INHERITED
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMMediaStream,
                                           DOMEventTargetHelper)
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DOMMEDIASTREAM_IID)

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  double CurrentTime();

  void GetId(nsAString& aID) const;

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
  mozilla::CORSMode GetCORSMode();
  void SetCORSMode(mozilla::CORSMode aCORSMode);

  




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

  
  
  void AssignId(const nsAString& aID) { mID = aID; }

  


  static already_AddRefed<DOMMediaStream> CreateSourceStream(nsIDOMWindow* aWindow,
                                                             MediaStreamGraph* aGraph = nullptr);

  


  static already_AddRefed<DOMMediaStream> CreateTrackUnionStream(nsIDOMWindow* aWindow,
                                                                 MediaStreamGraph* aGraph = nullptr);

  void SetLogicalStreamStartTime(StreamTime aTime)
  {
    mLogicalStreamStartTime = aTime;
  }

  
  
  MediaStreamTrack* BindDOMTrack(TrackID aTrackID, MediaSegment::Type aType);
  MediaStreamTrack* CreateDOMTrack(TrackID aTrackID, MediaSegment::Type aType);
  MediaStreamTrack* GetDOMTrackFor(TrackID aTrackID);

  class OnTracksAvailableCallback {
  public:
    virtual ~OnTracksAvailableCallback() {}
    virtual void NotifyTracksAvailable(DOMMediaStream* aStream) = 0;
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

  


  void DisconnectTrackListListeners(const AudioTrackList* aAudioTrackList,
                                    const VideoTrackList* aVideoTrackList);

  virtual void NotifyMediaStreamTrackCreated(MediaStreamTrack* aTrack);

  virtual void NotifyMediaStreamTrackEnded(MediaStreamTrack* aTrack);

protected:
  virtual ~DOMMediaStream();

  void Destroy();
  void InitSourceStream(nsIDOMWindow* aWindow,
                        MediaStreamGraph* aGraph = nullptr);
  void InitTrackUnionStream(nsIDOMWindow* aWindow,
                            MediaStreamGraph* aGraph = nullptr);
  void InitStreamCommon(MediaStream* aStream);
  already_AddRefed<AudioTrack> CreateAudioTrack(AudioStreamTrack* aStreamTrack);
  already_AddRefed<VideoTrack> CreateVideoTrack(VideoStreamTrack* aStreamTrack);

  
  
  void TracksCreated();

  void CheckTracksAvailable();

  class StreamListener;
  friend class StreamListener;

  
  StreamTime mLogicalStreamStartTime;

  
  nsCOMPtr<nsIDOMWindow> mWindow;

  
  
  MediaStream* mStream;

  nsAutoTArray<nsRefPtr<MediaStreamTrack>,2> mTracks;
  nsRefPtr<StreamListener> mListener;

  nsTArray<nsAutoPtr<OnTracksAvailableCallback> > mRunOnTracksAvailable;

  
  bool mTracksCreated;

  nsString mID;

  
  nsTArray<nsCOMPtr<nsISupports> > mConsumersToKeepAlive;

  bool mNotifiedOfMediaStreamGraphShutdown;

  
  
  nsTArray<MediaTrackListListener> mMediaTrackListListeners;

private:
  void NotifyPrincipalChanged();

  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsTArray<PrincipalChangeObserver*> mPrincipalChangeObservers;
  
  
  nsAutoPtr<PeerIdentity> mPeerIdentity;
  CORSMode mCORSMode;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMMediaStream,
                              NS_DOMMEDIASTREAM_IID)

#define NS_DOMLOCALMEDIASTREAM_IID \
{ 0xb1437260, 0xec61, 0x4dfa, \
  { 0x92, 0x54, 0x04, 0x44, 0xe2, 0xb5, 0x94, 0x9c } }

class DOMLocalMediaStream : public DOMMediaStream
{
public:
  DOMLocalMediaStream() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DOMLOCALMEDIASTREAM_IID)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual void Stop();

  virtual MediaEngineSource* GetMediaEngine(TrackID aTrackID) { return nullptr; }

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow,
                     MediaStreamGraph* aGraph = nullptr);

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow,
                         MediaStreamGraph* aGraph = nullptr);

protected:
  virtual ~DOMLocalMediaStream();
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMLocalMediaStream,
                              NS_DOMLOCALMEDIASTREAM_IID)

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
                         MediaStreamGraph* aGraph = nullptr);

protected:
  ~DOMAudioNodeMediaStream();

private:
  
  
  nsRefPtr<AudioNode> mStreamNode;
};

}

#endif 
