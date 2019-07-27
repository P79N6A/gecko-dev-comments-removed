



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

  virtual ~SourceStreamInfo() {}

  DOMMediaStream* GetMediaStream() const {
    return mMediaStream;
  }

  nsresult StorePipeline(const std::string& trackId,
                         const RefPtr<MediaPipeline>& aPipeline);

  nsresult StoreConduit(const std::string& trackId,
                        RefPtr<MediaSessionConduit> aConduit);

  virtual void AddTrack(const std::string& trackId) { mTracks.insert(trackId); }
  void RemoveTrack(const std::string& trackId);
  bool HasTrack(const std::string& trackId) const
  {
    return !!mTracks.count(trackId);
  }
  size_t GetTrackCount() const { return mTracks.size(); }

  
  
  const std::map<std::string, RefPtr<MediaPipeline>>&
  GetPipelines() const { return mPipelines; }
  RefPtr<MediaPipeline> GetPipelineByTrackId_m(const std::string& trackId);
  RefPtr<MediaSessionConduit> GetConduitByTrackId_m(const std::string& trackId);
  const std::string& GetId() const { return mId; }

  void DetachTransport_s();
  void DetachMedia_m();
  bool AnyCodecHasPluginID(uint64_t aPluginID);
protected:
  nsRefPtr<DOMMediaStream> mMediaStream;
  PeerConnectionMedia *mParent;
  const std::string mId;
  
  
  std::set<std::string> mTracks;
  std::map<std::string, RefPtr<MediaPipeline>> mPipelines;
  std::map<std::string, RefPtr<MediaSessionConduit>> mConduits;
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
                            const PeerIdentity* aSinkIdentity);
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

  virtual void AddTrack(const std::string& track) MOZ_OVERRIDE
  {
    mTrackIdMap.push_back(track);
    SourceStreamInfo::AddTrack(track);
  }

  TrackID GetNumericTrackId(const std::string& trackId) const
  {
    for (size_t i = 0; i < mTrackIdMap.size(); ++i) {
      if (mTrackIdMap[i] == trackId) {
        return static_cast<TrackID>(i + 1);
      }
    }
    return TRACK_INVALID;
  }

  nsresult GetTrackId(TrackID numericTrackId, std::string* trackId) const
  {
    if (numericTrackId <= 0 ||
        static_cast<size_t>(numericTrackId) > mTrackIdMap.size()) {
      return NS_ERROR_INVALID_ARG;;
    }

    *trackId = mTrackIdMap[numericTrackId - 1];
    return NS_OK;
  }

 private:
  
  
  
  
  
  
  
  std::vector<std::string> mTrackIdMap;
};

class PeerConnectionMedia : public sigslot::has_slots<> {
  ~PeerConnectionMedia() {}

 public:
  explicit PeerConnectionMedia(PeerConnectionImpl *parent);

  PeerConnectionImpl* GetPC() { return mParent; }
  nsresult Init(const std::vector<NrIceStunServer>& stun_servers,
                const std::vector<NrIceTurnServer>& turn_servers);
  
  void SelfDestruct();

  
  void SetAllowIceLoopback(bool val) { mAllowIceLoopback = val; }

  RefPtr<NrIceCtx> ice_ctx() const { return mIceCtx; }

  RefPtr<NrIceMediaStream> ice_media_stream(size_t i) const {
    
    
    if (i >= mIceStreams.size()) {
      return nullptr;
    }
    return mIceStreams[i];
  }

  size_t num_ice_media_streams() const {
    return mIceStreams.size();
  }

  
  void UpdateTransports(const JsepSession& session);

  
  void StartIceChecks(const JsepSession& session);

  
  void AddIceCandidate(const std::string& candidate, const std::string& mid,
                       uint32_t aMLine);

  
  nsresult UpdateMediaPipelines(const JsepSession& session);

  
  
  
  nsresult AddTrack(DOMMediaStream* aMediaStream,
                    std::string* streamId,
                    const std::string& trackId);

  nsresult RemoveLocalTrack(const std::string& streamId,
                            const std::string& trackId);
  nsresult RemoveRemoteTrack(const std::string& streamId,
                            const std::string& trackId);

