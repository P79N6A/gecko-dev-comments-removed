



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

#if !defined(MOZILLA_XPCOMRT_API)
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsICancelable.h"
#include "nsIDocument.h"
#include "nsILoadInfo.h"
#include "nsIContentPolicy.h"
#include "nsIProxyInfo.h"
#include "nsIProtocolProxyService.h"
#endif 

#include "nsProxyRelease.h"

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
#include "MediaStreamList.h"
#include "nsIScriptGlobalObject.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/RTCStatsReportBinding.h"
#include "MediaStreamTrack.h"
#include "VideoStreamTrack.h"
#endif



namespace mozilla {
using namespace dom;

static const char* logTag = "PeerConnectionMedia";

nsresult
PeerConnectionMedia::ReplaceTrack(const std::string& aOldStreamId,
                                  const std::string& aOldTrackId,
                                  DOMMediaStream* aNewStream,
                                  const std::string& aNewStreamId,
                                  const std::string& aNewTrackId)
{
  RefPtr<LocalSourceStreamInfo> oldInfo(GetLocalStreamById(aOldStreamId));

  if (!oldInfo) {
    CSFLogError(logTag, "Failed to find stream id %s", aOldStreamId.c_str());
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = AddTrack(aNewStream, aNewStreamId, aNewTrackId);
  NS_ENSURE_SUCCESS(rv, rv);

  RefPtr<LocalSourceStreamInfo> newInfo(GetLocalStreamById(aNewStreamId));

  if (!newInfo) {
    CSFLogError(logTag, "Failed to add track id %s", aNewTrackId.c_str());
    MOZ_ASSERT(false);
    return NS_ERROR_FAILURE;
  }

  rv = newInfo->TakePipelineFrom(oldInfo, aOldTrackId, aNewTrackId);
  NS_ENSURE_SUCCESS(rv, rv);

  return RemoveLocalTrack(aOldStreamId, aOldTrackId);
}

static void
PipelineReleaseRef_m(RefPtr<MediaPipeline> pipeline)
{}

static void
PipelineDetachTransport_s(RefPtr<MediaPipeline> pipeline,
                          nsCOMPtr<nsIThread> mainThread)
{
  pipeline->ShutdownTransport_s();
  mainThread->Dispatch(
      
      
      WrapRunnableNM(PipelineReleaseRef_m, pipeline.forget()),
      NS_DISPATCH_NORMAL);
}

void
SourceStreamInfo::RemoveTrack(const std::string& trackId)
{
  mTracks.erase(trackId);
  RefPtr<MediaPipeline> pipeline = GetPipelineByTrackId_m(trackId);
  if (pipeline) {
    mPipelines.erase(trackId);
    pipeline->ShutdownMedia_m();
    mParent->GetSTSThread()->Dispatch(
        WrapRunnableNM(PipelineDetachTransport_s,
                       pipeline.forget(),
                       mParent->GetMainThread()),
        NS_DISPATCH_NORMAL);
  }
}

void SourceStreamInfo::DetachTransport_s()
{
  ASSERT_ON_THREAD(mParent->GetSTSThread());
  
  
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
    it->second->ShutdownTransport_s();
  }
}

void SourceStreamInfo::DetachMedia_m()
{
  ASSERT_ON_THREAD(mParent->GetMainThread());

  
  
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
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

#if !defined(MOZILLA_XPCOMRT_API)
NS_IMETHODIMP PeerConnectionMedia::ProtocolProxyQueryHandler::
OnProxyAvailable(nsICancelable *request,
                 nsIChannel *aChannel,
                 nsIProxyInfo *proxyinfo,
                 nsresult result) {
  CSFLogInfo(logTag, "%s: Proxy Available: %d", __FUNCTION__, (int)result);

  if (NS_SUCCEEDED(result) && proxyinfo) {
    CSFLogInfo(logTag, "%s: Had proxyinfo", __FUNCTION__);
    nsresult rv;
    nsCString httpsProxyHost;
    int32_t httpsProxyPort;

    rv = proxyinfo->GetHost(httpsProxyHost);
    if (NS_FAILED(rv)) {
      CSFLogError(logTag, "%s: Failed to get proxy server host", __FUNCTION__);
      return rv;
    }

    rv = proxyinfo->GetPort(&httpsProxyPort);
    if (NS_FAILED(rv)) {
      CSFLogError(logTag, "%s: Failed to get proxy server port", __FUNCTION__);
      return rv;
    }

    if (pcm_->mIceCtx.get()) {
      assert(httpsProxyPort >= 0 && httpsProxyPort < (1 << 16));
      pcm_->mProxyServer.reset(
        new NrIceProxyServer(httpsProxyHost.get(),
                             static_cast<uint16_t>(httpsProxyPort)));
    } else {
      CSFLogError(logTag, "%s: Failed to set proxy server (ICE ctx unavailable)",
          __FUNCTION__);
    }
  }

  if (result != NS_ERROR_ABORT) {
    
    pcm_->mProxyResolveCompleted = true;
    pcm_->FlushIceCtxOperationQueueIfReady();
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS(PeerConnectionMedia::ProtocolProxyQueryHandler, nsIProtocolProxyCallback)
#endif 

PeerConnectionMedia::PeerConnectionMedia(PeerConnectionImpl *parent)
    : mParent(parent),
      mParentHandle(parent->GetHandle()),
      mParentName(parent->GetName()),
      mAllowIceLoopback(false),
      mIceCtx(nullptr),
      mDNSResolver(new NrIceResolver()),
      mUuidGen(MakeUnique<PCUuidGenerator>()),
      mMainThread(mParent->GetMainThread()),
      mSTSThread(mParent->GetSTSThread()),
      mProxyResolveCompleted(false) {
}

nsresult PeerConnectionMedia::Init(const std::vector<NrIceStunServer>& stun_servers,
                                   const std::vector<NrIceTurnServer>& turn_servers)
{
  nsresult rv;
#if defined(MOZILLA_XPCOMRT_API)
  
  mProxyResolveCompleted = true;
#else

  nsCOMPtr<nsIProtocolProxyService> pps =
    do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "%s: Failed to get proxy service: %d", __FUNCTION__, (int)rv);
    return NS_ERROR_FAILURE;
  }

  
  
  
  nsCOMPtr<nsIURI> fakeHttpsLocation;
  rv = NS_NewURI(getter_AddRefs(fakeHttpsLocation), "https://example.com");
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "%s: Failed to set URI: %d", __FUNCTION__, (int)rv);
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIChannel> channel;

#if defined(MOZILLA_INTERNAL_API)
  nsCOMPtr<nsIDocument> principal = mParent->GetWindow()->GetExtantDoc();
#else
  
  nsCOMPtr<nsIScriptSecurityManager> secMan(
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "%s: Failed to get IOService: %d",
        __FUNCTION__, (int)rv);
    CSFLogError(logTag, "%s: Failed to get securityManager: %d", __FUNCTION__, (int)rv);
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPrincipal> principal;
  rv = secMan->GetSystemPrincipal(getter_AddRefs(principal));
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "%s: Failed to get systemPrincipal: %d", __FUNCTION__, (int)rv);
    return NS_ERROR_FAILURE;
  }

  rv = NS_NewChannel(getter_AddRefs(channel),
                     fakeHttpsLocation,
                     principal,
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_OTHER);

  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "%s: Failed to get channel from URI: %d",
                __FUNCTION__, (int)rv);
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<ProtocolProxyQueryHandler> handler = new ProtocolProxyQueryHandler(this);
  rv = pps->AsyncResolve(channel,
                         nsIProtocolProxyService::RESOLVE_PREFER_HTTPS_PROXY |
                         nsIProtocolProxyService::RESOLVE_ALWAYS_TUNNEL,
                         handler, getter_AddRefs(mProxyRequest));
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "%s: Failed to resolve protocol proxy: %d", __FUNCTION__, (int)rv);
    return NS_ERROR_FAILURE;
  }
