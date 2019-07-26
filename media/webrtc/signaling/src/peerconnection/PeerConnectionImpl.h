



#ifndef _PEER_CONNECTION_IMPL_H_
#define _PEER_CONNECTION_IMPL_H_

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <cmath>

#include "prlock.h"
#include "mozilla/RefPtr.h"
#include "IPeerConnection.h"
#include "nsComponentManagerUtils.h"
#include "nsPIDOMWindow.h"

#ifdef USE_FAKE_MEDIA_STREAMS
#include "FakeMediaStreams.h"
#else
#include "nsDOMMediaStream.h"
#endif

#include "dtlsidentity.h"
#include "nricectx.h"
#include "nricemediastream.h"

#include "peer_connection_types.h"
#include "CallControlManager.h"
#include "CC_Device.h"
#include "CC_Call.h"
#include "CC_Observer.h"
#include "MediaPipeline.h"


#ifdef MOZILLA_INTERNAL_API
#include "mozilla/net/DataChannel.h"
#include "Layers.h"
#include "VideoUtils.h"
#include "ImageLayers.h"
#include "VideoSegment.h"
#else
namespace mozilla {
  class DataChannel;
}
#endif

using namespace mozilla;

namespace sipcc {


class Fake_AudioGenerator {
 public:
Fake_AudioGenerator(nsDOMMediaStream* aStream) : mStream(aStream), mCount(0) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    PR_ASSERT(mTimer);

    
    mozilla::AudioSegment *segment = new mozilla::AudioSegment();
    segment->Init(1); 
    mStream->GetStream()->AsSourceStream()->AddTrack(1, 16000, 0, segment);

    
    mTimer->InitWithFuncCallback(Callback, this, 100, nsITimer::TYPE_REPEATING_PRECISE);
  }

  static void Callback(nsITimer* timer, void *arg) {
    Fake_AudioGenerator* gen = static_cast<Fake_AudioGenerator*>(arg);

    nsRefPtr<mozilla::SharedBuffer> samples = mozilla::SharedBuffer::Create(1600 * 2 * sizeof(int16_t));
    for (int i=0; i<1600*2; i++) {
      reinterpret_cast<int16_t *>(samples->Data())[i] = ((gen->mCount % 8) * 4000) - (7*4000)/2;
      ++gen->mCount;
    }

    mozilla::AudioSegment segment;
    segment.Init(1);
    segment.AppendFrames(samples.forget(), 1600,
      0, 1600, nsAudioStream::FORMAT_S16);

    gen->mStream->GetStream()->AsSourceStream()->AppendToTrack(1, &segment);
  }

 private:
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<nsDOMMediaStream> mStream;
  int mCount;
};


#ifdef MOZILLA_INTERNAL_API
class Fake_VideoGenerator {
 public:
  Fake_VideoGenerator(nsDOMMediaStream* aStream) {
    mStream = aStream;
    mCount = 0;
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    PR_ASSERT(mTimer);

    
    mozilla::VideoSegment *segment = new mozilla::VideoSegment();
    mStream->GetStream()->AsSourceStream()->AddTrack(1, USECS_PER_S, 0, segment);
    mStream->GetStream()->AsSourceStream()->AdvanceKnownTracksTime(mozilla::STREAM_TIME_MAX);

    
    mTimer->InitWithFuncCallback(Callback, this, 100, nsITimer::TYPE_REPEATING_SLACK);
  }

  static void Callback(nsITimer* timer, void *arg) {
    Fake_VideoGenerator* gen = static_cast<Fake_VideoGenerator*>(arg);

    const PRUint32 WIDTH = 640;
    const PRUint32 HEIGHT = 480;

    
    mozilla::ImageFormat format = mozilla::PLANAR_YCBCR;
    nsRefPtr<mozilla::layers::ImageContainer> container =
      mozilla::layers::LayerManager::CreateImageContainer();

    nsRefPtr<mozilla::layers::Image> image = container->CreateImage(&format, 1);

    int len = ((WIDTH * HEIGHT) * 3 / 2);
    mozilla::layers::PlanarYCbCrImage* planar =
      static_cast<mozilla::layers::PlanarYCbCrImage*>(image.get());
    PRUint8* frame = (PRUint8*) PR_Malloc(len);
    ++gen->mCount;
    memset(frame, (gen->mCount / 8) & 0xff, len); 

    const PRUint8 lumaBpp = 8;
    const PRUint8 chromaBpp = 4;

    mozilla::layers::PlanarYCbCrImage::Data data;
    data.mYChannel = frame;
    data.mYSize = gfxIntSize(WIDTH, HEIGHT);
    data.mYStride = (PRInt32) (WIDTH * lumaBpp / 8.0);
    data.mCbCrStride = (PRInt32) (WIDTH * chromaBpp / 8.0);
    data.mCbChannel = frame + HEIGHT * data.mYStride;
    data.mCrChannel = data.mCbChannel + HEIGHT * data.mCbCrStride / 2;
    data.mCbCrSize = gfxIntSize(WIDTH / 2, HEIGHT / 2);
    data.mPicX = 0;
    data.mPicY = 0;
    data.mPicSize = gfxIntSize(WIDTH, HEIGHT);
    data.mStereoMode = mozilla::STEREO_MODE_MONO;

    
    planar->SetData(data);
    PR_Free(frame);

    
    mozilla::VideoSegment *segment = new mozilla::VideoSegment();
    
    segment->AppendFrame(image.forget(), USECS_PER_S / 10, gfxIntSize(WIDTH, HEIGHT));

    gen->mStream->GetStream()->AsSourceStream()->AppendToTrack(1, segment);
  }

