



#include <ostream>
#include <string>
#include <vector>

#include "CSFLog.h"

#include "nspr.h"

#include "nricectx.h"
#include "nricemediastream.h"
#include "MediaPipelineFactory.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionMedia.h"
#include "AudioConduit.h"
#include "VideoConduit.h"
#include "runnable_utils.h"
#include "transportlayerice.h"
#include "transportlayerdtls.h"
#include "signaling/src/jsep/JsepSession.h"
#include "signaling/src/jsep/JsepTransport.h"

#ifdef MOZILLA_INTERNAL_API
#include "MediaStreamList.h"
#include "nsIScriptGlobalObject.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/RTCStatsReportBinding.h"
#endif


using namespace mozilla::dom;

namespace mozilla {

static const char* logTag = "PeerConnectionMedia";
static const mozilla::TrackID TRACK_AUDIO = 0;
static const mozilla::TrackID TRACK_VIDEO = 1;





void
LocalSourceStreamInfo::ExpectAudio(const mozilla::TrackID aID)
{
  mAudioTracks.AppendElement(aID);
}

void
LocalSourceStreamInfo::RemoveAudio(const mozilla::TrackID aID)
{
  mAudioTracks.RemoveElement(aID);
}



void
LocalSourceStreamInfo::ExpectVideo(const mozilla::TrackID aID)
{
  mVideoTracks.AppendElement(aID);
}

void
LocalSourceStreamInfo::RemoveVideo(const mozilla::TrackID aID)
{
  mVideoTracks.RemoveElement(aID);
}

unsigned
LocalSourceStreamInfo::AudioTrackCount()
{
  return mAudioTracks.Length();
}

unsigned
LocalSourceStreamInfo::VideoTrackCount()
{
  return mVideoTracks.Length();
}

void LocalSourceStreamInfo::DetachTransport_s()
{
  ASSERT_ON_THREAD(mParent->GetSTSThread());
  
  
  for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
           mPipelines.begin(); it != mPipelines.end();
       ++it) {
    it->second->ShutdownTransport_s();
  }
}

void LocalSourceStreamInfo::DetachMedia_m()
{
  ASSERT_ON_THREAD(mParent->GetMainThread());
  
  
  for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
           mPipelines.begin(); it != mPipelines.end();
       ++it) {
    it->second->ShutdownMedia_m();
  }
  mAudioTracks.Clear();
  mVideoTracks.Clear();
  mMediaStream = nullptr;
}

#if 0


int LocalSourceStreamInfo::HasTrack(DOMMediaStream* aStream, TrackID aTrack)
{
  if (aStream != mMediaStream) {
    return -1;
  }
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
    if (it->second->trackid_locked() == aTrack) {
      return it->first;
    }
  }
  return -1;
}
#endif


int LocalSourceStreamInfo::HasTrackType(DOMMediaStream* aStream, bool aIsVideo)
{
  if (aStream != mMediaStream) {
    return -1;
  }
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
    if (it->second->IsVideo() == aIsVideo) {
      return it->first;
    }
  }
  return -1;
}


nsresult LocalSourceStreamInfo::ReplaceTrack(int aMLine,
                                             DOMMediaStream* aNewStream,
                                             TrackID aNewTrack)
{
  
  mozilla::RefPtr<mozilla::MediaPipeline> pipeline = mPipelines[aMLine];
  MOZ_ASSERT(pipeline);
  if (NS_SUCCEEDED(static_cast<mozilla::MediaPipelineTransmit*>(pipeline.get())->ReplaceTrack(aNewStream, aNewTrack))) {
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

void RemoteSourceStreamInfo::DetachTransport_s()
{
  ASSERT_ON_THREAD(mParent->GetSTSThread());
  
  
  for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
           mPipelines.begin(); it != mPipelines.end();
       ++it) {
    it->second->ShutdownTransport_s();
  }
}

void RemoteSourceStreamInfo::DetachMedia_m()
{
  ASSERT_ON_THREAD(mParent->GetMainThread());

  
  
  for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
         mPipelines.begin(); it != mPipelines.end();
       ++it) {
    it->second->ShutdownMedia_m();
  }
  mMediaStream = nullptr;
}

already_AddRefed<PeerConnectionImpl>
PeerConnectionImpl::Constructor(const dom::GlobalObject& aGlobal, ErrorResult& rv)
{
  nsRefPtr<PeerConnectionImpl> pc = new PeerConnectionImpl(&aGlobal);

  CSFLogDebug(logTag, "Created PeerConnection: %p", pc.get());

  return pc.forget();
}

PeerConnectionImpl* PeerConnectionImpl::CreatePeerConnection()
{
  PeerConnectionImpl *pc = new PeerConnectionImpl();

  CSFLogDebug(logTag, "Created PeerConnection: %p", pc);

  return pc;
}