#endif 
#endif 

  
  
  mIceCtx = NrIceCtx::Create("PC:" + mParentName,
                             true, 
                             true, 
                             mAllowIceLoopback);
  if(!mIceCtx) {
    CSFLogError(logTag, "%s: Failed to create Ice Context", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  if (NS_FAILED(rv = mIceCtx->SetStunServers(stun_servers))) {
    CSFLogError(logTag, "%s: Failed to set stun servers", __FUNCTION__);
    return rv;
  }
  
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
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
PeerConnectionMedia::UpdateTransports(const JsepSession& session,
                                      bool restartGathering) {

  auto transports = session.GetTransports();
  for (size_t i = 0; i < transports.size(); ++i) {
    RefPtr<JsepTransport> transport = transports[i];

    std::string ufrag;
    std::string pwd;
    std::vector<std::string> candidates;

    bool hasAttrs = false;
    if (transport->mIce) {
      CSFLogDebug(logTag, "Transport %u is active",
                          static_cast<unsigned>(i));
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

  if (restartGathering) {
    GatherIfReady();
  }
}

nsresult PeerConnectionMedia::UpdateMediaPipelines(
    const JsepSession& session) {
  auto trackPairs = session.GetNegotiatedTrackPairs();
  MediaPipelineFactory factory(this);
  nsresult rv;

  for (auto i = trackPairs.begin(); i != trackPairs.end(); ++i) {
    JsepTrackPair pair = *i;

    if (pair.mReceiving) {
      rv = factory.CreateOrUpdateMediaPipeline(pair, *pair.mReceiving);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }

    if (pair.mSending) {
      rv = factory.CreateOrUpdateMediaPipeline(pair, *pair.mSending);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  for (auto& stream : mRemoteSourceStreams) {
    stream->StartReceiving();
  }

  return NS_OK;
}

void
PeerConnectionMedia::StartIceChecks(const JsepSession& session) {

  std::vector<size_t> numComponentsByLevel;
  auto transports = session.GetTransports();
  for (size_t i = 0; i < transports.size(); ++i) {
    RefPtr<JsepTransport> transport = transports[i];
    if (transport->mState == JsepTransport::kJsepTransportClosed) {
      CSFLogDebug(logTag, "Transport %s is disabled",
                          transport->mTransportId.c_str());
      numComponentsByLevel.push_back(0);
      
      RemoveTransportFlow(i, false);
      RemoveTransportFlow(i, true);
    } else {
      CSFLogDebug(logTag, "Transport %s has %u components",
                          transport->mTransportId.c_str(),
                          static_cast<unsigned>(transport->mComponents));
      numComponentsByLevel.push_back(transport->mComponents);
    }
  }

  nsCOMPtr<nsIRunnable> runnable(
      WrapRunnable(
        RefPtr<PeerConnectionMedia>(this),
        &PeerConnectionMedia::StartIceChecks_s,
        session.IsIceControlling(),
        session.RemoteIsIceLite(),
        
        std::vector<std::string>(session.GetIceOptions()),
        numComponentsByLevel));

  PerformOrEnqueueIceCtxOperation(runnable);
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
      continue;
    }

    if (!aComponentCountByLevel[i]) {
      
      mIceCtx->SetStream(i, nullptr);
      continue;
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
  RefPtr<NrIceMediaStream> stream(mIceCtx->GetStream(aMLine));
  if (!stream) {
    CSFLogError(logTag, "No ICE stream for candidate at level %u: %s",
                        static_cast<unsigned>(aMLine), aCandidate.c_str());
    return;
  }

  nsresult rv = stream->ParseTrickleCandidate(aCandidate);
  if (NS_FAILED(rv)) {
    CSFLogError(logTag, "Couldn't process ICE candidate at level %u",
                static_cast<unsigned>(aMLine));
    return;
  }
}

void
PeerConnectionMedia::FlushIceCtxOperationQueueIfReady()
{
  ASSERT_ON_THREAD(mMainThread);

  if (IsIceCtxReady()) {
    for (auto i = mQueuedIceCtxOperations.begin();
         i != mQueuedIceCtxOperations.end();
         ++i) {
      GetSTSThread()->Dispatch(*i, NS_DISPATCH_NORMAL);
    }
    mQueuedIceCtxOperations.clear();
  }
}

void
PeerConnectionMedia::PerformOrEnqueueIceCtxOperation(nsIRunnable* runnable)
{
  ASSERT_ON_THREAD(mMainThread);

  if (IsIceCtxReady()) {
    GetSTSThread()->Dispatch(runnable, NS_DISPATCH_NORMAL);
  } else {
    mQueuedIceCtxOperations.push_back(runnable);
  }
}

void
PeerConnectionMedia::GatherIfReady() {
  ASSERT_ON_THREAD(mMainThread);

  nsCOMPtr<nsIRunnable> runnable(WrapRunnable(
        RefPtr<PeerConnectionMedia>(this),
        &PeerConnectionMedia::EnsureIceGathering_s));

  PerformOrEnqueueIceCtxOperation(runnable);
}

void
PeerConnectionMedia::EnsureIceGathering_s() {
  if (mProxyServer) {
    mIceCtx->SetProxyServer(*mProxyServer);
  }
  mIceCtx->StartGathering();
}

void
PeerConnectionMedia::UpdateIceMediaStream_s(size_t aMLine,
                                            size_t aComponentCount,
                                            bool aHasAttrs,
                                            const std::string& aUfrag,
                                            const std::string& aPassword,
                                            const std::vector<std::string>&
                                            aCandidateList) {
  RefPtr<NrIceMediaStream> stream(mIceCtx->GetStream(aMLine));
  if (!stream) {
    CSFLogDebug(logTag, "%s: Creating ICE media stream=%u components=%u",
                mParentHandle.c_str(),
                static_cast<unsigned>(aMLine),
                static_cast<unsigned>(aComponentCount));

    std::ostringstream os;
    os << mParentName << " level=" << aMLine;
    stream = mIceCtx->CreateStream(os.str().c_str(),
                                   aComponentCount);

    if (!stream) {
      CSFLogError(logTag, "Failed to create ICE stream.");
      return;
    }

    stream->SetLevel(aMLine);
    stream->SignalReady.connect(this, &PeerConnectionMedia::IceStreamReady_s);
    stream->SignalCandidate.connect(this,
                                    &PeerConnectionMedia::OnCandidateFound_s);

    mIceCtx->SetStream(aMLine, stream);
  }

  if (aHasAttrs && !stream->HasParsedAttributes()) {
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
PeerConnectionMedia::AddTrack(DOMMediaStream* aMediaStream,
                              const std::string& streamId,
                              const std::string& trackId)
{
  ASSERT_ON_THREAD(mMainThread);

  if (!aMediaStream) {
    CSFLogError(logTag, "%s - aMediaStream is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  CSFLogDebug(logTag, "%s: MediaStream: %p", __FUNCTION__, aMediaStream);

  nsRefPtr<LocalSourceStreamInfo> localSourceStream =
    GetLocalStreamById(streamId);

  if (!localSourceStream) {
    localSourceStream = new LocalSourceStreamInfo(aMediaStream, this, streamId);
    mLocalSourceStreams.AppendElement(localSourceStream);
  }

  localSourceStream->AddTrack(trackId);
  return NS_OK;
}

nsresult
PeerConnectionMedia::RemoveLocalTrack(const std::string& streamId,
                                      const std::string& trackId)
{
  ASSERT_ON_THREAD(mMainThread);

  CSFLogDebug(logTag, "%s: stream: %s track: %s", __FUNCTION__,
                      streamId.c_str(), trackId.c_str());

  nsRefPtr<LocalSourceStreamInfo> localSourceStream =
    GetLocalStreamById(streamId);
  if (!localSourceStream) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  localSourceStream->RemoveTrack(trackId);
  if (!localSourceStream->GetTrackCount()) {
    mLocalSourceStreams.RemoveElement(localSourceStream);
  }
  return NS_OK;
}

nsresult
PeerConnectionMedia::RemoveRemoteTrack(const std::string& streamId,
                                       const std::string& trackId)
{
  ASSERT_ON_THREAD(mMainThread);

  CSFLogDebug(logTag, "%s: stream: %s track: %s", __FUNCTION__,
                      streamId.c_str(), trackId.c_str());

  nsRefPtr<RemoteSourceStreamInfo> remoteSourceStream =
    GetRemoteStreamById(streamId);
  if (!remoteSourceStream) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  remoteSourceStream->RemoveTrack(trackId);
  if (!remoteSourceStream->GetTrackCount()) {
    mRemoteSourceStreams.RemoveElement(remoteSourceStream);
  }
  return NS_OK;
}

nsresult
PeerConnectionMedia::GetRemoteTrackId(const std::string streamId,
                                      TrackID numericTrackId,
                                      std::string* trackId) const
{
  auto* ncThis = const_cast<PeerConnectionMedia*>(this);
  const RemoteSourceStreamInfo* info =
    ncThis->GetRemoteStreamById(streamId);

  if (!info) {
    CSFLogError(logTag, "%s: Could not find stream info", __FUNCTION__);
    return NS_ERROR_NOT_AVAILABLE;
  }

  return info->GetTrackId(numericTrackId, trackId);
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

  if (mProxyRequest) {
    mProxyRequest->Cancel(NS_ERROR_ABORT);
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

nsresult
PeerConnectionMedia::AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo)
{
  ASSERT_ON_THREAD(mMainThread);

  mRemoteSourceStreams.AppendElement(aInfo);

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
  int index_inner = GetTransportFlowIndex(aIndex, aRtcp);

  MOZ_ASSERT(!mTransportFlows[index_inner]);
  mTransportFlows[index_inner] = aFlow;

  GetSTSThread()->Dispatch(
    WrapRunnable(this, &PeerConnectionMedia::ConnectDtlsListener_s, aFlow),
    NS_DISPATCH_NORMAL);
}

void
PeerConnectionMedia::RemoveTransportFlow(int aIndex, bool aRtcp)
{
  int index_inner = GetTransportFlowIndex(aIndex, aRtcp);
  TransportFlow* flow = mTransportFlows[index_inner].forget().take();
  if (flow) {
    NS_ProxyRelease(GetSTSThread(), flow);
  }
}

void
PeerConnectionMedia::ConnectDtlsListener_s(const RefPtr<TransportFlow>& aFlow)
{
  TransportLayer* dtls = aFlow->GetLayer(TransportLayerDtls::ID());
  if (dtls) {
    dtls->SignalStateChange.connect(this, &PeerConnectionMedia::DtlsConnected_s);
  }
}

nsresult
LocalSourceStreamInfo::TakePipelineFrom(RefPtr<LocalSourceStreamInfo>& info,
                                        const std::string& oldTrackId,
                                        const std::string& newTrackId)
{
  if (mPipelines.count(newTrackId)) {
    CSFLogError(logTag, "%s: Pipeline already exists for %s/%s",
                __FUNCTION__, mId.c_str(), newTrackId.c_str());
    return NS_ERROR_INVALID_ARG;
  }

  RefPtr<MediaPipeline> pipeline(info->ForgetPipelineByTrackId_m(oldTrackId));

  if (!pipeline) {
    
    
    CSFLogInfo(logTag, "%s: Replacing track before the pipeline has been "
                       "created, nothing to do.", __FUNCTION__);
    return NS_OK;
  }

  nsresult rv =
    static_cast<MediaPipelineTransmit*>(pipeline.get())->ReplaceTrack(
        mMediaStream, newTrackId);
  NS_ENSURE_SUCCESS(rv, rv);

  mPipelines[newTrackId] = pipeline;

  return NS_OK;
}

#if !defined(MOZILLA_EXTERNAL_LINKAGE)









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
SourceStreamInfo::AnyCodecHasPluginID(uint64_t aPluginID)
{
  
  for (auto it = mPipelines.begin(); it != mPipelines.end(); ++it) {
    if (it->second->Conduit()->CodecPluginID() == aPluginID) {
      return true;
    }
  }
  return false;
}

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
nsRefPtr<mozilla::dom::VideoStreamTrack>
SourceStreamInfo::GetVideoTrackByTrackId(const std::string& trackId)
{
  nsTArray<nsRefPtr<mozilla::dom::VideoStreamTrack>> videoTracks;

  mMediaStream->GetVideoTracks(videoTracks);

  for (size_t i = 0; i < videoTracks.Length(); ++i) {
    nsString aTrackId;
    videoTracks[i]->GetId(aTrackId);
    if (aTrackId.EqualsIgnoreCase(trackId.c_str())) {
      return videoTracks[i];
    }
  }

  return nullptr;
}
#endif

nsresult
SourceStreamInfo::StorePipeline(
    const std::string& trackId,
    const mozilla::RefPtr<mozilla::MediaPipeline>& aPipeline)
{
  MOZ_ASSERT(mPipelines.find(trackId) == mPipelines.end());
  if (mPipelines.find(trackId) != mPipelines.end()) {
    CSFLogError(logTag, "%s: Storing duplicate track", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  mPipelines[trackId] = aPipeline;
  return NS_OK;
}

void
RemoteSourceStreamInfo::SyncPipeline(
  RefPtr<MediaPipelineReceive> aPipeline)
{
  
  
  
  
  
  for (auto i = mPipelines.begin(); i != mPipelines.end(); ++i) {
    if (i->second->IsVideo() != aPipeline->IsVideo()) {
      
      WebrtcAudioConduit *audio_conduit =
        static_cast<WebrtcAudioConduit*>(aPipeline->IsVideo() ?
                                                  i->second->Conduit() :
                                                  aPipeline->Conduit());
      WebrtcVideoConduit *video_conduit =
        static_cast<WebrtcVideoConduit*>(aPipeline->IsVideo() ?
                                                  aPipeline->Conduit() :
                                                  i->second->Conduit());
      video_conduit->SyncTo(audio_conduit);
      CSFLogDebug(logTag, "Syncing %p to %p, %s to %s",
                          video_conduit, audio_conduit,
                          i->first.c_str(), aPipeline->trackid().c_str());
    }
  }
}

void
RemoteSourceStreamInfo::StartReceiving()
{
  if (mReceiving || mPipelines.empty()) {
    return;
  }

  mReceiving = true;

  SourceMediaStream* source = GetMediaStream()->GetStream()->AsSourceStream();
  source->FinishAddTracks();
  source->SetPullEnabled(true);
  
  
  
  
  
  source->AdvanceKnownTracksTime(STREAM_TIME_MAX);
  CSFLogDebug(logTag, "Finished adding tracks to MediaStream %p", source);
}

RefPtr<MediaPipeline> SourceStreamInfo::GetPipelineByTrackId_m(
    const std::string& trackId) {
  ASSERT_ON_THREAD(mParent->GetMainThread());

  
  
  
  
  
  if (mMediaStream) {
    if (mPipelines.count(trackId)) {
      return mPipelines[trackId];
    }
  }

  return nullptr;
}

TemporaryRef<MediaPipeline>
LocalSourceStreamInfo::ForgetPipelineByTrackId_m(const std::string& trackId)
{
  ASSERT_ON_THREAD(mParent->GetMainThread());

  
  
  
  
  
  if (mMediaStream) {
    if (mPipelines.count(trackId)) {
      RefPtr<MediaPipeline> pipeline(mPipelines[trackId]);
      mPipelines.erase(trackId);
      return pipeline.forget();
    }
  }

  return nullptr;
}

}  
