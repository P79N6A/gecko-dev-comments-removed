



#include "CSFLog.h"

#include "base/histogram.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionCtx.h"
#include "runnable_utils.h"
#include "prcvar.h"

#include "mozilla/Telemetry.h"

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
#include "mozilla/dom/RTCPeerConnectionBinding.h"
#include "mozilla/Preferences.h"
#include <mozilla/Types.h>
#endif

#include "nsNetCID.h" 
#include "nsServiceManagerUtils.h" 
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "mozilla/Services.h"
#include "StaticPtr.h"

#include "gmp-video-decode.h" 
#include "gmp-video-encode.h" 

static const char* logTag = "PeerConnectionCtx";

namespace mozilla {

using namespace dom;

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

  NS_IMETHODIMP Observe(nsISupports* aSubject, const char* aTopic,
                        const char16_t* aData) override {
    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
      CSFLogDebug(logTag, "Shutting down PeerConnectionCtx");
      PeerConnectionCtx::Destroy();

      nsCOMPtr<nsIObserverService> observerService =
        services::GetObserverService();
      if (!observerService)
        return NS_ERROR_FAILURE;

      nsresult rv = observerService->RemoveObserver(this,
                                                    NS_XPCOM_SHUTDOWN_OBSERVER_ID);
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(rv));

      
      nsRefPtr<PeerConnectionCtxShutdown> kungFuDeathGrip(this);
      PeerConnectionCtx::gPeerConnectionCtxShutdown = nullptr;
    }
    return NS_OK;
  }

private:
  virtual ~PeerConnectionCtxShutdown()
    {
      nsCOMPtr<nsIObserverService> observerService =
        services::GetObserverService();
      if (observerService)
        observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    }
};

NS_IMPL_ISUPPORTS(PeerConnectionCtxShutdown, nsIObserver);
}

