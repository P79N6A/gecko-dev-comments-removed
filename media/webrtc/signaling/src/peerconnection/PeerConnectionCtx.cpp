



#include "CSFLog.h"

#include "base/histogram.h"
#include "CallControlManager.h"
#include "CC_Device.h"
#include "CC_Call.h"
#include "CC_Observer.h"
#include "ccapi_call_info.h"
#include "CC_SIPCCCallInfo.h"
#include "ccapi_device_info.h"
#include "CC_SIPCCDeviceInfo.h"
#include "vcm.h"
#include "VcmSIPCCBinding.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionCtx.h"
#include "runnable_utils.h"
#include "cpr_socket.h"
#include "debug-psipcc-types.h"
#include "prcvar.h"

#include "mozilla/Telemetry.h"

#ifdef MOZILLA_INTERNAL_API
#include "mozilla/dom/RTCPeerConnectionBinding.h"
#include "mozilla/Preferences.h"
#endif

#include "nsNetCID.h" 
#include "nsServiceManagerUtils.h" 
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "mozilla/Services.h"
#include "StaticPtr.h"
extern "C" {
#include "../sipcc/core/common/thread_monitor.h"
}

static const char* logTag = "PeerConnectionCtx";

extern "C" {
extern PRCondVar *ccAppReadyToStartCond;
extern PRLock *ccAppReadyToStartLock;
extern char ccAppReadyToStart;
}

namespace mozilla {

using namespace dom;



#ifdef MOZILLA_INTERNAL_API
static void
Apply(const Optional<bool> &aSrc, cc_boolean_constraint_t *aDst,
      bool mandatory = false) {
  if (aSrc.WasPassed() && (mandatory || !aDst->was_passed)) {
    aDst->was_passed = true;
    aDst->value = aSrc.Value();
    aDst->mandatory = mandatory;
  }
}
#endif

MediaConstraintsExternal::MediaConstraintsExternal() {
  memset(&mConstraints, 0, sizeof(mConstraints));
}

MediaConstraintsExternal::MediaConstraintsExternal(
    const MediaConstraintsInternal &aSrc) {
  cc_media_constraints_t* c = &mConstraints;
  memset(c, 0, sizeof(*c));
#ifdef MOZILLA_INTERNAL_API
  Apply(aSrc.mMandatory.mOfferToReceiveAudio, &c->offer_to_receive_audio, true);
  Apply(aSrc.mMandatory.mOfferToReceiveVideo, &c->offer_to_receive_video, true);
  if (!Preferences::GetBool("media.peerconnection.video.enabled", true)) {
    c->offer_to_receive_video.was_passed = true;
    c->offer_to_receive_video.value = false;
  }
  Apply(aSrc.mMandatory.mMozDontOfferDataChannel, &c->moz_dont_offer_datachannel,
        true);
  Apply(aSrc.mMandatory.mMozBundleOnly, &c->moz_bundle_only, true);
  if (aSrc.mOptional.WasPassed()) {
    const Sequence<MediaConstraintSet> &array = aSrc.mOptional.Value();
    for (uint32_t i = 0; i < array.Length(); i++) {
      Apply(array[i].mOfferToReceiveAudio, &c->offer_to_receive_audio);
      Apply(array[i].mOfferToReceiveVideo, &c->offer_to_receive_video);
      Apply(array[i].mMozDontOfferDataChannel, &c->moz_dont_offer_datachannel);
      Apply(array[i].mMozBundleOnly, &c->moz_bundle_only);
    }
  }
#endif
}

cc_media_constraints_t*
MediaConstraintsExternal::build() const {
  cc_media_constraints_t* cc  = (cc_media_constraints_t*)
    cpr_malloc(sizeof(cc_media_constraints_t));
  if (cc) {
    *cc = mConstraints;
  }
  return cc;
}

class PeerConnectionCtxShutdown : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS

  PeerConnectionCtxShutdown() {}