PeerConnectionMedia::PeerConnectionMedia(PeerConnectionImpl *parent)
    : mParent(parent),
      mParentHandle(parent->GetHandle()),
      mParentName(parent->GetName()),
      mAllowIceLoopback(false),
      mIceCtx(nullptr),
      mDNSResolver(new mozilla::NrIceResolver()),
      mUuidGen(MakeUnique<PCUuidGenerator>()),
      mMainThread(mParent->GetMainThread()),
      mSTSThread(mParent->GetSTSThread()) {
}

nsresult PeerConnectionMedia::Init(const std::vector<NrIceStunServer>& stun_servers,
                                   const std::vector<NrIceTurnServer>& turn_servers)
{
  
  
  mIceCtx = NrIceCtx::Create("PC:" + mParentName,
                             true, 
                             true, 
                             mAllowIceLoopback);
  if(!mIceCtx) {
    CSFLogError(logTag, "%s: Failed to create Ice Context", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }
  nsresult rv;
  if (NS_FAILED(rv = mIceCtx->SetStunServers(stun_servers))) {
    CSFLogError(logTag, "%s: Failed to set stun servers", __FUNCTION__);
    return rv;
  }
  
#ifdef MOZILLA_INTERNAL_API
  bool disabled = Preferences::GetBool("media.peerconnection.turn.disable", false);
#else
  bool disabled = false;
#endif
  if (!disabled) {
    if (NS_FAILED(rv = mIceCtx->SetTurnServers(turn_servers))) {
      CSFLogError(logTag, "%s: Failed to set turn servers", __FUNCTION__);
      return rv;
    }
  } else if (turn_servers.size() != 0) {
    CSFLogError(logTag, "%s: Setting turn servers disabled", __FUNCTION__);
  }
  if (NS_FAILED(rv = mDNSResolver->Init())) {
    CSFLogError(logTag, "%s: Failed to initialize dns resolver", __FUNCTION__);
    return rv;
  }
  if (NS_FAILED(rv = mIceCtx->SetResolver(mDNSResolver->AllocateResolver()))) {
    CSFLogError(logTag, "%s: Failed to get dns resolver", __FUNCTION__);
    return rv;
  }
  mIceCtx->SignalGatheringStateChange.connect(
      this,
      &PeerConnectionMedia::IceGatheringStateChange_s);
  mIceCtx->SignalConnectionStateChange.connect(
      this,
      &PeerConnectionMedia::IceConnectionStateChange_s);

  return NS_OK;
}

void
PeerConnectionMedia::UpdateTransports(const mozilla::JsepSession& session) {

  size_t numTransports = session.GetTransportCount();
  for (size_t i = 0; i < numTransports; ++i) {
    RefPtr<JsepTransport> transport;

    nsresult rv = session.GetTransport(i, &transport);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    if (NS_FAILED(rv))
      break;

    std::string ufrag;
    std::string pwd;
    std::vector<std::string> candidates;

    bool hasAttrs = false;
    if (transport->mIce) {
      hasAttrs = true;
      ufrag = transport->mIce->GetUfrag();
      pwd = transport->mIce->GetPassword();
      candidates = transport->mIce->GetCandidates();
    }

    
    
    
    RUN_ON_THREAD(GetSTSThread(),
                  WrapRunnable(RefPtr<PeerConnectionMedia>(this),
                               &PeerConnectionMedia::UpdateIceMediaStream_s,
                               i,
                               transport->mComponents,
                               hasAttrs,
                               ufrag,
                               pwd,
                               candidates),
                  NS_DISPATCH_NORMAL);
  }


  
  
  RUN_ON_THREAD(GetSTSThread(),
                WrapRunnable(
                    RefPtr<PeerConnectionMedia>(this),
                    &PeerConnectionMedia::EnsureIceGathering_s),
                NS_DISPATCH_NORMAL);

}

nsresult PeerConnectionMedia::UpdateMediaPipelines(
    const mozilla::JsepSession& session) {
  size_t numPairs = session.GetNegotiatedTrackPairCount();
  mozilla::MediaPipelineFactory factory(this);
  const mozilla::JsepTrackPair* pair;

  for (size_t i = 0; i < numPairs; ++i) {
    nsresult rv = session.GetNegotiatedTrackPair(i, &pair);
    if (NS_FAILED(rv)) {
      MOZ_ASSERT(false);
      return rv;
    }

    
    
    
    if (pair->mReceiving) {
      rv = factory.CreateMediaPipeline(*pair, *pair->mReceiving);
      if (NS_FAILED(rv)) {
        CSFLogError(logTag, "Failed to create receiving pipeline, rv=%u",
                            static_cast<unsigned>(rv));
        return rv;
      }
    }
    if (pair->mSending) {
      rv = factory.CreateMediaPipeline(*pair, *pair->mSending);
      if (NS_FAILED(rv)) {
        CSFLogError(logTag, "Failed to create sending pipeline, rv=%u",
                            static_cast<unsigned>(rv));
        return rv;
      }
    }
  }

  return NS_OK;
}

void
PeerConnectionMedia::StartIceChecks(const mozilla::JsepSession& session) {

  std::vector<size_t> numComponentsByLevel;
  for (size_t i = 0; i < session.GetTransportCount(); ++i) {
    RefPtr<JsepTransport> transport;
    nsresult rv = session.GetTransport(i, &transport);
    if (NS_FAILED(rv)) {
      CSFLogError(logTag, "JsepSession::GetTransport() failed: %u",
                          static_cast<unsigned>(rv));
      MOZ_ASSERT(false, "JsepSession::GetTransport() failed!");
      break;
    }

    if (transport->mState == JsepTransport::kJsepTransportClosed) {
      numComponentsByLevel.push_back(0);
    } else {
      numComponentsByLevel.push_back(transport->mComponents);
    }
  }

  RUN_ON_THREAD(GetSTSThread(),
                WrapRunnable(
                  RefPtr<PeerConnectionMedia>(this),
                  &PeerConnectionMedia::StartIceChecks_s,
                  session.IsIceControlling(),
                  session.RemoteIsIceLite(),
                  
                  std::vector<std::string>(session.GetIceOptions()),
                  numComponentsByLevel),
                NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::StartIceChecks_s(
    bool aIsControlling,
    bool aIsIceLite,
    const std::vector<std::string>& aIceOptionsList,
    const std::vector<size_t>& aComponentCountByLevel) {

  CSFLogDebug(logTag, "Starting ICE Checking");

  std::vector<std::string> attributes;
  if (aIsIceLite) {
    attributes.push_back("ice-lite");
  }

  if (!aIceOptionsList.empty()) {
    attributes.push_back("ice-options:");
    for (auto i = aIceOptionsList.begin(); i != aIceOptionsList.end(); ++i) {
      attributes.back() += *i + ' ';
    }
  }

  nsresult rv = mIceCtx->ParseGlobalAttributes(attributes);
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "%s: couldn't parse global parameters", __FUNCTION__ );
  }

  mIceCtx->SetControlling(aIsControlling ?
                          NrIceCtx::ICE_CONTROLLING :
                          NrIceCtx::ICE_CONTROLLED);

  for (size_t i = 0; i < aComponentCountByLevel.size(); ++i) {
    RefPtr<NrIceMediaStream> stream(mIceCtx->GetStream(i));
    if (!stream) {
      MOZ_ASSERT(false, "JsepSession has more streams than the ICE ctx");
      break;
    }

    for (size_t c = aComponentCountByLevel[i]; c < stream->components(); ++c) {
      
      stream->DisableComponent(c + 1);
    }
  }

  mIceCtx->StartChecks();
}

void
PeerConnectionMedia::AddIceCandidate(const std::string& candidate,
                                     const std::string& mid,
                                     uint32_t aMLine) {
  RUN_ON_THREAD(GetSTSThread(),
                WrapRunnable(
                    RefPtr<PeerConnectionMedia>(this),
                    &PeerConnectionMedia::AddIceCandidate_s,
                    std::string(candidate), 
                    std::string(mid),
                    aMLine),
                NS_DISPATCH_NORMAL);
}
void
PeerConnectionMedia::AddIceCandidate_s(const std::string& aCandidate,
                                       const std::string& aMid,
                                       uint32_t aMLine) {
  if (aMLine >= mIceStreams.size()) {
    CSFLogError(logTag, "Couldn't process ICE candidate for bogus level %u",
                aMLine);
    return;
  }

  nsresult rv = mIceStreams[aMLine]->ParseTrickleCandidate(aCandidate);
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "Couldn't process ICE candidate at level %u",
                aMLine);
    return;
  }
}