 private:
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<nsDOMMediaStream> mStream;
  int mCount;
};
#endif

class LocalSourceStreamInfo : public mozilla::MediaStreamListener {
public:
  LocalSourceStreamInfo(nsDOMMediaStream* aMediaStream)
    : mMediaStream(aMediaStream) {}
  ~LocalSourceStreamInfo() {
    mMediaStream = NULL;
  }

  





  virtual void NotifyQueuedTrackChanges(
    mozilla::MediaStreamGraph* aGraph,
    mozilla::TrackID aID,
    mozilla::TrackRate aTrackRate,
    mozilla::TrackTicks aTrackOffset,
    PRUint32 aTrackEvents,
    const mozilla::MediaSegment& aQueuedMedia
  );

  virtual void NotifyPull(mozilla::MediaStreamGraph* aGraph,
    mozilla::StreamTime aDesiredTime) {}

  nsDOMMediaStream* GetMediaStream() {
    return mMediaStream;
  }
  void StorePipeline(int aTrack, mozilla::RefPtr<mozilla::MediaPipeline> aPipeline);

  void ExpectAudio(const mozilla::TrackID);
  void ExpectVideo(const mozilla::TrackID);
  unsigned AudioTrackCount();
  unsigned VideoTrackCount();

  void Detach() {
    
    GetMediaStream()->GetStream()->RemoveListener(this);

    
    for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
           mPipelines.begin(); it != mPipelines.end();
         ++it) {
      it->second->DetachMediaStream();
    }
    mMediaStream = NULL;
  }

private:
  std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> > mPipelines;
  nsRefPtr<nsDOMMediaStream> mMediaStream;
  nsTArray<mozilla::TrackID> mAudioTracks;
  nsTArray<mozilla::TrackID> mVideoTracks;
};

class RemoteSourceStreamInfo {
 public:
  RemoteSourceStreamInfo(nsDOMMediaStream* aMediaStream) :
      mMediaStream(aMediaStream),
      mPipelines() {}

  nsDOMMediaStream* GetMediaStream() {
    return mMediaStream;
  }
  void StorePipeline(int aTrack, mozilla::RefPtr<mozilla::MediaPipeline> aPipeline);

  void Detach() {
    
    for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
           mPipelines.begin(); it != mPipelines.end();
         ++it) {
      it->second->DetachMediaStream();
    }
    mMediaStream = NULL;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RemoteSourceStreamInfo)
 private:
  nsRefPtr<nsDOMMediaStream> mMediaStream;
  std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> > mPipelines;
};

class PeerConnectionWrapper;

class PeerConnectionImpl MOZ_FINAL : public IPeerConnection,
#ifdef MOZILLA_INTERNAL_API
                                     public mozilla::DataChannelConnection::DataConnectionListener,
