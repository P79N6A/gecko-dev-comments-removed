



#include "WebrtcGlobalInformation.h"

#include <deque>
#include <string>

#include "CSFLog.h"
#include "WebRtcLog.h"
#include "mozilla/dom/WebrtcGlobalInformationBinding.h"

#include "nsAutoPtr.h"
#include "nsNetCID.h" 
#include "nsServiceManagerUtils.h" 
#include "mozilla/ErrorResult.h"
#include "mozilla/Vector.h"
#include "nsProxyRelease.h"
#include "mozilla/Telemetry.h"

#include "rlogringbuffer.h"
#include "runnable_utils.h"
#include "PeerConnectionCtx.h"
#include "PeerConnectionImpl.h"
#include "webrtc/system_wrappers/interface/trace.h"

static const char* logTag = "WebrtcGlobalInformation";

namespace mozilla {

namespace dom {

typedef Vector<nsAutoPtr<RTCStatsQuery>> RTCStatsQueries;

static PeerConnectionCtx* GetPeerConnectionCtx()
{
  if(PeerConnectionCtx::isActive()) {
    MOZ_ASSERT(PeerConnectionCtx::GetInstance());
    return PeerConnectionCtx::GetInstance();
  }
  return nullptr;
}

static void OnStatsReport_m(
  nsMainThreadPtrHandle<WebrtcGlobalStatisticsCallback> aStatsCallback,
  nsAutoPtr<RTCStatsQueries> aQueryList)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aQueryList);

  WebrtcGlobalStatisticsReport report;
  report.mReports.Construct();

  
  for (auto q = aQueryList->begin(); q != aQueryList->end(); ++q) {
    MOZ_ASSERT(*q);
    report.mReports.Value().AppendElement(*(*q)->report);
  }

  PeerConnectionCtx* ctx = GetPeerConnectionCtx();
  if (ctx) {
    
    report.mReports.Value().AppendElements(ctx->mStatsForClosedPeerConnections);
  }

  ErrorResult rv;
  aStatsCallback.get()->Call(report, rv);

  if (rv.Failed()) {
    CSFLogError(logTag, "Error firing stats observer callback");
  }
}

static void GetAllStats_s(
  nsMainThreadPtrHandle<WebrtcGlobalStatisticsCallback> aStatsCallback,
  nsAutoPtr<RTCStatsQueries> aQueryList)
{
  MOZ_ASSERT(aQueryList);

  for (auto q = aQueryList->begin(); q != aQueryList->end(); ++q) {
    MOZ_ASSERT(*q);
    PeerConnectionImpl::ExecuteStatsQuery_s(*q);
  }

  NS_DispatchToMainThread(WrapRunnableNM(&OnStatsReport_m,
                                         aStatsCallback,
                                         aQueryList),
                          NS_DISPATCH_NORMAL);
}

static void OnGetLogging_m(
  nsMainThreadPtrHandle<WebrtcGlobalLoggingCallback> aLoggingCallback,
  const std::string& aPattern,
  nsAutoPtr<std::deque<std::string>> aLogList)
{
  ErrorResult rv;
  if (!aLogList->empty()) {
    Sequence<nsString> nsLogs;
    for (auto l = aLogList->begin(); l != aLogList->end(); ++l) {
      nsLogs.AppendElement(NS_ConvertUTF8toUTF16(l->c_str()));
    }
    aLoggingCallback.get()->Call(nsLogs, rv);
  }

  if (rv.Failed()) {
    CSFLogError(logTag, "Error firing logging observer callback");
  }
}

static void GetLogging_s(
  nsMainThreadPtrHandle<WebrtcGlobalLoggingCallback> aLoggingCallback,
  const std::string& aPattern)
{
  RLogRingBuffer* logs = RLogRingBuffer::GetInstance();
  nsAutoPtr<std::deque<std::string>> result(new std::deque<std::string>);
  
  if (logs) {
    logs->Filter(aPattern, 0, result);
  }
  NS_DispatchToMainThread(WrapRunnableNM(&OnGetLogging_m,
                                         aLoggingCallback,
                                         aPattern,
                                         result),
                          NS_DISPATCH_NORMAL);
}