void
PeerConnectionMedia::EnsureIceGathering_s() {
  if (mIceCtx->gathering_state() == NrIceCtx::ICE_CTX_GATHER_INIT) {
    mIceCtx->StartGathering();
  }
}

void
PeerConnectionMedia::UpdateIceMediaStream_s(size_t aMLine,
                                            size_t aComponentCount,
                                            bool aHasAttrs,
                                            const std::string& aUfrag,
                                            const std::string& aPassword,
                                            const std::vector<std::string>&
                                            aCandidateList) {
  if (aMLine > mIceStreams.size()) {
    CSFLogError(logTag, "Missing stream for previous m-line %u, this can "
                        "happen if we failed to create a stream earlier.",
                        static_cast<unsigned>(aMLine - 1));
    return;
  }

  CSFLogDebug(logTag, "%s: Creating ICE media stream=%u components=%u",
              mParentHandle.c_str(),
              static_cast<unsigned>(aMLine),
              static_cast<unsigned>(aComponentCount));
  RefPtr<NrIceMediaStream> stream;

  if (mIceStreams.size() == aMLine) {
    std::ostringstream os;
    os << mParentName << " level=" << aMLine;
    stream = mIceCtx->CreateStream(os.str().c_str(), aComponentCount);

    if (!stream) {
      CSFLogError(logTag, "Failed to create ICE stream.");
      return;
    }

    stream->SetLevel(aMLine);
    stream->SignalReady.connect(this, &PeerConnectionMedia::IceStreamReady_s);
    stream->SignalCandidate.connect(this,
                                    &PeerConnectionMedia::OnCandidateFound_s);

    mIceStreams.push_back(stream);
  } else {
    stream = mIceStreams[aMLine];
  }

  if (aHasAttrs) {
    std::vector<std::string> attrs;
    for (auto i = aCandidateList.begin(); i != aCandidateList.end(); ++i) {
      attrs.push_back("candidate:" + *i);
    }
    attrs.push_back("ice-ufrag:" + aUfrag);
    attrs.push_back("ice-pwd:" + aPassword);

    nsresult rv = stream->ParseAttributes(attrs);
    if (NS_FAILED(rv)) {
      CSFLogError(logTag, "Couldn't parse ICE attributes, rv=%u",
                          static_cast<unsigned>(rv));
    }
  }
}

