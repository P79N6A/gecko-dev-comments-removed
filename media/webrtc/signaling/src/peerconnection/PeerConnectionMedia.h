



#ifndef _PEER_CONNECTION_MEDIA_H_
#define _PEER_CONNECTION_MEDIA_H_

#include <string>
#include <vector>
#include <map>

#include "nspr.h"
#include "prlock.h"

#include "mozilla/RefPtr.h"
#include "nsComponentManagerUtils.h"

#ifdef USE_FAKE_MEDIA_STREAMS
#include "FakeMediaStreams.h"
#else
#include "DOMMediaStream.h"
#include "MediaSegment.h"
#endif

#include "signaling/src/jsep/JsepSession.h"
#include "AudioSegment.h"

#ifdef MOZILLA_INTERNAL_API
#include "Layers.h"
#include "VideoUtils.h"
#include "ImageLayers.h"
#include "VideoSegment.h"
#endif

class nsIPrincipal;

namespace mozilla {
class DataChannel;
class PeerIdentity;
class MediaPipelineFactory;
namespace dom {
struct RTCInboundRTPStreamStats;
struct RTCOutboundRTPStreamStats;
}
}

#include "nricectx.h"
#include "nriceresolver.h"
#include "nricemediastream.h"
#include "MediaPipeline.h"

namespace mozilla {

class PeerConnectionImpl;
class PeerConnectionMedia;
class PCUuidGenerator;


class Fake_AudioGenerator {
 public:
  explicit Fake_AudioGenerator(DOMMediaStream* aStream) : mStream(aStream), mCount(0) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    MOZ_ASSERT(mTimer);

    
    mozilla::AudioSegment *segment = new mozilla::AudioSegment();
    mStream->GetStream()->AsSourceStream()->AddAudioTrack(1, 16000, 0, segment);

    
    mTimer->InitWithFuncCallback(Callback, this, 100, nsITimer::TYPE_REPEATING_PRECISE);
  }

  static void Callback(nsITimer* timer, void *arg) {
    Fake_AudioGenerator* gen = static_cast<Fake_AudioGenerator*>(arg);

    nsRefPtr<mozilla::SharedBuffer> samples = mozilla::SharedBuffer::Create(1600 * sizeof(int16_t));
    int16_t* data = static_cast<int16_t*>(samples->Data());
    for (int i=0; i<1600; i++) {
      data[i] = ((gen->mCount % 8) * 4000) - (7*4000)/2;
      ++gen->mCount;
    }

    mozilla::AudioSegment segment;
    nsAutoTArray<const int16_t*,1> channelData;
    channelData.AppendElement(data);
    segment.AppendFrames(samples.forget(), channelData, 1600);
    gen->mStream->GetStream()->AsSourceStream()->AppendToTrack(1, &segment);
  }

 private:
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<DOMMediaStream> mStream;
  int mCount;
};


#ifdef MOZILLA_INTERNAL_API
class Fake_VideoGenerator {
 public:
  typedef mozilla::gfx::IntSize IntSize;

  explicit Fake_VideoGenerator(DOMMediaStream* aStream) {
    mStream = aStream;
    mCount = 0;
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    MOZ_ASSERT(mTimer);

    
    mozilla::VideoSegment *segment = new mozilla::VideoSegment();
    mStream->GetStream()->AsSourceStream()->AddTrack(1, 0, segment);
    mStream->GetStream()->AsSourceStream()->AdvanceKnownTracksTime(mozilla::STREAM_TIME_MAX);

    
    mTimer->InitWithFuncCallback(Callback, this, 100, nsITimer::TYPE_REPEATING_SLACK);
  }