void
WebrtcGlobalInformation::GetAllStats(
  const GlobalObject& aGlobal,
  WebrtcGlobalStatisticsCallback& aStatsCallback,
  const Optional<nsAString>& pcIdFilter,
  ErrorResult& aRv)
{
  if (!NS_IsMainThread()) {
    aRv.Throw(NS_ERROR_NOT_SAME_THREAD);
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIEventTarget> stsThread =
    do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  if (!stsThread) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  nsAutoPtr<RTCStatsQueries> queries(new RTCStatsQueries);

  
  
  PeerConnectionCtx *ctx = GetPeerConnectionCtx();
  if (ctx) {
    for (auto p = ctx->mPeerConnections.begin();
         p != ctx->mPeerConnections.end();
         ++p) {
      MOZ_ASSERT(p->second);

      if (!pcIdFilter.WasPassed() ||
          pcIdFilter.Value().EqualsASCII(p->second->GetIdAsAscii().c_str())) {
        if (p->second->HasMedia()) {
          queries->append(nsAutoPtr<RTCStatsQuery>(new RTCStatsQuery(true)));
          rv = p->second->BuildStatsQuery_m(nullptr, queries->back()); 
          if (NS_WARN_IF(NS_FAILED(rv))) {
            aRv.Throw(rv);
            return;
          }
          MOZ_ASSERT(queries->back()->report);
        }
      }
    }
  }

  
  
  nsMainThreadPtrHandle<WebrtcGlobalStatisticsCallback> callbackHandle(
    new nsMainThreadPtrHolder<WebrtcGlobalStatisticsCallback>(&aStatsCallback));

  rv = RUN_ON_THREAD(stsThread,
                     WrapRunnableNM(&GetAllStats_s, callbackHandle, queries),
                     NS_DISPATCH_NORMAL);
  aRv = rv;
}

void
WebrtcGlobalInformation::GetLogging(
  const GlobalObject& aGlobal,
  const nsAString& aPattern,
  WebrtcGlobalLoggingCallback& aLoggingCallback,
  ErrorResult& aRv)
{
  if (!NS_IsMainThread()) {
    aRv.Throw(NS_ERROR_NOT_SAME_THREAD);
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIEventTarget> stsThread =
    do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  if (!stsThread) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  std::string pattern(NS_ConvertUTF16toUTF8(aPattern).get());

  
  
  nsMainThreadPtrHandle<WebrtcGlobalLoggingCallback> callbackHandle(
    new nsMainThreadPtrHolder<WebrtcGlobalLoggingCallback>(&aLoggingCallback));

  rv = RUN_ON_THREAD(stsThread,
                     WrapRunnableNM(&GetLogging_s, callbackHandle, pattern),
                     NS_DISPATCH_NORMAL);

  if (NS_FAILED(rv)) {
    aLoggingCallback.Release();
  }

  aRv = rv;
}

static int32_t sLastSetLevel = 0;
static bool sLastAECDebug = false;

void
WebrtcGlobalInformation::SetDebugLevel(const GlobalObject& aGlobal, int32_t aLevel)
{
  StartWebRtcLog(webrtc::TraceLevel(aLevel));
  sLastSetLevel = aLevel;
}

int32_t
WebrtcGlobalInformation::DebugLevel(const GlobalObject& aGlobal)
{
  return sLastSetLevel;
}

void
WebrtcGlobalInformation::SetAecDebug(const GlobalObject& aGlobal, bool aEnable)
{
  StartWebRtcLog(sLastSetLevel); 
  webrtc::Trace::set_aec_debug(aEnable);
  sLastAECDebug = aEnable;
}

bool
WebrtcGlobalInformation::AecDebug(const GlobalObject& aGlobal)
{
  return sLastAECDebug;
}


struct StreamResult {
  StreamResult() : candidateTypeBitpattern(0), streamSucceeded(false) {}
  uint8_t candidateTypeBitpattern;
  bool streamSucceeded;
};

static void StoreLongTermICEStatisticsImpl_m(
    nsresult result,
    nsAutoPtr<RTCStatsQuery> query) {

  using namespace Telemetry;

  if (NS_FAILED(result) ||
      !query->error.empty() ||
      !query->report->mIceCandidateStats.WasPassed()) {
    return;
  }

  query->report->mClosed.Construct(true);

  
  enum {
    REMOTE_GATHERED_SERVER_REFLEXIVE = 1,
    REMOTE_GATHERED_TURN = 1 << 1,
    LOCAL_GATHERED_SERVER_REFLEXIVE = 1 << 2,
    LOCAL_GATHERED_TURN_UDP = 1 << 3,
    LOCAL_GATHERED_TURN_TCP = 1 << 4,
    LOCAL_GATHERED_TURN_TLS = 1 << 5,
    LOCAL_GATHERED_TURN_HTTPS = 1 << 6,
  };

  
  
  
  
  

  std::map<std::string, StreamResult> streamResults;

  
  for (size_t i = 0;
       i < query->report->mIceCandidatePairStats.Value().Length();
       ++i) {
    const RTCIceCandidatePairStats &pair =
      query->report->mIceCandidatePairStats.Value()[i];

    if (!pair.mState.WasPassed() || !pair.mComponentId.WasPassed()) {
      MOZ_CRASH();
      continue;
    }

    
    
    
    std::string streamId(
      NS_ConvertUTF16toUTF8(pair.mComponentId.Value()).get());

    streamResults[streamId].streamSucceeded |=
      pair.mState.Value() == RTCStatsIceCandidatePairState::Succeeded;
  }

  for (size_t i = 0;
       i < query->report->mIceCandidateStats.Value().Length();
       ++i) {
    const RTCIceCandidateStats &cand =
      query->report->mIceCandidateStats.Value()[i];

    if (!cand.mType.WasPassed() ||
        !cand.mCandidateType.WasPassed() ||
        !cand.mComponentId.WasPassed()) {
      
      MOZ_CRASH();
      continue;
    }

    
    
    
    std::string streamId(
      NS_ConvertUTF16toUTF8(cand.mComponentId.Value()).get());

    if (cand.mCandidateType.Value() == RTCStatsIceCandidateType::Relayed) {
      if (cand.mType.Value() == RTCStatsType::Localcandidate) {
        NS_ConvertUTF16toUTF8 transport(cand.mMozLocalTransport.Value());
        if (transport == kNrIceTransportUdp) {
          streamResults[streamId].candidateTypeBitpattern |=
            LOCAL_GATHERED_TURN_UDP;
        } else if (transport == kNrIceTransportTcp) {
          streamResults[streamId].candidateTypeBitpattern |=
            LOCAL_GATHERED_TURN_TCP;
        }
      } else {
        streamResults[streamId].candidateTypeBitpattern |= REMOTE_GATHERED_TURN;
      }
    } else if (cand.mCandidateType.Value() ==
               RTCStatsIceCandidateType::Serverreflexive) {
      if (cand.mType.Value() == RTCStatsType::Localcandidate) {
        streamResults[streamId].candidateTypeBitpattern |=
          LOCAL_GATHERED_SERVER_REFLEXIVE;
      } else {
        streamResults[streamId].candidateTypeBitpattern |=
          REMOTE_GATHERED_SERVER_REFLEXIVE;
      }
    }
  }

  for (auto i = streamResults.begin(); i != streamResults.end(); ++i) {
    if (i->second.streamSucceeded) {
      Telemetry::Accumulate(Telemetry::WEBRTC_CANDIDATE_TYPES_GIVEN_SUCCESS,
                            i->second.candidateTypeBitpattern);
    } else {
      Telemetry::Accumulate(Telemetry::WEBRTC_CANDIDATE_TYPES_GIVEN_FAILURE,
                            i->second.candidateTypeBitpattern);
    }
  }

  

  if (query->report->mOutboundRTPStreamStats.WasPassed()) {
    auto& array = query->report->mOutboundRTPStreamStats.Value();
    for (decltype(array.Length()) i = 0; i < array.Length(); i++) {
      auto& s = array[i];
      bool isVideo = (s.mId.Value().Find("video") != -1);
      if (!isVideo || s.mIsRemote) {
        continue;
      }
      if (s.mBitrateMean.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_ENCODER_BITRATE_AVG_PER_CALL_KBPS,
                   uint32_t(s.mBitrateMean.Value() / 1000));
      }
      if (s.mBitrateStdDev.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_ENCODER_BITRATE_STD_DEV_PER_CALL_KBPS,
                   uint32_t(s.mBitrateStdDev.Value() / 1000));
      }
      if (s.mFramerateMean.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_ENCODER_FRAMERATE_AVG_PER_CALL,
                   uint32_t(s.mFramerateMean.Value()));
      }
      if (s.mFramerateStdDev.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_ENCODER_FRAMERATE_10X_STD_DEV_PER_CALL,
                   uint32_t(s.mFramerateStdDev.Value() * 10));
      }
      if (s.mDroppedFrames.WasPassed() && !query->iceStartTime.IsNull()) {
        double mins = (TimeStamp::Now() - query->iceStartTime).ToSeconds() / 60;
        if (mins > 0) {
          Accumulate(WEBRTC_VIDEO_ENCODER_DROPPED_FRAMES_PER_CALL_FPM,
                     uint32_t(double(s.mDroppedFrames.Value()) / mins));
        }
      }
    }
  }

  if (query->report->mInboundRTPStreamStats.WasPassed()) {
    auto& array = query->report->mInboundRTPStreamStats.Value();
    for (decltype(array.Length()) i = 0; i < array.Length(); i++) {
      auto& s = array[i];
      bool isVideo = (s.mId.Value().Find("video") != -1);
      if (!isVideo || s.mIsRemote) {
        continue;
      }
      if (s.mBitrateMean.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_DECODER_BITRATE_AVG_PER_CALL_KBPS,
                   uint32_t(s.mBitrateMean.Value() / 1000));
      }
      if (s.mBitrateStdDev.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_DECODER_BITRATE_STD_DEV_PER_CALL_KBPS,
                   uint32_t(s.mBitrateStdDev.Value() / 1000));
      }
      if (s.mFramerateMean.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_DECODER_FRAMERATE_AVG_PER_CALL,
                   uint32_t(s.mFramerateMean.Value()));
      }
      if (s.mFramerateStdDev.WasPassed()) {
        Accumulate(WEBRTC_VIDEO_DECODER_FRAMERATE_10X_STD_DEV_PER_CALL,
                   uint32_t(s.mFramerateStdDev.Value() * 10));
      }
      if (s.mDiscardedPackets.WasPassed() && !query->iceStartTime.IsNull()) {
        double mins = (TimeStamp::Now() - query->iceStartTime).ToSeconds() / 60;
        if (mins > 0) {
          Accumulate(WEBRTC_VIDEO_DECODER_DISCARDED_PACKETS_PER_CALL_PPM,
                     uint32_t(double(s.mDiscardedPackets.Value()) / mins));
        }
      }
    }
  }

  

  PeerConnectionCtx *ctx = GetPeerConnectionCtx();
  if (ctx) {
    ctx->mStatsForClosedPeerConnections.AppendElement(*query->report);
  }
}