nsresult
PeerConnectionMedia::AddStream(DOMMediaStream* aMediaStream,
                               uint32_t hints,
                               std::string *stream_id)
{
  ASSERT_ON_THREAD(mMainThread);

  if (!aMediaStream) {
    CSFLogError(logTag, "%s - aMediaStream is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  CSFLogDebug(logTag, "%s: MediaStream: %p", __FUNCTION__, aMediaStream);

  
#ifdef MOZILLA_INTERNAL_API
  if (!Preferences::GetBool("media.peerconnection.video.enabled", true)) {
    hints &= ~(DOMMediaStream::HINT_CONTENTS_VIDEO);
  }
#endif

  if (!(hints & (DOMMediaStream::HINT_CONTENTS_AUDIO |
        DOMMediaStream::HINT_CONTENTS_VIDEO))) {
    CSFLogDebug(logTag, "Empty Stream !!");
    return NS_OK;
  }

  
  
  
  
  nsRefPtr<LocalSourceStreamInfo> localSourceStream = nullptr;

  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    auto& lss = mLocalSourceStreams[u];
    if (((hints & DOMMediaStream::HINT_CONTENTS_AUDIO) && lss->AudioTrackCount()) ||
        ((hints & DOMMediaStream::HINT_CONTENTS_VIDEO) && lss->VideoTrackCount())) {
      CSFLogError(logTag, "Only one stream of any given type allowed");
      return NS_ERROR_FAILURE;
    }
    if (aMediaStream == lss->GetMediaStream()) {
      localSourceStream = lss;
      *stream_id = lss->GetId();
      break;
    }
  }
  if (!localSourceStream) {
    std::string id;
    if (!mUuidGen->Generate(&id)) {
      CSFLogError(logTag, "Failed to generate UUID for stream");
      return NS_ERROR_FAILURE;
    }

    localSourceStream = new LocalSourceStreamInfo(aMediaStream, this, id);
    mLocalSourceStreams.AppendElement(localSourceStream);
    *stream_id = id;
  }

  if (hints & DOMMediaStream::HINT_CONTENTS_AUDIO) {
    localSourceStream->ExpectAudio(TRACK_AUDIO);
  }

  if (hints & DOMMediaStream::HINT_CONTENTS_VIDEO) {
    localSourceStream->ExpectVideo(TRACK_VIDEO);
  }
  return NS_OK;
}

nsresult
PeerConnectionMedia::RemoveStream(DOMMediaStream* aMediaStream,
                                  uint32_t hints,
                                  uint32_t *stream_id)
{
  MOZ_ASSERT(aMediaStream);
  ASSERT_ON_THREAD(mMainThread);

  CSFLogDebug(logTag, "%s: MediaStream: %p",
    __FUNCTION__, aMediaStream);

  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    nsRefPtr<LocalSourceStreamInfo> localSourceStream = mLocalSourceStreams[u];
    if (localSourceStream->GetMediaStream() == aMediaStream) {
      *stream_id = u;

      if (hints & DOMMediaStream::HINT_CONTENTS_AUDIO) {
        localSourceStream->RemoveAudio(TRACK_AUDIO);
      }
      if (hints & DOMMediaStream::HINT_CONTENTS_VIDEO) {
        localSourceStream->RemoveAudio(TRACK_VIDEO);
      }
      if (!(localSourceStream->AudioTrackCount() +
            localSourceStream->VideoTrackCount())) {
        mLocalSourceStreams.RemoveElementAt(u);
      }
      return NS_OK;
    }
  }

  return NS_ERROR_ILLEGAL_VALUE;
}