  void Init()
    {
      nsCOMPtr<nsIObserverService> observerService =
        services::GetObserverService();
      if (!observerService)
        return;

      nsresult rv = NS_OK;

#ifdef MOZILLA_INTERNAL_API
      rv = observerService->AddObserver(this,
                                        NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                        false);
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(rv));
#endif
      (void) rv;
    }

  virtual ~PeerConnectionCtxShutdown()
    {
      nsCOMPtr<nsIObserverService> observerService =
        services::GetObserverService();
      if (observerService)
        observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    }

  NS_IMETHODIMP Observe(nsISupports* aSubject, const char* aTopic,
                        const char16_t* aData) {
    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
      CSFLogDebug(logTag, "Shutting down PeerConnectionCtx");
      sipcc::PeerConnectionCtx::Destroy();

      nsCOMPtr<nsIObserverService> observerService =
        services::GetObserverService();
      if (!observerService)
        return NS_ERROR_FAILURE;

      nsresult rv = observerService->RemoveObserver(this,
                                                    NS_XPCOM_SHUTDOWN_OBSERVER_ID);
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(rv));

      
      nsRefPtr<PeerConnectionCtxShutdown> kungFuDeathGrip(this);
      sipcc::PeerConnectionCtx::gPeerConnectionCtxShutdown = nullptr;
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS(PeerConnectionCtxShutdown, nsIObserver);
}

using namespace mozilla;
namespace sipcc {

PeerConnectionCtx* PeerConnectionCtx::gInstance;
nsIThread* PeerConnectionCtx::gMainThread;
StaticRefPtr<PeerConnectionCtxShutdown> PeerConnectionCtx::gPeerConnectionCtxShutdown;





static void thread_ended_dispatcher(thread_ended_funct func, thread_monitor_id_t id)
{
  nsresult rv = PeerConnectionCtx::gMainThread->Dispatch(WrapRunnableNM(func, id),
                                                         NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    CSFLogError( logTag, "%s(): Could not dispatch to main thread", __FUNCTION__);
  }
}

static void join_waiter() {
  NS_ProcessPendingEvents(PeerConnectionCtx::gMainThread);
}

nsresult PeerConnectionCtx::InitializeGlobal(nsIThread *mainThread,
  nsIEventTarget* stsThread) {
  if (!gMainThread) {
    gMainThread = mainThread;
    CSF::VcmSIPCCBinding::setMainThread(gMainThread);
    init_thread_monitor(&thread_ended_dispatcher, &join_waiter);
  } else {
    MOZ_ASSERT(gMainThread == mainThread);
  }

  CSF::VcmSIPCCBinding::setSTSThread(stsThread);

  nsresult res;

  MOZ_ASSERT(NS_IsMainThread());

  if (!gInstance) {
    CSFLogDebug(logTag, "Creating PeerConnectionCtx");
    PeerConnectionCtx *ctx = new PeerConnectionCtx();

    res = ctx->Initialize();
    PR_ASSERT(NS_SUCCEEDED(res));
    if (!NS_SUCCEEDED(res))
      return res;

    gInstance = ctx;

    if (!sipcc::PeerConnectionCtx::gPeerConnectionCtxShutdown) {
      sipcc::PeerConnectionCtx::gPeerConnectionCtxShutdown = new PeerConnectionCtxShutdown();
      sipcc::PeerConnectionCtx::gPeerConnectionCtxShutdown->Init();
    }
  }

  return NS_OK;
}

PeerConnectionCtx* PeerConnectionCtx::GetInstance() {
  MOZ_ASSERT(gInstance);
  return gInstance;
}

bool PeerConnectionCtx::isActive() {
  return gInstance;
}

void PeerConnectionCtx::Destroy() {
  CSFLogDebug(logTag, "%s", __FUNCTION__);

  if (gInstance) {
    gInstance->Cleanup();
    delete gInstance;
    gInstance = nullptr;
  }
}

#ifdef MOZILLA_INTERNAL_API
typedef Vector<nsAutoPtr<RTCStatsQuery>> RTCStatsQueries;







static void
FreeOnMain_m(nsAutoPtr<RTCStatsQueries> aQueryList) {
  MOZ_ASSERT(NS_IsMainThread());
}

static void
EverySecondTelemetryCallback_s(nsAutoPtr<RTCStatsQueries> aQueryList) {
  using namespace Telemetry;

  for (auto q = aQueryList->begin(); q != aQueryList->end(); ++q) {
    PeerConnectionImpl::ExecuteStatsQuery_s(*q);
    auto& r = *(*q)->report;
    if (r.mInboundRTPStreamStats.WasPassed()) {
      auto& array = r.mInboundRTPStreamStats.Value();
      for (uint32_t i = 0; i < array.Length(); i++) {
        auto& s = array[i];
        MOZ_ASSERT(s.mId.WasPassed());
        bool isAudio = (s.mId.Value().Find("audio") != -1);
        if (s.mPacketsLost.WasPassed()) {
          Accumulate(s.mIsRemote?
                     (isAudio? WEBRTC_AUDIO_QUALITY_OUTBOUND_PACKETLOSS :
                               WEBRTC_VIDEO_QUALITY_OUTBOUND_PACKETLOSS) :
                     (isAudio? WEBRTC_AUDIO_QUALITY_INBOUND_PACKETLOSS :
                               WEBRTC_VIDEO_QUALITY_INBOUND_PACKETLOSS),
                      s.mPacketsLost.Value());
        }
        if (s.mJitter.WasPassed()) {
          Accumulate(s.mIsRemote?
                     (isAudio? WEBRTC_AUDIO_QUALITY_OUTBOUND_JITTER :
                               WEBRTC_VIDEO_QUALITY_OUTBOUND_JITTER) :
                     (isAudio? WEBRTC_AUDIO_QUALITY_INBOUND_JITTER :
                               WEBRTC_VIDEO_QUALITY_INBOUND_JITTER),
                      s.mJitter.Value());
        }
        if (s.mMozRtt.WasPassed()) {
          MOZ_ASSERT(s.mIsRemote);
          Accumulate(isAudio? WEBRTC_AUDIO_QUALITY_OUTBOUND_RTT :
                              WEBRTC_VIDEO_QUALITY_OUTBOUND_RTT,
                      s.mMozRtt.Value());
        }
      }
    }
  }
  
  NS_DispatchToMainThread(WrapRunnableNM(&FreeOnMain_m, aQueryList),
                          NS_DISPATCH_NORMAL);
}

void
PeerConnectionCtx::EverySecondTelemetryCallback_m(nsITimer* timer, void *closure) {
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(PeerConnectionCtx::isActive());
  auto ctx = static_cast<PeerConnectionCtx*>(closure);
  if (ctx->mPeerConnections.empty()) {
    return;
  }
  nsresult rv;
  nsCOMPtr<nsIEventTarget> stsThread =
      do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return;
  }
  MOZ_ASSERT(stsThread);