#endif
                                     public sigslot::has_slots<> {
public:
  PeerConnectionImpl();
  ~PeerConnectionImpl();

  enum ReadyState {
    kNew,
    kNegotiating,
    kActive,
    kClosing,
    kClosed
  };

  enum SipccState {
    kIdle,
    kStarting,
    kStarted
  };

  
  enum IceState {
    kIceGathering,
    kIceWaiting,
    kIceChecking,
    kIceConnected,
    kIceFailed
  };

  enum Role {
    kRoleUnknown,
    kRoleOfferer,
    kRoleAnswerer
  };

  NS_DECL_ISUPPORTS
  NS_DECL_IPEERCONNECTION

  static PeerConnectionImpl* CreatePeerConnection();
  static void Shutdown();

  Role GetRole() const { return mRole; }
  nsresult CreateRemoteSourceStreamInfo(PRUint32 aHint, RemoteSourceStreamInfo** aInfo);

  
  virtual void onCallEvent(
    ccapi_call_event_e aCallEvent,
    CSF::CC_CallPtr aCall,
    CSF::CC_CallInfoPtr aInfo
  );

  
  void NotifyConnection();
  void NotifyClosedConnection();
  void NotifyDataChannel(mozilla::DataChannel *aChannel);

  
  static PeerConnectionWrapper *AcquireInstance(const std::string& aHandle);
  virtual void ReleaseInstance();
  virtual const std::string& GetHandle();

  
  void IceGatheringCompleted(NrIceCtx *aCtx);
  void IceCompleted(NrIceCtx *aCtx);
  void IceStreamReady(NrIceMediaStream *aStream);

  mozilla::RefPtr<NrIceCtx> ice_ctx() const { return mIceCtx; }
  mozilla::RefPtr<NrIceMediaStream> ice_media_stream(size_t i) const {
    
    
    if (i >= mIceStreams.size()) {
      return NULL;
    }
    return mIceStreams[i];
  }

  
  nsRefPtr<LocalSourceStreamInfo> GetLocalStream(int aIndex);

  
  nsRefPtr<RemoteSourceStreamInfo> GetRemoteStream(int aIndex);

  
  nsresult AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo, int *aIndex);

  
  
  mozilla::RefPtr<TransportFlow> GetTransportFlow(int aStreamIndex, bool aIsRtcp) {
    int index_inner = aStreamIndex * 2 + (aIsRtcp ? 1 : 0);

    if (mTransportFlows.find(index_inner) == mTransportFlows.end())
      return NULL;

    return mTransportFlows[index_inner];
  }

  
  void AddTransportFlow(int aIndex, bool aRtcp, mozilla::RefPtr<TransportFlow> aFlow) {
    int index_inner = aIndex * 2 + (aRtcp ? 1 : 0);

    mTransportFlows[index_inner] = aFlow;
  }

  static void ListenThread(void *aData);
  static void ConnectThread(void *aData);

  
  nsCOMPtr<nsIThread> GetMainThread() { return mThread; }

  
  nsCOMPtr<nsIEventTarget> GetSTSThread() { return mSTSThread; }

  
  mozilla::RefPtr<DtlsIdentity> const GetIdentity() { return mIdentity; }

  
  nsresult CreateFakeMediaStream(PRUint32 hint, nsIDOMMediaStream** retval);

private:
  PeerConnectionImpl(const PeerConnectionImpl&rhs);
  PeerConnectionImpl& operator=(PeerConnectionImpl);

  void ChangeReadyState(ReadyState aReadyState);
  void CheckIceState() {
    PR_ASSERT(mIceState != kIceGathering);
  }

  
  void ShutdownMedia();

  
  
  void DisconnectMediaStreams();

  
  void ShutdownMediaTransport();

  nsresult MakeMediaStream(PRUint32 aHint, nsIDOMMediaStream** aStream);
  nsresult MakeRemoteSource(nsDOMMediaStream* aStream, RemoteSourceStreamInfo** aInfo);

  
  Role mRole;

  
  CSF::CC_CallPtr mCall;
  ReadyState mReadyState;


  nsCOMPtr<nsIThread> mThread;
  nsCOMPtr<IPeerConnectionObserver> mPCObserver;
  nsCOMPtr<nsPIDOMWindow> mWindow;

  
  std::string mLocalRequestedSDP;
  std::string mRemoteRequestedSDP;
  
  std::string mLocalSDP;
  std::string mRemoteSDP;

  
  std::string mFingerprint;
  std::string mRemoteFingerprint;

  
  PRLock *mLocalSourceStreamsLock;
  nsTArray<nsRefPtr<LocalSourceStreamInfo> > mLocalSourceStreams;

  
  PRLock *mRemoteSourceStreamsLock;
  nsTArray<nsRefPtr<RemoteSourceStreamInfo> > mRemoteSourceStreams;

  
  std::string mHandle;

  
  mozilla::RefPtr<NrIceCtx> mIceCtx;
  std::vector<mozilla::RefPtr<NrIceMediaStream> > mIceStreams;
  IceState mIceState;

  
  std::map<int, mozilla::RefPtr<TransportFlow> > mTransportFlows;

  
  mozilla::RefPtr<DtlsIdentity> mIdentity;

  
  nsCOMPtr<nsIEventTarget> mSTSThread;

#ifdef MOZILLA_INTERNAL_API
  
	nsRefPtr<mozilla::DataChannelConnection> mDataConnection;
#endif

  
  static std::map<const std::string, PeerConnectionImpl *> peerconnections;

public:
  
  unsigned short listenPort;
  unsigned short connectPort;
  char *connectStr; 
};


class PeerConnectionWrapper {
 public:
  PeerConnectionWrapper(PeerConnectionImpl *impl) : impl_(impl) {}

  ~PeerConnectionWrapper() {
    if (impl_)
      impl_->ReleaseInstance();
  }

  PeerConnectionImpl *impl() { return impl_; }

 private:
  PeerConnectionImpl *impl_;
};

}  

#endif  