void
PeerConnectionMedia::SelfDestruct()
{
  ASSERT_ON_THREAD(mMainThread);

  CSFLogDebug(logTag, "%s: ", __FUNCTION__);

  
  for (uint32_t i=0; i < mLocalSourceStreams.Length(); ++i) {
    mLocalSourceStreams[i]->DetachMedia_m();
  }

  for (uint32_t i=0; i < mRemoteSourceStreams.Length(); ++i) {
    mRemoteSourceStreams[i]->DetachMedia_m();
  }

  
  RUN_ON_THREAD(mSTSThread, WrapRunnable(
      this, &PeerConnectionMedia::ShutdownMediaTransport_s),
                NS_DISPATCH_NORMAL);

  CSFLogDebug(logTag, "%s: Media shut down", __FUNCTION__);
}

void
PeerConnectionMedia::SelfDestruct_m()
{
  CSFLogDebug(logTag, "%s: ", __FUNCTION__);

  ASSERT_ON_THREAD(mMainThread);
  mLocalSourceStreams.Clear();
  mRemoteSourceStreams.Clear();

  
  this->Release();
}

void
PeerConnectionMedia::ShutdownMediaTransport_s()
{
  ASSERT_ON_THREAD(mSTSThread);

  CSFLogDebug(logTag, "%s: ", __FUNCTION__);

  
  
  
  
  
  for (uint32_t i=0; i < mLocalSourceStreams.Length(); ++i) {
    mLocalSourceStreams[i]->DetachTransport_s();
  }

  for (uint32_t i=0; i < mRemoteSourceStreams.Length(); ++i) {
    mRemoteSourceStreams[i]->DetachTransport_s();
  }

  disconnect_all();
  mTransportFlows.clear();
  mIceStreams.clear();
  mIceCtx = nullptr;

  mMainThread->Dispatch(WrapRunnable(this, &PeerConnectionMedia::SelfDestruct_m),
                        NS_DISPATCH_NORMAL);
}

LocalSourceStreamInfo*
PeerConnectionMedia::GetLocalStreamByIndex(int aIndex)
{
  ASSERT_ON_THREAD(mMainThread);
  if(aIndex < 0 || aIndex >= (int) mLocalSourceStreams.Length()) {
    return nullptr;
  }

  MOZ_ASSERT(mLocalSourceStreams[aIndex]);
  return mLocalSourceStreams[aIndex];
}

LocalSourceStreamInfo*
PeerConnectionMedia::GetLocalStreamById(const std::string& id)
{
  ASSERT_ON_THREAD(mMainThread);
  for (size_t i = 0; i < mLocalSourceStreams.Length(); ++i) {
    if (id == mLocalSourceStreams[i]->GetId()) {
      return mLocalSourceStreams[i];
    }
  }

  MOZ_ASSERT(false);
  return nullptr;
}

RemoteSourceStreamInfo*
PeerConnectionMedia::GetRemoteStreamByIndex(size_t aIndex)
{
  ASSERT_ON_THREAD(mMainThread);
  MOZ_ASSERT(mRemoteSourceStreams.SafeElementAt(aIndex));
  return mRemoteSourceStreams.SafeElementAt(aIndex);
}

RemoteSourceStreamInfo*
PeerConnectionMedia::GetRemoteStreamById(const std::string& id)
{
  ASSERT_ON_THREAD(mMainThread);
  for (size_t i = 0; i < mRemoteSourceStreams.Length(); ++i) {
    if (id == mRemoteSourceStreams[i]->GetId()) {
      return mRemoteSourceStreams[i];
    }
  }

  
  
  
  
  return nullptr;
}

bool
PeerConnectionMedia::SetUsingBundle_m(int aMLine, bool decision)
{
  ASSERT_ON_THREAD(mMainThread);
  for (size_t i = 0; i < mRemoteSourceStreams.Length(); ++i) {
    if (mRemoteSourceStreams[i]->SetUsingBundle_m(aMLine, decision)) {
      
      return true;
    }
  }
  CSFLogWarn(logTag, "Could not locate level %d to set bundle flag to %s",
                     static_cast<int>(aMLine),
                     decision ? "true" : "false");
  return false;
}

