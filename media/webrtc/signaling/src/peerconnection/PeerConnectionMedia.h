



#ifndef _PEER_CONNECTION_MEDIA_H_
#define _PEER_CONNECTION_MEDIA_H_

#include <string>
#include <vector>
#include <map>

#include "nspr.h"
#include "prlock.h"

#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIProtocolProxyCallback.h"

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

  nsresult StorePipeline(
      const std::string& trackId,
      const mozilla::RefPtr<mozilla::MediaPipeline>& aPipeline);

  void AddTrack(const std::string& trackId) { mTracks.insert(trackId); }
  void RemoveTrack(const std::string& trackId) { mTracks.erase(trackId); }
  bool HasTrack(const std::string& trackId) const
  {
    return !!mTracks.count(trackId);
  }
  size_t GetTrackCount() const { return mTracks.size(); }

  
  
  const std::map<std::string, mozilla::RefPtr<mozilla::MediaPipeline>>&
  GetPipelines() const { return mPipelines; }
  mozilla::RefPtr<mozilla::MediaPipeline> GetPipelineByTrackId_m(
      const std::string& trackId);
  const std::string& GetId() const { return mId; }

  void DetachTransport_s();
  void DetachMedia_m();
  bool AnyCodecHasPluginID(uint64_t aPluginID);
protected:
  nsRefPtr<DOMMediaStream> mMediaStream;
  PeerConnectionMedia *mParent;
  const std::string mId;
  
  
  std::set<std::string> mTracks;
  
  std::map<std::string, mozilla::RefPtr<mozilla::MediaPipeline>> mPipelines;
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

  
  
  
  nsresult ReplaceTrack(const std::string& oldTrackId,
                        DOMMediaStream* aNewStream,
                        const std::string& aNewTrack);

#ifdef MOZILLA_INTERNAL_API
  void UpdateSinkIdentity_m(nsIPrincipal* aPrincipal,
                            const mozilla::PeerIdentity* aSinkIdentity);
#endif

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(LocalSourceStreamInfo)
};

class RemoteSourceStreamInfo : public SourceStreamInfo {
  ~RemoteSourceStreamInfo() {}
 public:
  RemoteSourceStreamInfo(already_AddRefed<DOMMediaStream> aMediaStream,
                         PeerConnectionMedia *aParent,
                         const std::string& aId)
    : SourceStreamInfo(aMediaStream, aParent, aId)
  {
  }

  void SyncPipeline(RefPtr<MediaPipelineReceive> aPipeline);

#ifdef MOZILLA_INTERNAL_API
  void UpdatePrincipal_m(nsIPrincipal* aPrincipal);
#endif

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RemoteSourceStreamInfo)
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

  
  
  
  nsresult AddTrack(DOMMediaStream* aMediaStream,
                    std::string* streamId,
                    const std::string& trackId);

  
  
  
  nsresult RemoveTrack(DOMMediaStream* aMediaStream,
                       const std::string& trackId);

  
  uint32_t LocalStreamsLength()
  {
    return mLocalSourceStreams.Length();
  }
  LocalSourceStreamInfo* GetLocalStreamByIndex(int index);
  LocalSourceStreamInfo* GetLocalStreamById(const std::string& id);
  LocalSourceStreamInfo* GetLocalStreamByDomStream(
      const DOMMediaStream& stream);

  
  uint32_t RemoteStreamsLength()
  {
    return mRemoteSourceStreams.Length();
  }

  RemoteSourceStreamInfo* GetRemoteStreamByIndex(size_t index);
  RemoteSourceStreamInfo* GetRemoteStreamById(const std::string& id);
  RemoteSourceStreamInfo* GetRemoteStreamByDomStream(
      const DOMMediaStream& stream);


  bool UpdateFilterFromRemoteDescription_m(
      const std::string& trackId,
      nsAutoPtr<mozilla::MediaPipelineFilter> filter);

  
  nsresult AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo);

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
  class ProtocolProxyQueryHandler : public nsIProtocolProxyCallback {
   public:
    explicit ProtocolProxyQueryHandler(PeerConnectionMedia *pcm) :
      pcm_(pcm) {}

    NS_IMETHODIMP OnProxyAvailable(nsICancelable *request,
                                   nsIChannel *aChannel,
                                   nsIProxyInfo *proxyinfo,
                                   nsresult result) MOZ_OVERRIDE;
    NS_DECL_ISUPPORTS

   private:
      RefPtr<PeerConnectionMedia> pcm_;
      virtual ~ProtocolProxyQueryHandler() {}
  };

  
  void ShutdownMediaTransport_s();

  
  
  void SelfDestruct_m();

  
  void UpdateIceMediaStream_s(size_t aMLine, size_t aComponentCount,
                              bool aHasAttrs,
                              const std::string& aUfrag,
                              const std::string& aPassword,
                              const std::vector<std::string>& aCandidateList);
  void GatherIfReady();
  void FlushIceCtxOperationQueueIfReady();
  void PerformOrEnqueueIceCtxOperation(const nsRefPtr<nsIRunnable>& runnable);
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
  bool IsIceCtxReady() const {
    return mProxyResolveCompleted;
  }


  
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

  
  
  
  
  std::vector<nsRefPtr<nsIRunnable>> mQueuedIceCtxOperations;

  
  nsCOMPtr<nsICancelable> mProxyRequest;

  
  bool mProxyResolveCompleted;

  
  UniquePtr<NrIceProxyServer> mProxyServer;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PeerConnectionMedia)
};

}  
#endif