  nsresult GetRemoteTrackId(DOMMediaStream* mediaStream,
                            TrackID numericTrackId,
                            std::string* trackId) const;

  
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

  
  nsresult AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo);

#ifdef MOZILLA_INTERNAL_API
  
  
  
  void UpdateSinkIdentity_m(nsIPrincipal* aPrincipal,
                            const PeerIdentity* aSinkIdentity);
  
  bool AnyLocalStreamHasPeerIdentity() const;
  
  
  void UpdateRemoteStreamPrincipals_m(nsIPrincipal* aPrincipal);
#endif

  bool AnyCodecHasPluginID(uint64_t aPluginID);

  const nsCOMPtr<nsIThread>& GetMainThread() const { return mMainThread; }
  const nsCOMPtr<nsIEventTarget>& GetSTSThread() const { return mSTSThread; }

  
  
  RefPtr<TransportFlow> GetTransportFlow(int aStreamIndex,
                                                           bool aIsRtcp) {
    int index_inner = aStreamIndex * 2 + (aIsRtcp ? 1 : 0);

    if (mTransportFlows.find(index_inner) == mTransportFlows.end())
      return nullptr;

    return mTransportFlows[index_inner];
  }

  
  void AddTransportFlow(int aIndex, bool aRtcp,
                        const RefPtr<TransportFlow> &aFlow);
  void ConnectDtlsListener_s(const RefPtr<TransportFlow>& aFlow);
  void DtlsConnected_s(TransportLayer* aFlow,
                       TransportLayer::State state);
  static void DtlsConnected_m(const std::string& aParentHandle,
                              bool aPrivacyRequested);

  RefPtr<MediaSessionConduit> GetConduit(const std::string& streamId,
                                         const std::string& trackId,
                                         bool aReceive) {
    SourceStreamInfo* info;
    if (aReceive) {
      info = GetRemoteStreamById(streamId);
    } else {
      info = GetLocalStreamById(streamId);
    }

    if (!info) {
      MOZ_ASSERT(false);
      return nullptr;
    }

    return info->GetConduitByTrackId_m(trackId);
  }

  
  void AddConduit(const std::string& streamId,
                  const std::string& trackId,
                  bool aReceive,
                  const RefPtr<MediaSessionConduit> &aConduit) {
    SourceStreamInfo* info;
    if (aReceive) {
      info = GetRemoteStreamById(streamId);
    } else {
      info = GetLocalStreamById(streamId);
    }

    if (!info) {
      MOZ_ASSERT(false);
      return;
    }

    info->StoreConduit(trackId, aConduit);
  }

  
  sigslot::signal2<NrIceCtx*, NrIceCtx::GatheringState>
      SignalIceGatheringStateChange;
  sigslot::signal2<NrIceCtx*, NrIceCtx::ConnectionState>
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


  
  void IceGatheringStateChange_s(NrIceCtx* ctx,
                               NrIceCtx::GatheringState state);
  void IceConnectionStateChange_s(NrIceCtx* ctx,
                                NrIceCtx::ConnectionState state);
  void IceStreamReady_s(NrIceMediaStream *aStream);
  void OnCandidateFound_s(NrIceMediaStream *aStream,
                        const std::string &candidate);
  void EndOfLocalCandidates(const std::string& aDefaultAddr,
                            uint16_t aDefaultPort,
                            uint16_t aMLine);

  void IceGatheringStateChange_m(NrIceCtx* ctx,
                                 NrIceCtx::GatheringState state);
  void IceConnectionStateChange_m(NrIceCtx* ctx,
                                  NrIceCtx::ConnectionState state);
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

  
  RefPtr<NrIceCtx> mIceCtx;
  std::vector<RefPtr<NrIceMediaStream> > mIceStreams;

  
  nsRefPtr<NrIceResolver> mDNSResolver;

  
  std::map<int, RefPtr<TransportFlow> > mTransportFlows;

  
  UniquePtr<PCUuidGenerator> mUuidGen;

  
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