static void
UpdateFilterFromRemoteDescription_s(
  RefPtr<mozilla::MediaPipeline> receive,
  RefPtr<mozilla::MediaPipeline> transmit,
  nsAutoPtr<mozilla::MediaPipelineFilter> filter) {

  
  mozilla::MediaPipelineFilter *finalFilter(
    receive->UpdateFilterFromRemoteDescription_s(filter));

  if (finalFilter) {
    filter = new mozilla::MediaPipelineFilter(*finalFilter);
  }

  
  transmit->UpdateFilterFromRemoteDescription_s(filter);
}

bool
PeerConnectionMedia::UpdateFilterFromRemoteDescription_m(
    int aMLine,
    nsAutoPtr<mozilla::MediaPipelineFilter> filter)
{
  ASSERT_ON_THREAD(mMainThread);

  RefPtr<mozilla::MediaPipeline> receive;
  for (size_t i = 0; !receive && i < mRemoteSourceStreams.Length(); ++i) {
    receive = mRemoteSourceStreams[i]->GetPipelineByLevel_m(aMLine);
  }

  RefPtr<mozilla::MediaPipeline> transmit;
  for (size_t i = 0; !transmit && i < mLocalSourceStreams.Length(); ++i) {
    transmit = mLocalSourceStreams[i]->GetPipelineByLevel_m(aMLine);
  }

  if (receive && transmit) {
    
    
    
    
    RUN_ON_THREAD(GetSTSThread(),
                  WrapRunnableNM(
                      &UpdateFilterFromRemoteDescription_s,
                      receive,
                      transmit,
                      filter
                  ),
                  NS_DISPATCH_NORMAL);
    return true;
  } else {
    CSFLogWarn(logTag, "Could not locate level %d to update filter",
        static_cast<int>(aMLine));
  }
  return false;
}

nsresult
PeerConnectionMedia::AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo)
{
  ASSERT_ON_THREAD(mMainThread);

  mRemoteSourceStreams.AppendElement(aInfo);

  return NS_OK;
}