namespace mozilla {

PeerConnectionCtx* PeerConnectionCtx::gInstance;
nsIThread* PeerConnectionCtx::gMainThread;
StaticRefPtr<PeerConnectionCtxShutdown> PeerConnectionCtx::gPeerConnectionCtxShutdown;

const std::map<const std::string, PeerConnectionImpl *>&
PeerConnectionCtx::mGetPeerConnections()
{
  return mPeerConnections;
}

nsresult PeerConnectionCtx::InitializeGlobal(nsIThread *mainThread,
  nsIEventTarget* stsThread) {
  if (!gMainThread) {
    gMainThread = mainThread;
  } else {
    MOZ_ASSERT(gMainThread == mainThread);
  }

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

    if (!PeerConnectionCtx::gPeerConnectionCtxShutdown) {
      PeerConnectionCtx::gPeerConnectionCtxShutdown = new PeerConnectionCtxShutdown();
      PeerConnectionCtx::gPeerConnectionCtxShutdown->Init();
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

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
typedef Vector<nsAutoPtr<RTCStatsQuery>> RTCStatsQueries;







static auto
FindId(const Sequence<RTCInboundRTPStreamStats>& aArray,
       const nsString &aId) -> decltype(aArray.Length()) {
  for (decltype(aArray.Length()) i = 0; i < aArray.Length(); i++) {
    if (aArray[i].mId.Value() == aId) {
      return i;
    }
  }
  return aArray.NoIndex;
}

static auto
FindId(const nsTArray<nsAutoPtr<RTCStatsReportInternal>>& aArray,
       const nsString &aId) -> decltype(aArray.Length()) {
  for (decltype(aArray.Length()) i = 0; i < aArray.Length(); i++) {
    if (aArray[i]->mPcid == aId) {
      return i;
    }
  }
  return aArray.NoIndex;
}

static void
FreeOnMain_m(nsAutoPtr<RTCStatsQueries> aQueryList) {
  MOZ_ASSERT(NS_IsMainThread());
}

static void
EverySecondTelemetryCallback_s(nsAutoPtr<RTCStatsQueries> aQueryList) {
  using namespace Telemetry;

  if(!PeerConnectionCtx::isActive()) {
    return;
  }
  PeerConnectionCtx *ctx = PeerConnectionCtx::GetInstance();

  for (auto q = aQueryList->begin(); q != aQueryList->end(); ++q) {
    PeerConnectionImpl::ExecuteStatsQuery_s(*q);
    auto& r = *(*q)->report;
    if (r.mInboundRTPStreamStats.WasPassed()) {
      
      const Sequence<RTCInboundRTPStreamStats> *lastInboundStats = nullptr;
      {
        auto i = FindId(ctx->mLastReports, r.mPcid);
        if (i != ctx->mLastReports.NoIndex) {
          lastInboundStats = &ctx->mLastReports[i]->mInboundRTPStreamStats.Value();
        }
      }
      
      auto& array = r.mInboundRTPStreamStats.Value();
      for (decltype(array.Length()) i = 0; i < array.Length(); i++) {
        auto& s = array[i];
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
        if (lastInboundStats && s.mBytesReceived.WasPassed()) {
          auto& laststats = *lastInboundStats;
          auto i = FindId(laststats, s.mId.Value());
          if (i != laststats.NoIndex) {
            auto& lasts = laststats[i];
            if (lasts.mBytesReceived.WasPassed()) {
              auto delta_ms = int32_t(s.mTimestamp.Value() -
                                      lasts.mTimestamp.Value());
              if (delta_ms > 0 && delta_ms < 60000) {
                Accumulate(s.mIsRemote?
                           (isAudio? WEBRTC_AUDIO_QUALITY_OUTBOUND_BANDWIDTH_KBITS :
                                     WEBRTC_VIDEO_QUALITY_OUTBOUND_BANDWIDTH_KBITS) :
                           (isAudio? WEBRTC_AUDIO_QUALITY_INBOUND_BANDWIDTH_KBITS :
                                     WEBRTC_VIDEO_QUALITY_INBOUND_BANDWIDTH_KBITS),
                           ((s.mBytesReceived.Value() -
                             lasts.mBytesReceived.Value()) * 8) / delta_ms);
              }
            }
          }
        }
      }
    }
  }
  
  ctx->mLastReports.Clear();
  for (auto q = aQueryList->begin(); q != aQueryList->end(); ++q) {
    ctx->mLastReports.AppendElement((*q)->report.forget()); 
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
      if (NS_WARN_IF(NS_FAILED(p->second->BuildStatsQuery_m(nullptr, 
                                                            queries->back())))) {
        queries->popBack();
      } else {
        MOZ_ASSERT(queries->back()->report);
      }
    }
  }
  if (!queries->empty()) {
    rv = RUN_ON_THREAD(stsThread,
                       WrapRunnableNM(&EverySecondTelemetryCallback_s, queries),
                       NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS_VOID(rv);
  }
}
#endif

nsresult PeerConnectionCtx::Initialize() {
  initGMP();

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  mConnectionCounter = 0;
  Telemetry::GetHistogramById(Telemetry::WEBRTC_CALL_COUNT)->Add(0);

  mTelemetryTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  MOZ_ASSERT(mTelemetryTimer);
  nsresult rv = mTelemetryTimer->SetTarget(gMainThread);
  NS_ENSURE_SUCCESS(rv, rv);
  mTelemetryTimer->InitWithFuncCallback(EverySecondTelemetryCallback_m, this, 1000,
                                        nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);

  if (XRE_IsContentProcess()) {
    WebrtcGlobalChild::Create();
  }
#endif 

  return NS_OK;
}

static void GMPReady_m() {
  if (PeerConnectionCtx::isActive()) {
    PeerConnectionCtx::GetInstance()->onGMPReady();
  }
};

static void GMPReady() {
  PeerConnectionCtx::gMainThread->Dispatch(WrapRunnableNM(&GMPReady_m),
                                           NS_DISPATCH_NORMAL);
};

void PeerConnectionCtx::initGMP()
{
  mGMPService = do_GetService("@mozilla.org/gecko-media-plugin-service;1");

  if (!mGMPService) {
    CSFLogError(logTag, "%s failed to get the gecko-media-plugin-service",
                __FUNCTION__);
    return;
  }

  nsCOMPtr<nsIThread> thread;
  nsresult rv = mGMPService->GetThread(getter_AddRefs(thread));

  if (NS_FAILED(rv)) {
    mGMPService = nullptr;
    CSFLogError(logTag,
                "%s failed to get the gecko-media-plugin thread, err=%u",
                __FUNCTION__,
                static_cast<unsigned>(rv));
    return;
  }

  
  thread->Dispatch(WrapRunnableNM(&GMPReady), NS_DISPATCH_NORMAL);
}

nsresult PeerConnectionCtx::Cleanup() {
  CSFLogDebug(logTag, "%s", __FUNCTION__);

  mQueuedJSEPOperations.Clear();
  mGMPService = nullptr;
  return NS_OK;
}

PeerConnectionCtx::~PeerConnectionCtx() {
    
  MOZ_ASSERT(NS_IsMainThread());
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  if (mTelemetryTimer) {
    mTelemetryTimer->Cancel();
  }
#endif
};

void PeerConnectionCtx::queueJSEPOperation(nsIRunnable* aOperation) {
  mQueuedJSEPOperations.AppendElement(aOperation);
}

void PeerConnectionCtx::onGMPReady() {
  mGMPReady = true;
  for (size_t i = 0; i < mQueuedJSEPOperations.Length(); ++i) {
    mQueuedJSEPOperations[i]->Run();
  }
  mQueuedJSEPOperations.Clear();
}

bool PeerConnectionCtx::gmpHasH264() {
  if (!mGMPService) {
    return false;
  }

  

  nsTArray<nsCString> tags;
  tags.AppendElement(NS_LITERAL_CSTRING("h264"));

  bool has_gmp;
  nsresult rv;
  rv = mGMPService->HasPluginForAPI(NS_LITERAL_CSTRING(GMP_API_VIDEO_ENCODER),
                                    &tags,
                                    &has_gmp);
  if (NS_FAILED(rv) || !has_gmp) {
    return false;
  }

  rv = mGMPService->HasPluginForAPI(NS_LITERAL_CSTRING(GMP_API_VIDEO_DECODER),
                                    &tags,
                                    &has_gmp);
  if (NS_FAILED(rv) || !has_gmp) {
    return false;
  }

  return true;
}

}  
