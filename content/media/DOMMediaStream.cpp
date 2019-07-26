




#include "DOMMediaStream.h"
#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/LocalMediaStreamBinding.h"
#include "MediaStreamGraph.h"
#include "AudioStreamTrack.h"
#include "VideoStreamTrack.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMMediaStream)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaStream)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMMediaStream)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMMediaStream)
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(DOMMediaStream, mWindow, mTracks)

NS_IMPL_ISUPPORTS_INHERITED1(DOMLocalMediaStream, DOMMediaStream,
                             nsIDOMLocalMediaStream)

class DOMMediaStream::StreamListener : public MediaStreamListener {
public:
  StreamListener(DOMMediaStream* aStream)
    : mStream(aStream)
  {}

  
  void Forget() { mStream = nullptr; }
  DOMMediaStream* GetStream() { return mStream; }

  class TrackChange : public nsRunnable {
  public:
    TrackChange(StreamListener* aListener,
                TrackID aID, TrackTicks aTrackOffset,
                uint32_t aEvents, MediaSegment::Type aType)
      : mListener(aListener), mID(aID), mEvents(aEvents), mType(aType)
    {
    }

    NS_IMETHOD Run()
    {
      NS_ASSERTION(NS_IsMainThread(), "main thread only");

      DOMMediaStream* stream = mListener->GetStream();
      if (!stream) {
        return NS_OK;
      }

      nsRefPtr<MediaStreamTrack> track;
      if (mEvents & MediaStreamListener::TRACK_EVENT_CREATED) {
        track = stream->CreateDOMTrack(mID, mType);
      } else {
        track = stream->GetDOMTrackFor(mID);
      }
      if (mEvents & MediaStreamListener::TRACK_EVENT_ENDED) {
        track->NotifyEnded();
      }
      return NS_OK;
    }

    StreamTime mEndTime;
    nsRefPtr<StreamListener> mListener;
    TrackID mID;
    uint32_t mEvents;
    MediaSegment::Type mType;
  };

  






  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackRate aTrackRate,
                                        TrackTicks aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia)
  {
    if (aTrackEvents & (TRACK_EVENT_CREATED | TRACK_EVENT_ENDED)) {
      nsRefPtr<TrackChange> runnable =
        new TrackChange(this, aID, aTrackOffset, aTrackEvents,
                        aQueuedMedia.GetType());
      NS_DispatchToMainThread(runnable);
    }
  }

private:
  
  DOMMediaStream* mStream;
};

DOMMediaStream::DOMMediaStream()
  : mStream(nullptr), mHintContents(0)
{
  SetIsDOMBinding();
}

DOMMediaStream::~DOMMediaStream()
{
  if (mListener) {
    mListener->Forget();
  }
  if (mStream) {
    mStream->Destroy();
  }
}

JSObject*
DOMMediaStream::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return dom::MediaStreamBinding::Wrap(aCx, aScope, this);
}

double
DOMMediaStream::CurrentTime()
{
  return mStream ? MediaTimeToSeconds(mStream->GetCurrentTime()) : 0.0;
}

bool
DOMMediaStream::IsFinished()
{
  return !mStream || mStream->IsFinished();
}

void
DOMMediaStream::InitSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  mWindow = aWindow;
  SetHintContents(aHintContents);
  MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
  InitStreamCommon(gm->CreateSourceStream(this));
}

void
DOMMediaStream::InitTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  mWindow = aWindow;
  SetHintContents(aHintContents);
  MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
  InitStreamCommon(gm->CreateTrackUnionStream(this));
}

void
DOMMediaStream::InitStreamCommon(MediaStream* aStream)
{
  mStream = aStream;

  
  mListener = new StreamListener(this);
  aStream->AddListener(mListener);
}

already_AddRefed<DOMMediaStream>
DOMMediaStream::CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMMediaStream> stream = new DOMMediaStream();
  stream->InitSourceStream(aWindow, aHintContents);
  return stream.forget();
}

already_AddRefed<DOMMediaStream>
DOMMediaStream::CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMMediaStream> stream = new DOMMediaStream();
  stream->InitTrackUnionStream(aWindow, aHintContents);
  return stream.forget();
}

bool
DOMMediaStream::CombineWithPrincipal(nsIPrincipal* aPrincipal)
{
  return nsContentUtils::CombineResourcePrincipals(&mPrincipal, aPrincipal);
}

MediaStreamTrack*
DOMMediaStream::CreateDOMTrack(TrackID aTrackID, MediaSegment::Type aType)
{
  MediaStreamTrack* track;
  switch (aType) {
  case MediaSegment::AUDIO:
    track = new AudioStreamTrack(this, aTrackID);
    break;
  case MediaSegment::VIDEO:
    track = new VideoStreamTrack(this, aTrackID);
    break;
  default:
    MOZ_NOT_REACHED("Unhandled track type");
    return nullptr;
  }

  mTracks.AppendElement(track);
  return track;
}

MediaStreamTrack*
DOMMediaStream::GetDOMTrackFor(TrackID aTrackID)
{
  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    MediaStreamTrack* t = mTracks[i];
    
    
    if (t->GetTrackID() == aTrackID && t->GetStream() == this) {
      return t;
    }
  }
  return nullptr;
}

DOMLocalMediaStream::~DOMLocalMediaStream()
{
  if (mStream) {
    
    Stop();
  }
}

JSObject*
DOMLocalMediaStream::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return dom::LocalMediaStreamBinding::Wrap(aCx, aScope, this);
}

void
DOMLocalMediaStream::Stop()
{
  if (mStream && mStream->AsSourceStream()) {
    mStream->AsSourceStream()->EndAllTrackAndFinish();
  }
}

already_AddRefed<DOMLocalMediaStream>
DOMLocalMediaStream::CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMLocalMediaStream> stream = new DOMLocalMediaStream();
  stream->InitSourceStream(aWindow, aHintContents);
  return stream.forget();
}

already_AddRefed<DOMLocalMediaStream>
DOMLocalMediaStream::CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMLocalMediaStream> stream = new DOMLocalMediaStream();
  stream->InitTrackUnionStream(aWindow, aHintContents);
  return stream.forget();
}