  static void Callback(nsITimer* timer, void *arg) {
    Fake_VideoGenerator* gen = static_cast<Fake_VideoGenerator*>(arg);

    const uint32_t WIDTH = 640;
    const uint32_t HEIGHT = 480;

    
    nsRefPtr<mozilla::layers::ImageContainer> container =
      mozilla::layers::LayerManager::CreateImageContainer();

    nsRefPtr<mozilla::layers::Image> image =
      container->CreateImage(mozilla::ImageFormat::PLANAR_YCBCR);

    int len = ((WIDTH * HEIGHT) * 3 / 2);
    mozilla::layers::PlanarYCbCrImage* planar =
      static_cast<mozilla::layers::PlanarYCbCrImage*>(image.get());
    uint8_t* frame = (uint8_t*) PR_Malloc(len);
    ++gen->mCount;
    memset(frame, (gen->mCount / 8) & 0xff, len); 

    const uint8_t lumaBpp = 8;
    const uint8_t chromaBpp = 4;

    mozilla::layers::PlanarYCbCrData data;
    data.mYChannel = frame;
    data.mYSize = IntSize(WIDTH, HEIGHT);
    data.mYStride = (int32_t) (WIDTH * lumaBpp / 8.0);
    data.mCbCrStride = (int32_t) (WIDTH * chromaBpp / 8.0);
    data.mCbChannel = frame + HEIGHT * data.mYStride;
    data.mCrChannel = data.mCbChannel + HEIGHT * data.mCbCrStride / 2;
    data.mCbCrSize = IntSize(WIDTH / 2, HEIGHT / 2);
    data.mPicX = 0;
    data.mPicY = 0;
    data.mPicSize = IntSize(WIDTH, HEIGHT);
    data.mStereoMode = mozilla::StereoMode::MONO;

    
    planar->SetData(data);
    PR_Free(frame);

    
    mozilla::VideoSegment *segment = new mozilla::VideoSegment();
    
    segment->AppendFrame(image.forget(),
                         gen->mStream->GetStream()->GraphRate() / 10,
                         IntSize(WIDTH, HEIGHT));

    gen->mStream->GetStream()->AsSourceStream()->AppendToTrack(1, segment);
  }

 private:
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<DOMMediaStream> mStream;
  int mCount;
};
#endif


class SourceStreamInfo {
public:
  SourceStreamInfo(DOMMediaStream* aMediaStream,
                   PeerConnectionMedia *aParent,
                   const std::string& aId)
      : mMediaStream(aMediaStream),
        mParent(aParent),
        mId(aId) {
    MOZ_ASSERT(mMediaStream);
  }

  SourceStreamInfo(already_AddRefed<DOMMediaStream>& aMediaStream,
                   PeerConnectionMedia *aParent,
                   const std::string& aId)
      : mMediaStream(aMediaStream),
        mParent(aParent),
        mId(aId) {
    MOZ_ASSERT(mMediaStream);
  }

  DOMMediaStream* GetMediaStream() const {
    return mMediaStream;
  }

  
  
  const std::map<mozilla::TrackID, mozilla::RefPtr<mozilla::MediaPipeline>>&
  GetPipelines() const { return mPipelines; }
  mozilla::RefPtr<mozilla::MediaPipeline> GetPipelineByLevel_m(int aMLine);
  const std::string& GetId() const { return mId; }

protected:
  std::map<mozilla::TrackID, mozilla::RefPtr<mozilla::MediaPipeline>> mPipelines;
  nsRefPtr<DOMMediaStream> mMediaStream;
  PeerConnectionMedia *mParent;
  const std::string mId;
};



class LocalSourceStreamInfo : public SourceStreamInfo {
  ~LocalSourceStreamInfo() {
    mMediaStream = nullptr;
  }
public:
  LocalSourceStreamInfo(DOMMediaStream *aMediaStream,
                        PeerConnectionMedia *aParent,
                        const std::string& aId)
     : SourceStreamInfo(aMediaStream, aParent, aId) {}

  
#if 0
  int HasTrack(DOMMediaStream* aStream, mozilla::TrackID aMLine);
#endif
  int HasTrackType(DOMMediaStream* aStream, bool aIsVideo);
  
  
  
  
  nsresult ReplaceTrack(int aMLine, DOMMediaStream* aNewStream, mozilla::TrackID aNewTrack);

  void StorePipeline(int aMLine,
                     mozilla::RefPtr<mozilla::MediaPipelineTransmit> aPipeline);

#ifdef MOZILLA_INTERNAL_API
  void UpdateSinkIdentity_m(nsIPrincipal* aPrincipal,
                            const mozilla::PeerIdentity* aSinkIdentity);
#endif

  void ExpectAudio(const mozilla::TrackID);
  void ExpectVideo(const mozilla::TrackID);
  void RemoveAudio(const mozilla::TrackID);
  void RemoveVideo(const mozilla::TrackID);
  unsigned AudioTrackCount();
  unsigned VideoTrackCount();
  void DetachTransport_s();
  void DetachMedia_m();

  bool AnyCodecHasPluginID(uint64_t aPluginID);

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(LocalSourceStreamInfo)
private:
  nsTArray<mozilla::TrackID> mAudioTracks;
  nsTArray<mozilla::TrackID> mVideoTracks;
};