static void GetStatsForLongTermStorage_s(
    nsAutoPtr<RTCStatsQuery> query) {

  MOZ_ASSERT(query);

  nsresult rv = PeerConnectionImpl::ExecuteStatsQuery_s(query.get());

  
  
  unsigned char rate_limit_bit_pattern = 0;
  if (!mozilla::nr_socket_short_term_violation_time().IsNull() &&
      !query->iceStartTime.IsNull() &&
      mozilla::nr_socket_short_term_violation_time() >= query->iceStartTime) {
    rate_limit_bit_pattern |= 1;
  }
  if (!mozilla::nr_socket_long_term_violation_time().IsNull() &&
      !query->iceStartTime.IsNull() &&
      mozilla::nr_socket_long_term_violation_time() >= query->iceStartTime) {
    rate_limit_bit_pattern |= 2;
  }

  if (query->failed) {
    Telemetry::Accumulate(
        Telemetry::WEBRTC_STUN_RATE_LIMIT_EXCEEDED_BY_TYPE_GIVEN_FAILURE,
        rate_limit_bit_pattern);
  } else {
    Telemetry::Accumulate(
        Telemetry::WEBRTC_STUN_RATE_LIMIT_EXCEEDED_BY_TYPE_GIVEN_SUCCESS,
        rate_limit_bit_pattern);
  }

  
  
  NS_DispatchToMainThread(
      WrapRunnableNM(
          &StoreLongTermICEStatisticsImpl_m,
          rv,
          query),
      NS_DISPATCH_NORMAL);
}

void WebrtcGlobalInformation::StoreLongTermICEStatistics(
    PeerConnectionImpl& aPc) {
  Telemetry::Accumulate(Telemetry::WEBRTC_ICE_FINAL_CONNECTION_STATE,
                        static_cast<uint32_t>(aPc.IceConnectionState()));

  if (aPc.IceConnectionState() == PCImplIceConnectionState::New) {
    
    
    return;
  }

  nsAutoPtr<RTCStatsQuery> query(new RTCStatsQuery(true));

  nsresult rv = aPc.BuildStatsQuery_m(nullptr, query.get());

  NS_ENSURE_SUCCESS_VOID(rv);

  RUN_ON_THREAD(aPc.GetSTSThread(),
                WrapRunnableNM(&GetStatsForLongTermStorage_s,
                               query),
                NS_DISPATCH_NORMAL);
}


} 
} 