  nsAutoPtr<RTCStatsQueries> queries(new RTCStatsQueries);
  for (auto p = ctx->mPeerConnections.begin();
        p != ctx->mPeerConnections.end(); ++p) {
    if (p->second->HasMedia()) {
      queries->append(nsAutoPtr<RTCStatsQuery>(new RTCStatsQuery(true)));
      p->second->BuildStatsQuery_m(nullptr, 
                                   queries->back());
    }
  }
  rv = RUN_ON_THREAD(stsThread,
                     WrapRunnableNM(&EverySecondTelemetryCallback_s, queries),
                     NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS_VOID(rv);
}
#endif

nsresult PeerConnectionCtx::Initialize() {
  mCCM = CSF::CallControlManager::create();

  NS_ENSURE_TRUE(mCCM.get(), NS_ERROR_FAILURE);

  
  
  int codecMask = 0;
  codecMask |= VCM_CODEC_RESOURCE_G711;
  codecMask |= VCM_CODEC_RESOURCE_OPUS;
  
  
  
  
  mCCM->setAudioCodecs(codecMask);

  
  
  
  codecMask = 0;
  
  

#ifdef MOZILLA_INTERNAL_API
  if (Preferences::GetBool("media.peerconnection.video.h264_enabled")) {
    codecMask |= VCM_CODEC_RESOURCE_H264;
  }
#endif

  codecMask |= VCM_CODEC_RESOURCE_VP8;
  
  mCCM->setVideoCodecs(codecMask);

  ccAppReadyToStartLock = PR_NewLock();
  if (!ccAppReadyToStartLock) {
    return NS_ERROR_FAILURE;
  }

  ccAppReadyToStartCond = PR_NewCondVar(ccAppReadyToStartLock);
  if (!ccAppReadyToStartCond) {
    return NS_ERROR_FAILURE;
  }

  if (!mCCM->startSDPMode())
    return NS_ERROR_FAILURE;

  mDevice = mCCM->getActiveDevice();
  mCCM->addCCObserver(this);
  NS_ENSURE_TRUE(mDevice.get(), NS_ERROR_FAILURE);
  ChangeSipccState(dom::PCImplSipccState::Starting);

  
  
  PR_Lock(ccAppReadyToStartLock);
  ccAppReadyToStart = 1;
  PR_NotifyAllCondVar(ccAppReadyToStartCond);
  PR_Unlock(ccAppReadyToStartLock);

  mConnectionCounter = 0;
#ifdef MOZILLA_INTERNAL_API
  Telemetry::GetHistogramById(Telemetry::WEBRTC_CALL_COUNT)->Add(0);

  mTelemetryTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  MOZ_ASSERT(mTelemetryTimer);
  nsresult rv = mTelemetryTimer->SetTarget(gMainThread);
  NS_ENSURE_SUCCESS(rv, rv);
  mTelemetryTimer->InitWithFuncCallback(EverySecondTelemetryCallback_m, this, 1000,
                                        nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
#endif
  return NS_OK;
}

nsresult PeerConnectionCtx::Cleanup() {
  CSFLogDebug(logTag, "%s", __FUNCTION__);

  mCCM->destroy();
  mCCM->removeCCObserver(this);
  return NS_OK;
}

PeerConnectionCtx::~PeerConnectionCtx() {
    
  MOZ_ASSERT(NS_IsMainThread());
  if (mTelemetryTimer) {
    mTelemetryTimer->Cancel();
  }
};

CSF::CC_CallPtr PeerConnectionCtx::createCall() {
  return mDevice->createCall();
}

void PeerConnectionCtx::onDeviceEvent(ccapi_device_event_e aDeviceEvent,
                                      CSF::CC_DevicePtr aDevice,
                                      CSF::CC_DeviceInfoPtr aInfo ) {
  cc_service_state_t state = aInfo->getServiceState();
  
  
  dom::PCImplSipccState currentSipccState = mSipccState;

  switch (aDeviceEvent) {
    case CCAPI_DEVICE_EV_STATE:
      CSFLogDebug(logTag, "%s - %d : %d", __FUNCTION__, state,
                  static_cast<uint32_t>(currentSipccState));

      if (CC_STATE_INS == state) {
        
        if (dom::PCImplSipccState::Starting == currentSipccState ||
            dom::PCImplSipccState::Idle == currentSipccState) {
          ChangeSipccState(dom::PCImplSipccState::Started);
        } else {
          CSFLogError(logTag, "%s PeerConnection already started", __FUNCTION__);
        }
      } else {
        NS_NOTREACHED("Unsupported Signaling State Transition");
      }
      break;
    default:
      CSFLogDebug(logTag, "%s: Ignoring event: %s\n",__FUNCTION__,
                  device_event_getname(aDeviceEvent));
  }
}

static void onCallEvent_m(nsAutoPtr<std::string> peerconnection,
                          ccapi_call_event_e aCallEvent,
                          CSF::CC_CallInfoPtr aInfo);

void PeerConnectionCtx::onCallEvent(ccapi_call_event_e aCallEvent,
                                    CSF::CC_CallPtr aCall,
                                    CSF::CC_CallInfoPtr aInfo) {
  
  
  
  
  
  
  nsAutoPtr<std::string> pcDuped(new std::string(aCall->getPeerConnection()));

  
  nsresult rv = gMainThread->Dispatch(WrapRunnableNM(&onCallEvent_m, pcDuped,
                                                     aCallEvent, aInfo),
                                      NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    CSFLogError( logTag, "%s(): Could not dispatch to main thread", __FUNCTION__);
  }
}


static void onCallEvent_m(nsAutoPtr<std::string> peerconnection,
                          ccapi_call_event_e aCallEvent,
                          CSF::CC_CallInfoPtr aInfo) {
  CSFLogDebug(logTag, "onCallEvent()");
  PeerConnectionWrapper pc(peerconnection->c_str());
  if (!pc.impl())  
    return;
  CSFLogDebug(logTag, "Calling PC");
  pc.impl()->onCallEvent(OnCallEventArgs(aCallEvent, aInfo));
}

}  