class RemoteSourceStreamInfo : public SourceStreamInfo {
  ~RemoteSourceStreamInfo() {}
 public:
  RemoteSourceStreamInfo(already_AddRefed<DOMMediaStream> aMediaStream,
                         PeerConnectionMedia *aParent,
                         const std::string& aId)
    : SourceStreamInfo(aMediaStream, aParent, aId),
      mTrackTypeHints(0) {
  }

  void StorePipeline(int aMLine, bool aIsVideo,
                     mozilla::RefPtr<mozilla::MediaPipelineReceive> aPipeline);

  bool SetUsingBundle_m(int aMLine, bool decision);

  void DetachTransport_s();
  void DetachMedia_m();

#ifdef MOZILLA_INTERNAL_API
  void UpdatePrincipal_m(nsIPrincipal* aPrincipal);
#endif

  bool AnyCodecHasPluginID(uint64_t aPluginID);

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RemoteSourceStreamInfo)

public:
  DOMMediaStream::TrackTypeHints mTrackTypeHints;
 private:
  std::map<int, bool> mTypes;
};

class PeerConnectionMedia : public sigslot::has_slots<> {
  ~PeerConnectionMedia() {}

 public:
  explicit PeerConnectionMedia(PeerConnectionImpl *parent);

  PeerConnectionImpl* GetPC() { return mParent; }
  nsresult Init(const std::vector<mozilla::NrIceStunServer>& stun_servers,
                const std::vector<mozilla::NrIceTurnServer>& turn_servers);
  
  void SelfDestruct();

  
  void SetAllowIceLoopback(bool val) { mAllowIceLoopback = val; }

  mozilla::RefPtr<mozilla::NrIceCtx> ice_ctx() const { return mIceCtx; }

  mozilla::RefPtr<mozilla::NrIceMediaStream> ice_media_stream(size_t i) const {
    
    
    if (i >= mIceStreams.size()) {
      return nullptr;
    }
    return mIceStreams[i];
  }

  size_t num_ice_media_streams() const {
    return mIceStreams.size();
  }

  
  void UpdateTransports(const mozilla::JsepSession& session);

  
  void StartIceChecks(const mozilla::JsepSession& session);

  
  void AddIceCandidate(const std::string& candidate, const std::string& mid,
                       uint32_t aMLine);

  
  nsresult UpdateMediaPipelines(const mozilla::JsepSession& session);

  
  nsresult AddStream(DOMMediaStream* aMediaStream, uint32_t hints,
                     std::string* stream_id);

  
  nsresult RemoveStream(DOMMediaStream* aMediaStream,
                        uint32_t hints,
                        uint32_t *stream_id);

  
  uint32_t LocalStreamsLength()
  {
    return mLocalSourceStreams.Length();
  }
  LocalSourceStreamInfo* GetLocalStreamByIndex(int index);
  LocalSourceStreamInfo* GetLocalStreamById(const std::string& id);

  
  uint32_t RemoteStreamsLength()
  {
    return mRemoteSourceStreams.Length();
  }

  RemoteSourceStreamInfo* GetRemoteStreamByIndex(size_t index);
  RemoteSourceStreamInfo* GetRemoteStreamById(const std::string& id);

  bool SetUsingBundle_m(int aMLine, bool decision);
  bool UpdateFilterFromRemoteDescription_m(
      int aMLine,
      nsAutoPtr<mozilla::MediaPipelineFilter> filter);

  
  nsresult AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo);
  nsresult AddRemoteStreamHint(int aIndex, bool aIsVideo);

#ifdef MOZILLA_INTERNAL_API
  
  
  
  void UpdateSinkIdentity_m(nsIPrincipal* aPrincipal,
                            const mozilla::PeerIdentity* aSinkIdentity);
  
  bool AnyLocalStreamHasPeerIdentity() const;
  
  
  void UpdateRemoteStreamPrincipals_m(nsIPrincipal* aPrincipal);