nsresult
PeerConnectionMedia::AddRemoteStreamHint(int aIndex, bool aIsVideo)
{
  if (aIndex < 0 ||
      static_cast<unsigned int>(aIndex) >= mRemoteSourceStreams.Length()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  RemoteSourceStreamInfo *pInfo = mRemoteSourceStreams.ElementAt(aIndex);
  MOZ_ASSERT(pInfo);

  if (aIsVideo) {
    pInfo->mTrackTypeHints |= DOMMediaStream::HINT_CONTENTS_VIDEO;
  } else {
    pInfo->mTrackTypeHints |= DOMMediaStream::HINT_CONTENTS_AUDIO;
  }

  return NS_OK;
}


void
PeerConnectionMedia::IceGatheringStateChange_s(NrIceCtx* ctx,
                                               NrIceCtx::GatheringState state)
{
  ASSERT_ON_THREAD(mSTSThread);

  if (state == NrIceCtx::ICE_CTX_GATHER_COMPLETE) {
    
    for (size_t i = 0; ; ++i) {
      RefPtr<NrIceMediaStream> stream(ctx->GetStream(i));
      if (!stream) {
        break;
      }

      NrIceCandidate candidate;
      nsresult res = stream->GetDefaultCandidate(&candidate);
      if (NS_SUCCEEDED(res)) {
        EndOfLocalCandidates(candidate.cand_addr.host,
                             candidate.cand_addr.port,
                             i);
      } else {
        CSFLogError(logTag, "%s: GetDefaultCandidate failed for level %u, "
                            "res=%u",
                            __FUNCTION__,
                            static_cast<unsigned>(i),
                            static_cast<unsigned>(res));
      }
    }
  }

  
  
  
  
  GetMainThread()->Dispatch(
    WrapRunnable(this,
                 &PeerConnectionMedia::IceGatheringStateChange_m,
                 ctx,
                 state),
    NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::IceConnectionStateChange_s(NrIceCtx* ctx,
                                                NrIceCtx::ConnectionState state)
{
  ASSERT_ON_THREAD(mSTSThread);
  
  
  
  
  GetMainThread()->Dispatch(
    WrapRunnable(this,
                 &PeerConnectionMedia::IceConnectionStateChange_m,
                 ctx,
                 state),
    NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::OnCandidateFound_s(NrIceMediaStream *aStream,
                                        const std::string &candidate)
{
  ASSERT_ON_THREAD(mSTSThread);
  MOZ_ASSERT(aStream);

  CSFLogDebug(logTag, "%s: %s", __FUNCTION__, aStream->name().c_str());

  
  
  
  
  GetMainThread()->Dispatch(
    WrapRunnable(this,
                 &PeerConnectionMedia::OnCandidateFound_m,
                 candidate,
                 aStream->GetLevel()),
    NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::EndOfLocalCandidates(const std::string& aDefaultAddr,
                                          uint16_t aDefaultPort,
                                          uint16_t aMLine) {
  
  GetMainThread()->Dispatch(
    WrapRunnable(this,
                 &PeerConnectionMedia::EndOfLocalCandidates_m,
                 aDefaultAddr, aDefaultPort, aMLine),
    NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::IceGatheringStateChange_m(NrIceCtx* ctx,
                                               NrIceCtx::GatheringState state)
{
  ASSERT_ON_THREAD(mMainThread);
  SignalIceGatheringStateChange(ctx, state);
}

void
PeerConnectionMedia::IceConnectionStateChange_m(NrIceCtx* ctx,
                                                NrIceCtx::ConnectionState state)
{
  ASSERT_ON_THREAD(mMainThread);
  SignalIceConnectionStateChange(ctx, state);
}

void
PeerConnectionMedia::IceStreamReady_s(NrIceMediaStream *aStream)
{
  MOZ_ASSERT(aStream);

  CSFLogDebug(logTag, "%s: %s", __FUNCTION__, aStream->name().c_str());
}

void
PeerConnectionMedia::OnCandidateFound_m(const std::string &candidate,
                                        uint16_t aMLine)
{
  ASSERT_ON_THREAD(mMainThread);
  SignalCandidate(candidate, aMLine);
}

void
PeerConnectionMedia::EndOfLocalCandidates_m(const std::string& aDefaultAddr,
                                            uint16_t aDefaultPort,
                                            uint16_t aMLine) {
  SignalEndOfLocalCandidates(aDefaultAddr, aDefaultPort, aMLine);
}

void
PeerConnectionMedia::DtlsConnected_s(TransportLayer *dtlsLayer,
                                     TransportLayer::State state)
{
  dtlsLayer->SignalStateChange.disconnect(this);

  bool privacyRequested = false;
  
  
  GetMainThread()->Dispatch(
    WrapRunnableNM(&PeerConnectionMedia::DtlsConnected_m,
                   mParentHandle, privacyRequested),
    NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::DtlsConnected_m(const std::string& aParentHandle,
                                     bool aPrivacyRequested)
{
  PeerConnectionWrapper pcWrapper(aParentHandle);
  PeerConnectionImpl* pc = pcWrapper.impl();
  if (pc) {
    pc->SetDtlsConnected(aPrivacyRequested);
  }
}

void
PeerConnectionMedia::AddTransportFlow(int aIndex, bool aRtcp,
                                      const RefPtr<TransportFlow> &aFlow)
{
  int index_inner = aIndex * 2 + (aRtcp ? 1 : 0);

  MOZ_ASSERT(!mTransportFlows[index_inner]);
  mTransportFlows[index_inner] = aFlow;

  GetSTSThread()->Dispatch(
    WrapRunnable(this, &PeerConnectionMedia::ConnectDtlsListener_s, aFlow),
    NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::ConnectDtlsListener_s(const RefPtr<TransportFlow>& aFlow)
{
  TransportLayer* dtls = aFlow->GetLayer(TransportLayerDtls::ID());
  if (dtls) {
    dtls->SignalStateChange.connect(this, &PeerConnectionMedia::DtlsConnected_s);
  }
}

#ifdef MOZILLA_INTERNAL_API









bool
PeerConnectionMedia::AnyLocalStreamHasPeerIdentity() const
{
  ASSERT_ON_THREAD(mMainThread);

  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    
    DOMMediaStream* stream = mLocalSourceStreams[u]->GetMediaStream();
    if (stream->GetPeerIdentity()) {
      return true;
    }
  }
  return false;
}

void
PeerConnectionMedia::UpdateRemoteStreamPrincipals_m(nsIPrincipal* aPrincipal)
{
  ASSERT_ON_THREAD(mMainThread);

  for (uint32_t u = 0; u < mRemoteSourceStreams.Length(); u++) {
    mRemoteSourceStreams[u]->UpdatePrincipal_m(aPrincipal);
  }
}

void
PeerConnectionMedia::UpdateSinkIdentity_m(nsIPrincipal* aPrincipal,
                                          const PeerIdentity* aSinkIdentity)
{
  ASSERT_ON_THREAD(mMainThread);

  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    mLocalSourceStreams[u]->UpdateSinkIdentity_m(aPrincipal, aSinkIdentity);
  }
}

void
LocalSourceStreamInfo::UpdateSinkIdentity_m(nsIPrincipal* aPrincipal,
                                            const PeerIdentity* aSinkIdentity)
{
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
    MediaPipelineTransmit* pipeline =
      static_cast<MediaPipelineTransmit*>((*it).second.get());
    pipeline->UpdateSinkIdentity_m(aPrincipal, aSinkIdentity);
  }
}

void RemoteSourceStreamInfo::UpdatePrincipal_m(nsIPrincipal* aPrincipal)
{
  
  
  
  mMediaStream->SetPrincipal(aPrincipal);
}
#endif 

bool
PeerConnectionMedia::AnyCodecHasPluginID(uint64_t aPluginID)
{
  for (uint32_t i=0; i < mLocalSourceStreams.Length(); ++i) {
    if (mLocalSourceStreams[i]->AnyCodecHasPluginID(aPluginID)) {
      return true;
    }
  }
  for (uint32_t i=0; i < mRemoteSourceStreams.Length(); ++i) {
    if (mRemoteSourceStreams[i]->AnyCodecHasPluginID(aPluginID)) {
      return true;
    }
  }
  return false;
}

bool
LocalSourceStreamInfo::AnyCodecHasPluginID(uint64_t aPluginID)
{
  
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
    if (it->second->Conduit()->CodecPluginID() == aPluginID) {
      return true;
    }
  }
  return false;
}

bool
RemoteSourceStreamInfo::AnyCodecHasPluginID(uint64_t aPluginID)
{
  
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
    if (it->second->Conduit()->CodecPluginID() == aPluginID) {
      return true;
    }
  }
  return false;
}

void
LocalSourceStreamInfo::StorePipeline(
  int aMLine, mozilla::RefPtr<mozilla::MediaPipelineTransmit> aPipeline)
{
  MOZ_ASSERT(mPipelines.find(aMLine) == mPipelines.end());
  if (mPipelines.find(aMLine) != mPipelines.end()) {
    CSFLogError(logTag, "%s: Storing duplicate track", __FUNCTION__);
    return;
  }
  
  
  mPipelines[aMLine] = aPipeline;
}

void
RemoteSourceStreamInfo::StorePipeline(
  int aMLine, bool aIsVideo,
  mozilla::RefPtr<mozilla::MediaPipelineReceive> aPipeline)
{
  MOZ_ASSERT(mPipelines.find(aMLine) == mPipelines.end());
  if (mPipelines.find(aMLine) != mPipelines.end()) {
    CSFLogError(logTag, "%s: Request to store duplicate track %d", __FUNCTION__, aMLine);
    return;
  }
  CSFLogDebug(logTag, "%s track %d %s = %p", __FUNCTION__, aMLine, aIsVideo ? "video" : "audio",
              aPipeline.get());
  
  
  for (std::map<int, bool>::iterator it = mTypes.begin(); it != mTypes.end(); ++it) {
    if (it->second != aIsVideo) {
      
      mozilla::WebrtcAudioConduit *audio_conduit = static_cast<mozilla::WebrtcAudioConduit*>
                                                   (aIsVideo ?
                                                    mPipelines[it->first]->Conduit() :
                                                    aPipeline->Conduit());
      mozilla::WebrtcVideoConduit *video_conduit = static_cast<mozilla::WebrtcVideoConduit*>
                                                   (aIsVideo ?
                                                    aPipeline->Conduit() :
                                                    mPipelines[it->first]->Conduit());
      video_conduit->SyncTo(audio_conduit);
      CSFLogDebug(logTag, "Syncing %p to %p, %d to %d", video_conduit, audio_conduit,
                  aMLine, it->first);
    }
  }
  
  
  mPipelines[aMLine] = aPipeline;
  
  mTypes[aMLine] = aIsVideo;
}

RefPtr<MediaPipeline> SourceStreamInfo::GetPipelineByLevel_m(int aMLine) {
  ASSERT_ON_THREAD(mParent->GetMainThread());

  
  
  
  
  
  if (mMediaStream) {
    for (auto p = mPipelines.begin(); p != mPipelines.end(); ++p) {
      if (p->second->level() == aMLine) {
        return p->second;
      }
    }
  }

  return nullptr;
}

bool RemoteSourceStreamInfo::SetUsingBundle_m(int aMLine, bool decision) {
  ASSERT_ON_THREAD(mParent->GetMainThread());

  
  MediaPipeline *pipeline = GetPipelineByLevel_m(aMLine);

  if (pipeline) {
    RUN_ON_THREAD(mParent->GetSTSThread(),
                  WrapRunnable(
                      RefPtr<MediaPipeline>(pipeline),
                      &MediaPipeline::SetUsingBundle_s,
                      decision
                  ),
                  NS_DISPATCH_NORMAL);
    return true;
  }
  return false;
}

}  