#endif

  bool AnyCodecHasPluginID(uint64_t aPluginID);

  const nsCOMPtr<nsIThread>& GetMainThread() const { return mMainThread; }
  const nsCOMPtr<nsIEventTarget>& GetSTSThread() const { return mSTSThread; }

  
  
  mozilla::RefPtr<mozilla::TransportFlow> GetTransportFlow(int aStreamIndex,
                                                           bool aIsRtcp) {
    int index_inner = aStreamIndex * 2 + (aIsRtcp ? 1 : 0);

    if (mTransportFlows.find(index_inner) == mTransportFlows.end())
      return nullptr;

    return mTransportFlows[index_inner];
  }

  
  void AddTransportFlow(int aIndex, bool aRtcp,
                        const mozilla::RefPtr<mozilla::TransportFlow> &aFlow);
  void ConnectDtlsListener_s(const mozilla::RefPtr<mozilla::TransportFlow>& aFlow);
  void DtlsConnected_s(mozilla::TransportLayer* aFlow,
                       mozilla::TransportLayer::State state);
  static void DtlsConnected_m(const std::string& aParentHandle,
                              bool aPrivacyRequested);

  mozilla::RefPtr<mozilla::MediaSessionConduit> GetConduit(int aStreamIndex, bool aReceive) {
    int index_inner = aStreamIndex * 2 + (aReceive ? 0 : 1);

    if (mConduits.find(index_inner) == mConduits.end())
      return nullptr;

    return mConduits[index_inner];
  }

  
  void AddConduit(int aIndex, bool aReceive,
                  const mozilla::RefPtr<mozilla::MediaSessionConduit> &aConduit) {
    int index_inner = aIndex * 2 + (aReceive ? 0 : 1);

    MOZ_ASSERT(!mConduits[index_inner]);
    mConduits[index_inner] = aConduit;
  }

  
  sigslot::signal2<mozilla::NrIceCtx*, mozilla::NrIceCtx::GatheringState>
      SignalIceGatheringStateChange;
  sigslot::signal2<mozilla::NrIceCtx*, mozilla::NrIceCtx::ConnectionState>
      SignalIceConnectionStateChange;
  
  sigslot::signal2<const std::string&, uint16_t> SignalCandidate;
  
  sigslot::signal3<const std::string&, uint16_t, uint16_t>
      SignalEndOfLocalCandidates;

 private:
  
  void ShutdownMediaTransport_s();

  
  
  void SelfDestruct_m();

  
  void UpdateIceMediaStream_s(size_t aMLine, size_t aComponentCount,
                              bool aHasAttrs,
                              const std::string& aUfrag,
                              const std::string& aPassword,
                              const std::vector<std::string>& aCandidateList);
  void EnsureIceGathering_s();
  void StartIceChecks_s(bool aIsControlling,
                        bool aIsIceLite,
                        const std::vector<std::string>& aIceOptionsList,
                        const std::vector<size_t>& aComponentCountByLevel);

  
  void AddIceCandidate_s(const std::string& aCandidate, const std::string& aMid,
                         uint32_t aMLine);


  
  void IceGatheringStateChange_s(mozilla::NrIceCtx* ctx,
                               mozilla::NrIceCtx::GatheringState state);
  void IceConnectionStateChange_s(mozilla::NrIceCtx* ctx,
                                mozilla::NrIceCtx::ConnectionState state);
  void IceStreamReady_s(mozilla::NrIceMediaStream *aStream);
  void OnCandidateFound_s(mozilla::NrIceMediaStream *aStream,
                        const std::string &candidate);
  void EndOfLocalCandidates(const std::string& aDefaultAddr,
                            uint16_t aDefaultPort,
                            uint16_t aMLine);

  void IceGatheringStateChange_m(mozilla::NrIceCtx* ctx,
                                 mozilla::NrIceCtx::GatheringState state);
  void IceConnectionStateChange_m(mozilla::NrIceCtx* ctx,
                                  mozilla::NrIceCtx::ConnectionState state);
  void OnCandidateFound_m(const std::string &candidate, uint16_t aMLine);
  void EndOfLocalCandidates_m(const std::string& aDefaultAddr,
                              uint16_t aDefaultPort,
                              uint16_t aMLine);


  
  PeerConnectionImpl *mParent;
  
  std::string mParentHandle;
  std::string mParentName;

  
  
  nsTArray<nsRefPtr<LocalSourceStreamInfo> > mLocalSourceStreams;

  
  
  nsTArray<nsRefPtr<RemoteSourceStreamInfo> > mRemoteSourceStreams;

  
  bool mAllowIceLoopback;

  
  mozilla::RefPtr<mozilla::NrIceCtx> mIceCtx;
  std::vector<mozilla::RefPtr<mozilla::NrIceMediaStream> > mIceStreams;

  
  nsRefPtr<mozilla::NrIceResolver> mDNSResolver;

  
  std::map<int, mozilla::RefPtr<mozilla::TransportFlow> > mTransportFlows;

  
  
  std::map<int, mozilla::RefPtr<mozilla::MediaSessionConduit> > mConduits;

  
  mozilla::UniquePtr<PCUuidGenerator> mUuidGen;

  
  nsCOMPtr<nsIThread> mMainThread;

  
  nsCOMPtr<nsIEventTarget> mSTSThread;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PeerConnectionMedia)
};

}  
#endif
