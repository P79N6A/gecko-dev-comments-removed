



#include <cstdlib>
#include <cerrno>
#include <deque>
#include <sstream>

#include "base/histogram.h"
#include "vcm.h"
#include "CSFLog.h"
#include "timecard.h"
#include "ccapi_call_info.h"
#include "CC_SIPCCCallInfo.h"
#include "ccapi_device_info.h"
#include "CC_SIPCCDeviceInfo.h"
#include "cpr_string.h"
#include "cpr_stdlib.h"

#include "jsapi.h"
#include "nspr.h"
#include "nss.h"
#include "pk11pub.h"

#include "nsNetCID.h"
#include "nsIProperty.h"
#include "nsIPropertyBag2.h"
#include "nsIServiceManager.h"
#include "nsISimpleEnumerator.h"
#include "nsServiceManagerUtils.h"
#include "nsISocketTransportService.h"
#include "nsIConsoleService.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "prtime.h"

#include "AudioConduit.h"
#include "VideoConduit.h"
#include "runnable_utils.h"
#include "PeerConnectionCtx.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionMedia.h"
#include "nsDOMDataChannelDeclarations.h"
#include "dtlsidentity.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsPerformance.h"
#include "nsGlobalWindow.h"
#include "nsDOMDataChannel.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Telemetry.h"
#include "mozilla/PublicSSL.h"
#include "nsXULAppAPI.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "nsIDocument.h"
#include "nsIScriptError.h"
#include "nsPrintfCString.h"
#include "nsURLHelper.h"
#include "nsNetUtil.h"
#include "nsIDOMDataChannel.h"
#include "nsIDOMLocation.h"
#include "mozilla/dom/RTCConfigurationBinding.h"
#include "mozilla/dom/RTCStatsReportBinding.h"
#include "mozilla/dom/RTCPeerConnectionBinding.h"
#include "mozilla/dom/PeerConnectionImplBinding.h"
#include "mozilla/dom/DataChannelBinding.h"
#include "MediaStreamList.h"
#include "MediaStreamTrack.h"
#include "AudioStreamTrack.h"
#include "VideoStreamTrack.h"
#include "nsIScriptGlobalObject.h"
#include "DOMMediaStream.h"
#include "rlogringbuffer.h"
#endif

#ifndef USE_FAKE_MEDIA_STREAMS
#include "MediaSegment.h"
#endif

#ifdef USE_FAKE_PCOBSERVER
#include "FakePCObserver.h"
#else
#include "mozilla/dom/PeerConnectionObserverBinding.h"
#endif
#include "mozilla/dom/PeerConnectionObserverEnumsBinding.h"

#define ICE_PARSING "In RTCConfiguration passed to RTCPeerConnection constructor"

using namespace mozilla;
using namespace mozilla::dom;

typedef PCObserverString ObString;

static const char* logTag = "PeerConnectionImpl";

#ifdef MOZILLA_INTERNAL_API
static nsresult InitNSSInContent()
{
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_NOT_SAME_THREAD);

  if (XRE_GetProcessType() != GeckoProcessType_Content) {
    MOZ_ASSUME_UNREACHABLE("Must be called in content process");
  }

  static bool nssStarted = false;
  if (nssStarted) {
    return NS_OK;
  }

  if (NSS_NoDB_Init(nullptr) != SECSuccess) {
    CSFLogError(logTag, "NSS_NoDB_Init failed.");
    return NS_ERROR_FAILURE;
  }

  if (NS_FAILED(mozilla::psm::InitializeCipherSuite())) {
    CSFLogError(logTag, "Fail to set up nss cipher suite.");
    return NS_ERROR_FAILURE;
  }

  mozilla::psm::DisableMD5();

  nssStarted = true;

  return NS_OK;
}
#endif 

namespace mozilla {
  class DataChannel;
}

class nsIDOMDataChannel;

static const int DTLS_FINGERPRINT_LENGTH = 64;
static const int MEDIA_STREAM_MUTE = 0x80;

PRLogModuleInfo *signalingLogInfo() {
  static PRLogModuleInfo *logModuleInfo = nullptr;
  if (!logModuleInfo) {
    logModuleInfo = PR_NewLogModule("signaling");
  }
  return logModuleInfo;
}


namespace sipcc {


namespace {
class JSErrorResult : public ErrorResult
{
public:
  ~JSErrorResult()
  {
#ifdef MOZILLA_INTERNAL_API
    WouldReportJSException();
    if (IsJSException()) {
      MOZ_ASSERT(NS_IsMainThread());
      AutoJSContext cx;
      Optional<JS::Handle<JS::Value> > value(cx);
      StealJSException(cx, &value.Value());
    }
#endif
  }
};











class WrappableJSErrorResult {
public:
  WrappableJSErrorResult() : isCopy(false) {}
  WrappableJSErrorResult(WrappableJSErrorResult &other) : mRv(), isCopy(true) {}
  ~WrappableJSErrorResult() {
    if (isCopy) {
#ifdef MOZILLA_INTERNAL_API
      MOZ_ASSERT(NS_IsMainThread());
#endif
    }
  }
  operator JSErrorResult &() { return mRv; }
private:
  JSErrorResult mRv;
  bool isCopy;
};
}

class PeerConnectionObserverDispatch : public nsRunnable {

public:
  PeerConnectionObserverDispatch(CSF::CC_CallInfoPtr aInfo,
                                 nsRefPtr<PeerConnectionImpl> aPC,
                                 PeerConnectionObserver* aObserver)
      : mPC(aPC),
        mObserver(aObserver),
        mCode(static_cast<PeerConnectionImpl::Error>(aInfo->getStatusCode())),
        mReason(aInfo->getStatus()),
        mSdpStr(),
	mCandidateStr(),
        mCallState(aInfo->getCallState()),
        mFsmState(aInfo->getFsmState()),
        mStateStr(aInfo->callStateToString(mCallState)),
        mFsmStateStr(aInfo->fsmStateToString(mFsmState)) {
    if (mCallState == REMOTESTREAMADD) {
      MediaStreamTable *streams = nullptr;
      streams = aInfo->getMediaStreams();
      mRemoteStream = mPC->media()->GetRemoteStream(streams->media_stream_id);
      MOZ_ASSERT(mRemoteStream);
    } else if (mCallState == FOUNDICECANDIDATE) {
	mCandidateStr = aInfo->getCandidate();
    } else if ((mCallState == CREATEOFFERSUCCESS) ||
	       (mCallState == CREATEANSWERSUCCESS)) {
        mSdpStr = aInfo->getSDP();
    }
  }

  ~PeerConnectionObserverDispatch(){}

#ifdef MOZILLA_INTERNAL_API
  class TracksAvailableCallback : public DOMMediaStream::OnTracksAvailableCallback
  {
  public:
    TracksAvailableCallback(DOMMediaStream::TrackTypeHints aTrackTypeHints,
                            nsRefPtr<PeerConnectionObserver> aObserver)
    : DOMMediaStream::OnTracksAvailableCallback(aTrackTypeHints)
    , mObserver(aObserver) {}

    virtual void NotifyTracksAvailable(DOMMediaStream* aStream) MOZ_OVERRIDE
    {
      MOZ_ASSERT(NS_IsMainThread());

      
      
      aStream->SetLogicalStreamStartTime(aStream->GetStream()->GetCurrentTime());

      CSFLogInfo(logTag, "Returning success for OnAddStream()");
      
      
      JSErrorResult rv;
      mObserver->OnAddStream(*aStream, rv);
      if (rv.Failed()) {
        CSFLogError(logTag, ": OnAddStream() failed! Error: %d", rv.ErrorCode());
      }
    }
  private:
    nsRefPtr<PeerConnectionObserver> mObserver;
  };
#endif

  NS_IMETHOD Run() {

    CSFLogInfo(logTag, "PeerConnectionObserverDispatch processing "
               "mCallState = %d (%s), mFsmState = %d (%s)",
               mCallState, mStateStr.c_str(), mFsmState, mFsmStateStr.c_str());

    if (mCallState == SETLOCALDESCERROR || mCallState == SETREMOTEDESCERROR) {
      const std::vector<std::string> &errors = mPC->GetSdpParseErrors();
      std::vector<std::string>::const_iterator i;
      for (i = errors.begin(); i != errors.end(); ++i) {
        mReason += " | SDP Parsing Error: " + *i;
      }
      if (errors.size()) {
        mCode = PeerConnectionImpl::kInvalidSessionDescription;
      }
      mPC->ClearSdpParseErrorMessages();
    }

    if (mReason.length()) {
      CSFLogInfo(logTag, "Message contains error: %d: %s",
                 mCode, mReason.c_str());
    }

    







    if (mFsmState >= FSMDEF_S_STABLE && mFsmState <= FSMDEF_S_CLOSED) {
      int offset = FSMDEF_S_STABLE - int(PCImplSignalingState::SignalingStable);
      mPC->SetSignalingState_m(static_cast<PCImplSignalingState>(mFsmState - offset));
    } else {
      CSFLogError(logTag, ": **** UNHANDLED SIGNALING STATE : %d (%s)",
                  mFsmState, mFsmStateStr.c_str());
    }

    JSErrorResult rv;

    switch (mCallState) {
      case CREATEOFFERSUCCESS:
        mObserver->OnCreateOfferSuccess(ObString(mSdpStr.c_str()), rv);
        break;

      case CREATEANSWERSUCCESS:
        mObserver->OnCreateAnswerSuccess(ObString(mSdpStr.c_str()), rv);
        break;

      case CREATEOFFERERROR:
        mObserver->OnCreateOfferError(mCode, ObString(mReason.c_str()), rv);
        break;

      case CREATEANSWERERROR:
        mObserver->OnCreateAnswerError(mCode, ObString(mReason.c_str()), rv);
        break;

      case SETLOCALDESCSUCCESS:
        
        
        
        
        
        mPC->ClearSdpParseErrorMessages();
        mObserver->OnSetLocalDescriptionSuccess(rv);
        break;

      case SETREMOTEDESCSUCCESS:
        
        
        
        
        
        mPC->ClearSdpParseErrorMessages();
        mObserver->OnSetRemoteDescriptionSuccess(rv);
#ifdef MOZILLA_INTERNAL_API
        mPC->startCallTelem();
#endif
        break;

      case SETLOCALDESCERROR:
        mObserver->OnSetLocalDescriptionError(mCode,
                                              ObString(mReason.c_str()), rv);
        break;

      case SETREMOTEDESCERROR:
        mObserver->OnSetRemoteDescriptionError(mCode,
                                               ObString(mReason.c_str()), rv);
        break;

      case ADDICECANDIDATE:
        mObserver->OnAddIceCandidateSuccess(rv);
        break;

      case ADDICECANDIDATEERROR:
        mObserver->OnAddIceCandidateError(mCode, ObString(mReason.c_str()), rv);
        break;

      case FOUNDICECANDIDATE:
        {
            size_t end_of_level = mCandidateStr.find('\t');
            if (end_of_level == std::string::npos) {
                MOZ_ASSERT(false);
                return NS_OK;
            }
            std::string level = mCandidateStr.substr(0, end_of_level);
            if (!level.size()) {
                MOZ_ASSERT(false);
                return NS_OK;
            }
            char *endptr;
            errno = 0;
            unsigned long level_long =
                strtoul(level.c_str(), &endptr, 10);
            if (errno || *endptr != 0 || level_long > 65535) {
                
                MOZ_ASSERT(false);
                return NS_OK;
            }
            size_t end_of_mid = mCandidateStr.find('\t', end_of_level + 1);
            if (end_of_mid == std::string::npos) {
                MOZ_ASSERT(false);
                return NS_OK;
            }

            std::string mid = mCandidateStr.substr(end_of_level + 1,
                                                   end_of_mid - (end_of_level + 1));

            std::string candidate = mCandidateStr.substr(end_of_mid + 1);

            mObserver->OnIceCandidate(level_long & 0xffff,
                                      ObString(mid.c_str()),
                                      ObString(candidate.c_str()), rv);
        }
        break;
      case REMOTESTREAMADD:
        {
          DOMMediaStream* stream = nullptr;

          if (!mRemoteStream) {
            CSFLogError(logTag, "%s: GetRemoteStream returned NULL", __FUNCTION__);
          } else {
            stream = mRemoteStream->GetMediaStream();
          }

          if (!stream) {
            CSFLogError(logTag, "%s: GetMediaStream returned NULL", __FUNCTION__);
          } else {
#ifdef MOZILLA_INTERNAL_API
            TracksAvailableCallback* tracksAvailableCallback =
              new TracksAvailableCallback(mRemoteStream->mTrackTypeHints, mObserver);

            stream->OnTracksAvailable(tracksAvailableCallback);
#else
            mObserver->OnAddStream(stream, rv);
#endif
          }
          break;
        }

      case UPDATELOCALDESC:
        
        break;

      default:
        CSFLogError(logTag, ": **** UNHANDLED CALL STATE : %d (%s)",
                    mCallState, mStateStr.c_str());
        break;
    }
    return NS_OK;
  }

private:
  nsRefPtr<PeerConnectionImpl> mPC;
  nsRefPtr<PeerConnectionObserver> mObserver;
  PeerConnectionImpl::Error mCode;
  std::string mReason;
  std::string mSdpStr;
  std::string mCandidateStr;
  cc_call_state_t mCallState;
  fsmdef_states_t mFsmState;
  std::string mStateStr;
  std::string mFsmStateStr;
  nsRefPtr<RemoteSourceStreamInfo> mRemoteStream;
};

NS_IMPL_ISUPPORTS0(PeerConnectionImpl)

#ifdef MOZILLA_INTERNAL_API
JSObject*
PeerConnectionImpl::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return PeerConnectionImplBinding::Wrap(aCx, aScope, this);
}
#endif

struct PeerConnectionImpl::Internal {
  CSF::CC_CallPtr mCall;
};

PeerConnectionImpl::PeerConnectionImpl(const GlobalObject* aGlobal)
: mTimeCard(PR_LOG_TEST(signalingLogInfo(),PR_LOG_ERROR) ?
            create_timecard() : nullptr)
  , mInternal(new Internal())
  , mReadyState(PCImplReadyState::New)
  , mSignalingState(PCImplSignalingState::SignalingStable)
  , mIceConnectionState(PCImplIceConnectionState::New)
  , mIceGatheringState(PCImplIceGatheringState::New)
  , mWindow(nullptr)
  , mIdentity(nullptr)
  , mSTSThread(nullptr)
  , mMedia(nullptr)
  , mNumAudioStreams(0)
  , mNumVideoStreams(0)
  , mHaveDataStream(false)
  , mTrickle(true) 
{
#ifdef MOZILLA_INTERNAL_API
  MOZ_ASSERT(NS_IsMainThread());
  if (aGlobal) {
    mWindow = do_QueryInterface(aGlobal->GetAsSupports());
  }
#endif
  MOZ_ASSERT(mInternal);
  CSFLogInfo(logTag, "%s: PeerConnectionImpl constructor for %s",
             __FUNCTION__, mHandle.c_str());
  STAMP_TIMECARD(mTimeCard, "Constructor Completed");
}

PeerConnectionImpl::~PeerConnectionImpl()
{
  if (mTimeCard) {
    STAMP_TIMECARD(mTimeCard, "Destructor Invoked");
    print_timecard(mTimeCard);
    destroy_timecard(mTimeCard);
    mTimeCard = nullptr;
  }
  
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  if (PeerConnectionCtx::isActive()) {
    PeerConnectionCtx::GetInstance()->mPeerConnections.erase(mHandle);
  } else {
    CSFLogError(logTag, "PeerConnectionCtx is already gone. Ignoring...");
  }

  CSFLogInfo(logTag, "%s: PeerConnectionImpl destructor invoked for %s",
             __FUNCTION__, mHandle.c_str());
  CloseInt();

#ifdef MOZILLA_INTERNAL_API
  {
    
    nsNSSShutDownPreventionLock locker;
    if (!isAlreadyShutDown()) {
      destructorSafeDestroyNSSReference();
      shutdown(calledFromObject);
    }
  }
#endif

  
  

  
  
  
  
}

already_AddRefed<DOMMediaStream>
PeerConnectionImpl::MakeMediaStream(nsPIDOMWindow* aWindow,
                                    uint32_t aHint)
{
  nsRefPtr<DOMMediaStream> stream =
    DOMMediaStream::CreateSourceStream(aWindow, aHint);
#ifdef MOZILLA_INTERNAL_API
  nsIDocument* doc = aWindow->GetExtantDoc();
  if (!doc) {
    return nullptr;
  }
  
  stream->CombineWithPrincipal(doc->NodePrincipal());
#endif

  CSFLogDebug(logTag, "Created media stream %p, inner: %p", stream.get(), stream->GetStream());

  return stream.forget();
}

nsresult
PeerConnectionImpl::CreateRemoteSourceStreamInfo(nsRefPtr<RemoteSourceStreamInfo>*
                                                 aInfo)
{
  MOZ_ASSERT(aInfo);
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

  
  
  
  
  nsRefPtr<DOMMediaStream> stream = MakeMediaStream(mWindow, 0);
  if (!stream) {
    return NS_ERROR_FAILURE;
  }

  static_cast<SourceMediaStream*>(stream->GetStream())->SetPullEnabled(true);

  nsRefPtr<RemoteSourceStreamInfo> remote;
  remote = new RemoteSourceStreamInfo(stream.forget(), mMedia);
  *aInfo = remote;

  return NS_OK;
}










nsresult
PeerConnectionImpl::ConvertRTCConfiguration(const RTCConfiguration& aSrc,
                                            IceConfiguration *aDst)
{
#ifdef MOZILLA_INTERNAL_API
  if (!aSrc.mIceServers.WasPassed()) {
    return NS_OK;
  }
  for (uint32_t i = 0; i < aSrc.mIceServers.Value().Length(); i++) {
    const RTCIceServer& server = aSrc.mIceServers.Value()[i];
    NS_ENSURE_TRUE(server.mUrl.WasPassed(), NS_ERROR_UNEXPECTED);

    
    
    
    
    
    nsRefPtr<nsIURI> url;
    nsresult rv = NS_NewURI(getter_AddRefs(url), server.mUrl.Value());
    NS_ENSURE_SUCCESS(rv, rv);
    bool isStun = false, isStuns = false, isTurn = false, isTurns = false;
    url->SchemeIs("stun", &isStun);
    url->SchemeIs("stuns", &isStuns);
    url->SchemeIs("turn", &isTurn);
    url->SchemeIs("turns", &isTurns);
    if (!(isStun || isStuns || isTurn || isTurns)) {
      return NS_ERROR_FAILURE;
    }
    nsAutoCString spec;
    rv = url->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    int32_t port;
    nsAutoCString host;
    nsAutoCString transport;
    {
      uint32_t hostPos;
      int32_t hostLen;
      nsAutoCString path;
      rv = url->GetPath(path);
      NS_ENSURE_SUCCESS(rv, rv);

      
      int32_t questionmark = path.FindChar('?');
      if (questionmark >= 0) {
        const nsCString match = NS_LITERAL_CSTRING("transport=");

        for (int32_t i = questionmark, endPos; i >= 0; i = endPos) {
          endPos = path.FindCharInSet("&", i + 1);
          const nsDependentCSubstring fieldvaluepair = Substring(path, i + 1,
                                                                 endPos);
          if (StringBeginsWith(fieldvaluepair, match)) {
            transport = Substring(fieldvaluepair, match.Length());
            ToLowerCase(transport);
          }
        }
        path.SetLength(questionmark);
      }

      rv = net_GetAuthURLParser()->ParseAuthority(path.get(), path.Length(),
                                                  nullptr,  nullptr,
                                                  nullptr,  nullptr,
                                                  &hostPos,  &hostLen, &port);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!hostLen) {
        return NS_ERROR_FAILURE;
      }
      if (hostPos > 1)  
        return NS_ERROR_FAILURE;
      path.Mid(host, hostPos, hostLen);
    }
    if (port == -1)
      port = (isStuns || isTurns)? 5349 : 3478;

    if (isTurn || isTurns) {
      NS_ConvertUTF16toUTF8 credential(server.mCredential);
      NS_ConvertUTF16toUTF8 username(server.mUsername);

#ifdef MOZ_WIDGET_GONK
      if (transport.get() == kNrIceTransportTcp)
          continue;
#endif
      if (!aDst->addTurnServer(host.get(), port,
                               username.get(),
                               credential.get(),
                               (transport.IsEmpty() ?
                                kNrIceTransportUdp : transport.get()))) {
        return NS_ERROR_FAILURE;
      }
    } else {
      if (!aDst->addStunServer(host.get(), port)) {
        return NS_ERROR_FAILURE;
      }
    }
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::Initialize(PeerConnectionObserver& aObserver,
                               nsGlobalWindow* aWindow,
                               const IceConfiguration* aConfiguration,
                               const RTCConfiguration* aRTCConfiguration,
                               nsISupports* aThread)
{
  nsresult res;

  
  MOZ_ASSERT(!aConfiguration != !aRTCConfiguration);
#ifdef MOZILLA_INTERNAL_API
  MOZ_ASSERT(NS_IsMainThread());
#endif
  MOZ_ASSERT(aThread);
  mThread = do_QueryInterface(aThread);

  mPCObserver = do_GetWeakReference(&aObserver);

  

  mSTSThread = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &res);
  MOZ_ASSERT(mSTSThread);
#ifdef MOZILLA_INTERNAL_API

  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    nsCOMPtr<nsISupports> nssDummy = do_GetService("@mozilla.org/psm;1", &res);
    NS_ENSURE_SUCCESS(res, res);
  } else {
    NS_ENSURE_SUCCESS(res = InitNSSInContent(), res);
  }

  
  
  MOZ_ASSERT(aWindow);
  mWindow = aWindow;
  NS_ENSURE_STATE(mWindow);

#endif 

  PRTime timestamp = PR_Now();
  
  char temp[128];

#ifdef MOZILLA_INTERNAL_API
  nsIDOMLocation* location = nullptr;
  mWindow->GetLocation(&location);
  MOZ_ASSERT(location);
  nsString locationAStr;
  location->ToString(locationAStr);
  location->Release();

  nsCString locationCStr;
  CopyUTF16toUTF8(locationAStr, locationCStr);
  MOZ_ASSERT(mWindow);
  PR_snprintf(temp,
              sizeof(temp),
              "%llu (id=%u url=%s)",
              (unsigned long long)timestamp,
              (unsigned)mWindow->WindowID(),
              locationCStr.get() ? locationCStr.get() : "NULL");
#else
  PR_snprintf(temp, sizeof(temp), "%llu", (unsigned long long)timestamp);
#endif 

  mName = temp;

  
  unsigned char handle_bin[8];
  SECStatus rv;
  rv = PK11_GenerateRandom(handle_bin, sizeof(handle_bin));
  if (rv != SECSuccess) {
    MOZ_CRASH();
    return NS_ERROR_UNEXPECTED;
  }

  char hex[17];
  PR_snprintf(hex,sizeof(hex),"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
    handle_bin[0],
    handle_bin[1],
    handle_bin[2],
    handle_bin[3],
    handle_bin[4],
    handle_bin[5],
    handle_bin[6],
    handle_bin[7]);

  mHandle = hex;

  STAMP_TIMECARD(mTimeCard, "Initializing PC Ctx");
  res = PeerConnectionCtx::InitializeGlobal(mThread, mSTSThread);
  NS_ENSURE_SUCCESS(res, res);

  PeerConnectionCtx *pcctx = PeerConnectionCtx::GetInstance();
  MOZ_ASSERT(pcctx);
  STAMP_TIMECARD(mTimeCard, "Done Initializing PC Ctx");

  mInternal->mCall = pcctx->createCall();
  if (!mInternal->mCall.get()) {
    CSFLogError(logTag, "%s: Couldn't Create Call Object", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  IceConfiguration converted;
  if (aRTCConfiguration) {
    res = ConvertRTCConfiguration(*aRTCConfiguration, &converted);
    if (NS_FAILED(res)) {
      CSFLogError(logTag, "%s: Invalid RTCConfiguration", __FUNCTION__);
      return res;
    }
    aConfiguration = &converted;
  }

  mMedia = new PeerConnectionMedia(this);

  
  mMedia->SignalIceGatheringStateChange.connect(
      this,
      &PeerConnectionImpl::IceGatheringStateChange);
  mMedia->SignalIceConnectionStateChange.connect(
      this,
      &PeerConnectionImpl::IceConnectionStateChange);

  
  res = mMedia->Init(aConfiguration->getStunServers(),
                     aConfiguration->getTurnServers());
  if (NS_FAILED(res)) {
    CSFLogError(logTag, "%s: Couldn't initialize media object", __FUNCTION__);
    return res;
  }

  
  mInternal->mCall->setPeerConnection(mHandle);
  PeerConnectionCtx::GetInstance()->mPeerConnections[mHandle] = this;

  STAMP_TIMECARD(mTimeCard, "Generating DTLS Identity");
  
  mIdentity = DtlsIdentity::Generate();
  STAMP_TIMECARD(mTimeCard, "Done Generating DTLS Identity");

  if (!mIdentity) {
    CSFLogError(logTag, "%s: Generate returned NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  mFingerprint = mIdentity->GetFormattedFingerprint();
  if (mFingerprint.empty()) {
    CSFLogError(logTag, "%s: unable to get fingerprint", __FUNCTION__);
    return res;
  }

  return NS_OK;
}

RefPtr<DtlsIdentity> const
PeerConnectionImpl::GetIdentity() const
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  return mIdentity;
}

std::string
PeerConnectionImpl::GetFingerprint() const
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  return mFingerprint;
}

NS_IMETHODIMP
PeerConnectionImpl::FingerprintSplitHelper(std::string& fingerprint,
    size_t& spaceIdx) const
{
  fingerprint = GetFingerprint();
  spaceIdx = fingerprint.find_first_of(' ');
  if (spaceIdx == std::string::npos) {
    CSFLogError(logTag, "%s: fingerprint is messed up: %s",
        __FUNCTION__, fingerprint.c_str());
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

std::string
PeerConnectionImpl::GetFingerprintAlgorithm() const
{
  std::string fp;
  size_t spc;
  if (NS_SUCCEEDED(FingerprintSplitHelper(fp, spc))) {
    return fp.substr(0, spc);
  }
  return "";
}

std::string
PeerConnectionImpl::GetFingerprintHexValue() const
{
  std::string fp;
  size_t spc;
  if (NS_SUCCEEDED(FingerprintSplitHelper(fp, spc))) {
    return fp.substr(spc + 1);
  }
  return "";
}


nsresult
PeerConnectionImpl::CreateFakeMediaStream(uint32_t aHint, nsIDOMMediaStream** aRetval)
{
  MOZ_ASSERT(aRetval);
  PC_AUTO_ENTER_API_CALL(false);

  bool mute = false;

  
  if (aHint & MEDIA_STREAM_MUTE) {
    mute = true;
    aHint &= ~MEDIA_STREAM_MUTE;
  }

  nsRefPtr<DOMMediaStream> stream = MakeMediaStream(mWindow, aHint);
  if (!stream) {
    return NS_ERROR_FAILURE;
  }

  if (!mute) {
    if (aHint & DOMMediaStream::HINT_CONTENTS_AUDIO) {
      new Fake_AudioGenerator(stream);
    } else {
#ifdef MOZILLA_INTERNAL_API
    new Fake_VideoGenerator(stream);
#endif
    }
  }

  *aRetval = stream.forget().get();
  return NS_OK;
}




NS_IMETHODIMP
PeerConnectionImpl::ConnectDataConnection(uint16_t aLocalport,
                                          uint16_t aRemoteport,
                                          uint16_t aNumstreams)
{
  return NS_OK; 
}




NS_IMETHODIMP
PeerConnectionImpl::EnsureDataConnection(uint16_t aNumstreams)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

#ifdef MOZILLA_INTERNAL_API
  if (mDataConnection) {
    CSFLogDebug(logTag,"%s DataConnection already connected",__FUNCTION__);
    
    
    
    return NS_OK;
  }
  mDataConnection = new DataChannelConnection(this);
  if (!mDataConnection->Init(5000, aNumstreams, true)) {
    CSFLogError(logTag,"%s DataConnection Init Failed",__FUNCTION__);
    return NS_ERROR_FAILURE;
  }
  CSFLogDebug(logTag,"%s DataChannelConnection %p attached to %s",
              __FUNCTION__, (void*) mDataConnection.get(), mHandle.c_str());
#endif
  return NS_OK;
}

nsresult
PeerConnectionImpl::InitializeDataChannel(int track_id,
                                          uint16_t aLocalport,
                                          uint16_t aRemoteport,
                                          uint16_t aNumstreams)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

#ifdef MOZILLA_INTERNAL_API
  nsresult rv = EnsureDataConnection(aNumstreams);
  if (NS_SUCCEEDED(rv)) {
    
    nsRefPtr<TransportFlow> flow = mMedia->GetTransportFlow(track_id, false).get();
    CSFLogDebug(logTag, "Transportflow[%d] = %p", track_id, flow.get());
    if (flow) {
      if (mDataConnection->ConnectViaTransportFlow(flow, aLocalport, aRemoteport)) {
        return NS_OK;
      }
    }
    
    mDataConnection->Destroy();
  }
  mDataConnection = nullptr;
#endif
  return NS_ERROR_FAILURE;
}

already_AddRefed<nsDOMDataChannel>
PeerConnectionImpl::CreateDataChannel(const nsAString& aLabel,
                                      const nsAString& aProtocol,
                                      uint16_t aType,
                                      bool outOfOrderAllowed,
                                      uint16_t aMaxTime,
                                      uint16_t aMaxNum,
                                      bool aExternalNegotiated,
                                      uint16_t aStream,
                                      ErrorResult &rv)
{
#ifdef MOZILLA_INTERNAL_API
  nsRefPtr<nsDOMDataChannel> result;
  rv = CreateDataChannel(aLabel, aProtocol, aType, outOfOrderAllowed,
                         aMaxTime, aMaxNum, aExternalNegotiated,
                         aStream, getter_AddRefs(result));
  return result.forget();
#else
  return nullptr;
#endif
}

NS_IMETHODIMP
PeerConnectionImpl::CreateDataChannel(const nsAString& aLabel,
                                      const nsAString& aProtocol,
                                      uint16_t aType,
                                      bool outOfOrderAllowed,
                                      uint16_t aMaxTime,
                                      uint16_t aMaxNum,
                                      bool aExternalNegotiated,
                                      uint16_t aStream,
                                      nsDOMDataChannel** aRetval)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aRetval);

#ifdef MOZILLA_INTERNAL_API
  nsRefPtr<DataChannel> dataChannel;
  DataChannelConnection::Type theType =
    static_cast<DataChannelConnection::Type>(aType);

  nsresult rv = EnsureDataConnection(WEBRTC_DATACHANNEL_STREAMS_DEFAULT);
  if (NS_FAILED(rv)) {
    return rv;
  }
  dataChannel = mDataConnection->Open(
    NS_ConvertUTF16toUTF8(aLabel), NS_ConvertUTF16toUTF8(aProtocol), theType,
    !outOfOrderAllowed,
    aType == DataChannelConnection::PARTIAL_RELIABLE_REXMIT ? aMaxNum :
    (aType == DataChannelConnection::PARTIAL_RELIABLE_TIMED ? aMaxTime : 0),
    nullptr, nullptr, aExternalNegotiated, aStream
  );
  NS_ENSURE_TRUE(dataChannel,NS_ERROR_FAILURE);

  CSFLogDebug(logTag, "%s: making DOMDataChannel", __FUNCTION__);

  if (!mHaveDataStream) {
    
    mInternal->mCall->addStream(0, 2, DATA, 0);
    mHaveDataStream = true;
  }
  nsIDOMDataChannel *retval;
  rv = NS_NewDOMDataChannel(dataChannel.forget(), mWindow, &retval);
  if (NS_FAILED(rv)) {
    return rv;
  }
  *aRetval = static_cast<nsDOMDataChannel*>(retval);
#endif
  return NS_OK;
}















static already_AddRefed<PeerConnectionObserver>
do_QueryObjectReferent(nsIWeakReference* aRawPtr) {
  nsCOMPtr<nsISupportsWeakReference> tmp = do_QueryReferent(aRawPtr);
  if (!tmp) {
    return nullptr;
  }
  nsRefPtr<nsSupportsWeakReference> tmp2 = do_QueryObject(tmp);
  nsRefPtr<PeerConnectionObserver> tmp3 = static_cast<PeerConnectionObserver*>(&*tmp2);
  return tmp3.forget();
}

void
PeerConnectionImpl::NotifyConnection()
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

  CSFLogDebug(logTag, "%s", __FUNCTION__);

#ifdef MOZILLA_INTERNAL_API
  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return;
  }
  WrappableJSErrorResult rv;
  RUN_ON_THREAD(mThread,
                WrapRunnable(pco,
                             &PeerConnectionObserver::NotifyConnection,
                             rv, static_cast<JSCompartment*>(nullptr)),
                NS_DISPATCH_NORMAL);
#endif
}

void
PeerConnectionImpl::NotifyClosedConnection()
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

  CSFLogDebug(logTag, "%s", __FUNCTION__);

#ifdef MOZILLA_INTERNAL_API
  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return;
  }
  WrappableJSErrorResult rv;
  RUN_ON_THREAD(mThread,
    WrapRunnable(pco, &PeerConnectionObserver::NotifyClosedConnection,
                 rv, static_cast<JSCompartment*>(nullptr)),
    NS_DISPATCH_NORMAL);
#endif
}


#ifdef MOZILLA_INTERNAL_API

static void NotifyDataChannel_m(nsRefPtr<nsIDOMDataChannel> aChannel,
                                nsRefPtr<PeerConnectionObserver> aObserver)
{
  MOZ_ASSERT(NS_IsMainThread());
  JSErrorResult rv;
  nsRefPtr<nsDOMDataChannel> channel = static_cast<nsDOMDataChannel*>(&*aChannel);
  aObserver->NotifyDataChannel(*channel, rv);
  NS_DataChannelAppReady(aChannel);
}
#endif

void
PeerConnectionImpl::NotifyDataChannel(already_AddRefed<DataChannel> aChannel)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aChannel.get());

  CSFLogDebug(logTag, "%s: channel: %p", __FUNCTION__, aChannel.get());

#ifdef MOZILLA_INTERNAL_API
  nsCOMPtr<nsIDOMDataChannel> domchannel;
  nsresult rv = NS_NewDOMDataChannel(aChannel, mWindow,
                                     getter_AddRefs(domchannel));
  NS_ENSURE_SUCCESS_VOID(rv);

  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return;
  }

  RUN_ON_THREAD(mThread,
                WrapRunnableNM(NotifyDataChannel_m,
                               domchannel.get(),
                               pco),
                NS_DISPATCH_NORMAL);
#endif
}

NS_IMETHODIMP
PeerConnectionImpl::CreateOffer(const MediaConstraintsInternal& aConstraints)
{
  return CreateOffer(MediaConstraintsExternal (aConstraints));
}


NS_IMETHODIMP
PeerConnectionImpl::CreateOffer(const MediaConstraintsExternal& aConstraints)
{
  PC_AUTO_ENTER_API_CALL(true);

  Timecard *tc = mTimeCard;
  mTimeCard = nullptr;
  STAMP_TIMECARD(tc, "Create Offer");

  cc_media_constraints_t* cc_constraints = aConstraints.build();
  NS_ENSURE_TRUE(cc_constraints, NS_ERROR_UNEXPECTED);
  mInternal->mCall->createOffer(cc_constraints, tc);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::CreateAnswer(const MediaConstraintsInternal& aConstraints)
{
  return CreateAnswer(MediaConstraintsExternal (aConstraints));
}

NS_IMETHODIMP
PeerConnectionImpl::CreateAnswer(const MediaConstraintsExternal& aConstraints)
{
  PC_AUTO_ENTER_API_CALL(true);

  Timecard *tc = mTimeCard;
  mTimeCard = nullptr;
  STAMP_TIMECARD(tc, "Create Answer");

  cc_media_constraints_t* cc_constraints = aConstraints.build();
  NS_ENSURE_TRUE(cc_constraints, NS_ERROR_UNEXPECTED);
  mInternal->mCall->createAnswer(cc_constraints, tc);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::SetLocalDescription(int32_t aAction, const char* aSDP)
{
  PC_AUTO_ENTER_API_CALL(true);

  if (!aSDP) {
    CSFLogError(logTag, "%s - aSDP is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  Timecard *tc = mTimeCard;
  mTimeCard = nullptr;
  STAMP_TIMECARD(tc, "Set Local Description");

  mLocalRequestedSDP = aSDP;
  mInternal->mCall->setLocalDescription((cc_jsep_action_t)aAction,
                                        mLocalRequestedSDP, tc);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::SetRemoteDescription(int32_t action, const char* aSDP)
{
  PC_AUTO_ENTER_API_CALL(true);

  if (!aSDP) {
    CSFLogError(logTag, "%s - aSDP is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  Timecard *tc = mTimeCard;
  mTimeCard = nullptr;
  STAMP_TIMECARD(tc, "Set Remote Description");

  mRemoteRequestedSDP = aSDP;
  mInternal->mCall->setRemoteDescription((cc_jsep_action_t)action,
                                         mRemoteRequestedSDP, tc);
  return NS_OK;
}



#ifdef MOZILLA_INTERNAL_API
nsresult
PeerConnectionImpl::GetTimeSinceEpoch(DOMHighResTimeStamp *result) {
  MOZ_ASSERT(NS_IsMainThread());
  nsPerformance *perf = mWindow->GetPerformance();
  NS_ENSURE_TRUE(perf && perf->Timing(), NS_ERROR_UNEXPECTED);
  *result = perf->Now() + perf->Timing()->NavigationStart();
  return NS_OK;
}

class RTCStatsReportInternalConstruct : public RTCStatsReportInternal {
public:
  RTCStatsReportInternalConstruct(const nsString &pcid, DOMHighResTimeStamp now) {
    mPcid = pcid;
    mInboundRTPStreamStats.Construct();
    mOutboundRTPStreamStats.Construct();
    mMediaStreamTrackStats.Construct();
    mMediaStreamStats.Construct();
    mTransportStats.Construct();
    mIceComponentStats.Construct();
    mIceCandidatePairStats.Construct();
    mIceCandidateStats.Construct();
    mCodecStats.Construct();
  }
};



static void
PushBackSelect(std::vector<RefPtr<MediaPipeline>>& aDst,
               const std::map<TrackID, RefPtr<mozilla::MediaPipeline>> & aSrc,
               TrackID aKey = 0) {
  auto begin = aKey ? aSrc.find(aKey) : aSrc.begin(), it = begin;
  for (auto end = (aKey && begin != aSrc.end())? ++begin : aSrc.end();
       it != end; ++it) {
    aDst.push_back(it->second);
  }
}
#endif

NS_IMETHODIMP
PeerConnectionImpl::GetStats(MediaStreamTrack *aSelector, bool internalStats) {
  PC_AUTO_ENTER_API_CALL(true);

#ifdef MOZILLA_INTERNAL_API
  if (!mMedia) {
    
    return NS_ERROR_UNEXPECTED;
  }

  

  std::vector<RefPtr<MediaPipeline>> pipelines;
  TrackID trackId = aSelector ? aSelector->GetTrackID() : 0;

  for (int i = 0, len = mMedia->LocalStreamsLength(); i < len; i++) {
    PushBackSelect(pipelines, mMedia->GetLocalStream(i)->GetPipelines(), trackId);
  }
  for (int i = 0, len = mMedia->RemoteStreamsLength(); i < len; i++) {
    PushBackSelect(pipelines, mMedia->GetRemoteStream(i)->GetPipelines(), trackId);
  }

  
  
  std::vector<RefPtr<NrIceMediaStream> > streams;
  RefPtr<NrIceCtx> iceCtx(mMedia->ice_ctx());
  for (auto p = pipelines.begin(); p != pipelines.end(); ++p) {
    size_t level = p->get()->level();
    
    
    RefPtr<NrIceMediaStream> temp(mMedia->ice_media_stream(level-1));
    if (temp.get()) {
      streams.push_back(temp);
    } else {
       CSFLogError(logTag, "Failed to get NrIceMediaStream for level %u "
                           "in %s:  %s",
                           uint32_t(level), __FUNCTION__, mHandle.c_str());
       MOZ_CRASH();
    }
  }

  DOMHighResTimeStamp now;
  nsresult rv = GetTimeSinceEpoch(&now);
  NS_ENSURE_SUCCESS(rv, rv);

  RUN_ON_THREAD(mSTSThread,
                WrapRunnableNM(&PeerConnectionImpl::GetStats_s,
                               mHandle,
                               mName,
                               mThread,
                               internalStats,
                               pipelines,
                               iceCtx,
                               streams,
                               now),
                NS_DISPATCH_NORMAL);
#endif
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::GetLogging(const nsAString& aPattern) {
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

#ifdef MOZILLA_INTERNAL_API
  std::string pattern(NS_ConvertUTF16toUTF8(aPattern).get());
  RUN_ON_THREAD(mSTSThread,
                WrapRunnableNM(&PeerConnectionImpl::GetLogging_s,
                               mHandle,
                               mThread,
                               pattern),
                NS_DISPATCH_NORMAL);

#endif
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::AddIceCandidate(const char* aCandidate, const char* aMid, unsigned short aLevel) {
  PC_AUTO_ENTER_API_CALL(true);

  Timecard *tc = mTimeCard;
  mTimeCard = nullptr;
  STAMP_TIMECARD(tc, "Add Ice Candidate");

  mInternal->mCall->addICECandidate(aCandidate, aMid, aLevel, tc);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::CloseStreams() {
  PC_AUTO_ENTER_API_CALL(false);

  if (mReadyState != PCImplReadyState::Closed)  {
    ChangeReadyState(PCImplReadyState::Closing);
  }

  CSFLogInfo(logTag, "%s: Ending associated call", __FUNCTION__);

  mInternal->mCall->endCall();
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::AddStream(DOMMediaStream &aMediaStream,
                              const MediaConstraintsInternal& aConstraints)
{
  return AddStream(aMediaStream, MediaConstraintsExternal(aConstraints));
}

NS_IMETHODIMP
PeerConnectionImpl::AddStream(DOMMediaStream& aMediaStream,
                              const MediaConstraintsExternal& aConstraints) {
  PC_AUTO_ENTER_API_CALL(true);

  uint32_t hints = aMediaStream.GetHintContents();

  
  
  
  if ((hints & DOMMediaStream::HINT_CONTENTS_AUDIO) &&
      mNumAudioStreams > 0) {
    CSFLogError(logTag, "%s: Only one local audio stream is supported for now",
                __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  
  
  
  if ((hints & DOMMediaStream::HINT_CONTENTS_VIDEO) &&
      mNumVideoStreams > 0) {
    CSFLogError(logTag, "%s: Only one local video stream is supported for now",
                __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  uint32_t stream_id;
  nsresult res = mMedia->AddStream(&aMediaStream, &stream_id);
  if (NS_FAILED(res))
    return res;

  
  if (hints & DOMMediaStream::HINT_CONTENTS_AUDIO) {
    cc_media_constraints_t* cc_constraints = aConstraints.build();
    NS_ENSURE_TRUE(cc_constraints, NS_ERROR_UNEXPECTED);
    mInternal->mCall->addStream(stream_id, 0, AUDIO, cc_constraints);
    mNumAudioStreams++;
  }

  if (hints & DOMMediaStream::HINT_CONTENTS_VIDEO) {
    cc_media_constraints_t* cc_constraints = aConstraints.build();
    NS_ENSURE_TRUE(cc_constraints, NS_ERROR_UNEXPECTED);
    mInternal->mCall->addStream(stream_id, 1, VIDEO, cc_constraints);
    mNumVideoStreams++;
  }

  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::RemoveStream(DOMMediaStream& aMediaStream) {
  PC_AUTO_ENTER_API_CALL(true);

  uint32_t stream_id;
  nsresult res = mMedia->RemoveStream(&aMediaStream, &stream_id);

  if (NS_FAILED(res))
    return res;

  uint32_t hints = aMediaStream.GetHintContents();

  if (hints & DOMMediaStream::HINT_CONTENTS_AUDIO) {
    mInternal->mCall->removeStream(stream_id, 0, AUDIO);
    MOZ_ASSERT(mNumAudioStreams > 0);
    mNumAudioStreams--;
  }

  if (hints & DOMMediaStream::HINT_CONTENTS_VIDEO) {
    mInternal->mCall->removeStream(stream_id, 1, VIDEO);
    MOZ_ASSERT(mNumVideoStreams > 0);
    mNumVideoStreams--;
  }

  return NS_OK;
}



















NS_IMETHODIMP
PeerConnectionImpl::GetFingerprint(char** fingerprint)
{
  MOZ_ASSERT(fingerprint);

  if (!mIdentity) {
    return NS_ERROR_FAILURE;
  }

  char* tmp = new char[mFingerprint.size() + 1];
  std::copy(mFingerprint.begin(), mFingerprint.end(), tmp);
  tmp[mFingerprint.size()] = '\0';

  *fingerprint = tmp;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::GetLocalDescription(char** aSDP)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aSDP);

  char* tmp = new char[mLocalSDP.size() + 1];
  std::copy(mLocalSDP.begin(), mLocalSDP.end(), tmp);
  tmp[mLocalSDP.size()] = '\0';

  *aSDP = tmp;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::GetRemoteDescription(char** aSDP)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aSDP);

  char* tmp = new char[mRemoteSDP.size() + 1];
  std::copy(mRemoteSDP.begin(), mRemoteSDP.end(), tmp);
  tmp[mRemoteSDP.size()] = '\0';

  *aSDP = tmp;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::ReadyState(PCImplReadyState* aState)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aState);

  *aState = mReadyState;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::SignalingState(PCImplSignalingState* aState)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aState);

  *aState = mSignalingState;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::SipccState(PCImplSipccState* aState)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aState);

  PeerConnectionCtx* pcctx = PeerConnectionCtx::GetInstance();
  
  
  if (pcctx) {
    *aState = pcctx->sipcc_state();
  } else {
    *aState = PCImplSipccState::Idle;
  }
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::IceConnectionState(PCImplIceConnectionState* aState)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aState);

  *aState = mIceConnectionState;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::IceGatheringState(PCImplIceGatheringState* aState)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aState);

  *aState = mIceGatheringState;
  return NS_OK;
}

nsresult
PeerConnectionImpl::CheckApiState(bool assert_ice_ready) const
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(mTrickle || !assert_ice_ready ||
             (mIceGatheringState == PCImplIceGatheringState::Complete));

  if (mReadyState == PCImplReadyState::Closed)
    return NS_ERROR_FAILURE;
  if (!mMedia)
    return NS_ERROR_FAILURE;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::Close()
{
  CSFLogDebug(logTag, "%s: for %s", __FUNCTION__, mHandle.c_str());
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

  return CloseInt();
}


nsresult
PeerConnectionImpl::CloseInt()
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

  if (mInternal->mCall) {
    CSFLogInfo(logTag, "%s: Closing PeerConnectionImpl %s; "
               "ending call", __FUNCTION__, mHandle.c_str());
    mInternal->mCall->endCall();
  }
#ifdef MOZILLA_INTERNAL_API
  if (mDataConnection) {
    CSFLogInfo(logTag, "%s: Destroying DataChannelConnection %p for %s",
               __FUNCTION__, (void *) mDataConnection.get(), mHandle.c_str());
    mDataConnection->Destroy();
    mDataConnection = nullptr; 
  }
#endif

  ShutdownMedia();

  

  return NS_OK;
}

void
PeerConnectionImpl::ShutdownMedia()
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();

  if (!mMedia)
    return;

#ifdef MOZILLA_INTERNAL_API
  
  if (!mStartTime.IsNull()){
    TimeDuration timeDelta = TimeStamp::Now() - mStartTime;
    Telemetry::Accumulate(Telemetry::WEBRTC_CALL_DURATION, timeDelta.ToSeconds());
  }
#endif

  
  
  mMedia.forget().get()->SelfDestruct();
}

#ifdef MOZILLA_INTERNAL_API



void
PeerConnectionImpl::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void
PeerConnectionImpl::destructorSafeDestroyNSSReference()
{
  MOZ_ASSERT(NS_IsMainThread());
  CSFLogDebug(logTag, "%s: NSS shutting down; freeing our DtlsIdentity.", __FUNCTION__);
  mIdentity = nullptr;
}
#endif

void
PeerConnectionImpl::onCallEvent(const OnCallEventArgs& args)
{
  const ccapi_call_event_e &aCallEvent = args.mCallEvent;
  const CSF::CC_CallInfoPtr &aInfo = args.mInfo;

  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aInfo.get());

  cc_call_state_t event = aInfo->getCallState();
  std::string statestr = aInfo->callStateToString(event);
  Timecard *timecard = aInfo->takeTimecard();

  if (timecard) {
    mTimeCard = timecard;
    STAMP_TIMECARD(mTimeCard, "Operation Completed");
  }

  if (CCAPI_CALL_EV_CREATED != aCallEvent && CCAPI_CALL_EV_STATE != aCallEvent) {
    CSFLogDebug(logTag, "%s: **** CALL HANDLE IS: %s, **** CALL STATE IS: %s",
      __FUNCTION__, mHandle.c_str(), statestr.c_str());
    return;
  }

  switch (event) {
    case SETLOCALDESCSUCCESS:
    case UPDATELOCALDESC:
      mLocalSDP = aInfo->getSDP();
      break;

    case SETREMOTEDESCSUCCESS:
    case ADDICECANDIDATE:
      mRemoteSDP = aInfo->getSDP();
      break;

    case CONNECTED:
      CSFLogDebug(logTag, "Setting PeerConnnection state to kActive");
      ChangeReadyState(PCImplReadyState::Active);
      break;
    default:
      break;
  }

  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return;
  }

  PeerConnectionObserverDispatch* runnable =
      new PeerConnectionObserverDispatch(aInfo, this, pco);

  if (mThread) {
    mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    return;
  }
  runnable->Run();
  delete runnable;
}

void
PeerConnectionImpl::ChangeReadyState(PCImplReadyState aReadyState)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  mReadyState = aReadyState;

  
  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return;
  }
  WrappableJSErrorResult rv;
  RUN_ON_THREAD(mThread,
                WrapRunnable(pco,
                             &PeerConnectionObserver::OnStateChange,
                             PCObserverStateType::ReadyState,
                             rv, static_cast<JSCompartment*>(nullptr)),
                NS_DISPATCH_NORMAL);
}

void
PeerConnectionImpl::SetSignalingState_m(PCImplSignalingState aSignalingState)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  if (mSignalingState == aSignalingState) {
    return;
  }

  mSignalingState = aSignalingState;
  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return;
  }
  JSErrorResult rv;
  pco->OnStateChange(PCObserverStateType::SignalingState, rv);
  MOZ_ASSERT(!rv.Failed());
}

PeerConnectionWrapper::PeerConnectionWrapper(const std::string& handle)
    : impl_(nullptr) {
  if (PeerConnectionCtx::GetInstance()->mPeerConnections.find(handle) ==
    PeerConnectionCtx::GetInstance()->mPeerConnections.end()) {
    return;
  }

  PeerConnectionImpl *impl = PeerConnectionCtx::GetInstance()->mPeerConnections[handle];

  if (!impl->media())
    return;

  impl_ = impl;
}

const std::string&
PeerConnectionImpl::GetHandle()
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  return mHandle;
}

const std::string&
PeerConnectionImpl::GetName()
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  return mName;
}

static mozilla::dom::PCImplIceConnectionState
toDomIceConnectionState(NrIceCtx::ConnectionState state) {
  switch (state) {
    case NrIceCtx::ICE_CTX_INIT:
      return PCImplIceConnectionState::New;
    case NrIceCtx::ICE_CTX_CHECKING:
      return PCImplIceConnectionState::Checking;
    case NrIceCtx::ICE_CTX_OPEN:
      return PCImplIceConnectionState::Connected;
    case NrIceCtx::ICE_CTX_FAILED:
      return PCImplIceConnectionState::Failed;
  }
  MOZ_CRASH();
}

static mozilla::dom::PCImplIceGatheringState
toDomIceGatheringState(NrIceCtx::GatheringState state) {
  switch (state) {
    case NrIceCtx::ICE_CTX_GATHER_INIT:
      return PCImplIceGatheringState::New;
    case NrIceCtx::ICE_CTX_GATHER_STARTED:
      return PCImplIceGatheringState::Gathering;
    case NrIceCtx::ICE_CTX_GATHER_COMPLETE:
      return PCImplIceGatheringState::Complete;
  }
  MOZ_CRASH();
}



void PeerConnectionImpl::IceConnectionStateChange(
    NrIceCtx* ctx,
    NrIceCtx::ConnectionState state) {
  (void)ctx;
  
  nsRefPtr<PeerConnectionImpl> pc(this);
  RUN_ON_THREAD(mThread,
                WrapRunnable(pc,
                             &PeerConnectionImpl::IceConnectionStateChange_m,
                             toDomIceConnectionState(state)),
                NS_DISPATCH_NORMAL);
}

void
PeerConnectionImpl::IceGatheringStateChange(
    NrIceCtx* ctx,
    NrIceCtx::GatheringState state)
{
  (void)ctx;
  
  nsRefPtr<PeerConnectionImpl> pc(this);
  RUN_ON_THREAD(mThread,
                WrapRunnable(pc,
                             &PeerConnectionImpl::IceGatheringStateChange_m,
                             toDomIceGatheringState(state)),
                NS_DISPATCH_NORMAL);
}

nsresult
PeerConnectionImpl::IceConnectionStateChange_m(PCImplIceConnectionState aState)
{
  PC_AUTO_ENTER_API_CALL(false);

  CSFLogDebug(logTag, "%s", __FUNCTION__);

  mIceConnectionState = aState;

  
  
  switch (mIceConnectionState) {
    case PCImplIceConnectionState::New:
      STAMP_TIMECARD(mTimeCard, "Ice state: new");
      break;
    case PCImplIceConnectionState::Checking:
      STAMP_TIMECARD(mTimeCard, "Ice state: checking");
      break;
    case PCImplIceConnectionState::Connected:
      STAMP_TIMECARD(mTimeCard, "Ice state: connected");
      break;
    case PCImplIceConnectionState::Completed:
      STAMP_TIMECARD(mTimeCard, "Ice state: completed");
      break;
    case PCImplIceConnectionState::Failed:
      STAMP_TIMECARD(mTimeCard, "Ice state: failed");
      break;
    case PCImplIceConnectionState::Disconnected:
      STAMP_TIMECARD(mTimeCard, "Ice state: disconnected");
      break;
    case PCImplIceConnectionState::Closed:
      STAMP_TIMECARD(mTimeCard, "Ice state: closed");
      break;
  }

  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return NS_OK;
  }
  WrappableJSErrorResult rv;
  RUN_ON_THREAD(mThread,
                WrapRunnable(pco,
                             &PeerConnectionObserver::OnStateChange,
                             PCObserverStateType::IceConnectionState,
                             rv, static_cast<JSCompartment*>(nullptr)),
                NS_DISPATCH_NORMAL);
  return NS_OK;
}

nsresult
PeerConnectionImpl::IceGatheringStateChange_m(PCImplIceGatheringState aState)
{
  PC_AUTO_ENTER_API_CALL(false);

  CSFLogDebug(logTag, "%s", __FUNCTION__);

  mIceGatheringState = aState;

  
  
  switch (mIceGatheringState) {
    case PCImplIceGatheringState::New:
      STAMP_TIMECARD(mTimeCard, "Ice gathering state: new");
      break;
    case PCImplIceGatheringState::Gathering:
      STAMP_TIMECARD(mTimeCard, "Ice gathering state: gathering");
      break;
    case PCImplIceGatheringState::Complete:
      STAMP_TIMECARD(mTimeCard, "Ice state: complete");
      break;
  }

  nsRefPtr<PeerConnectionObserver> pco = do_QueryObjectReferent(mPCObserver);
  if (!pco) {
    return NS_OK;
  }
  WrappableJSErrorResult rv;
  RUN_ON_THREAD(mThread,
                WrapRunnable(pco,
                             &PeerConnectionObserver::OnStateChange,
                             PCObserverStateType::IceGatheringState,
                             rv, static_cast<JSCompartment*>(nullptr)),
                NS_DISPATCH_NORMAL);
  return NS_OK;
}

#ifdef MOZILLA_INTERNAL_API
nsresult
PeerConnectionImpl::GetStatsImpl_s(
    bool internalStats,
    const std::vector<RefPtr<MediaPipeline>>& pipelines,
    const RefPtr<NrIceCtx>& iceCtx,
    const std::vector<RefPtr<NrIceMediaStream>>& streams,
    DOMHighResTimeStamp now,
    RTCStatsReportInternal* report) {

  ASSERT_ON_THREAD(iceCtx->thread());

  

  for (auto it = pipelines.begin(); it != pipelines.end(); ++it) {
    const MediaPipeline& mp = **it;
    nsString idstr = (mp.Conduit()->type() == MediaSessionConduit::AUDIO) ?
        NS_LITERAL_STRING("audio_") : NS_LITERAL_STRING("video_");
    idstr.AppendInt(mp.trackid());

    switch (mp.direction()) {
      case MediaPipeline::TRANSMIT: {
        nsString localId = NS_LITERAL_STRING("outbound_rtp_") + idstr;
        nsString remoteId;
        nsString ssrc;
        unsigned int ssrcval;
        if (mp.Conduit()->GetLocalSSRC(&ssrcval)) {
          ssrc.AppendInt(ssrcval);
        }
        {
          
          
          
          DOMHighResTimeStamp timestamp;
          uint32_t jitterMs;
          uint32_t packetsReceived;
          uint64_t bytesReceived;
          if (mp.Conduit()->GetRTCPReceiverReport(&timestamp, &jitterMs,
                                                  &packetsReceived,
                                                  &bytesReceived)) {
            remoteId = NS_LITERAL_STRING("outbound_rtcp_") + idstr;
            RTCInboundRTPStreamStats s;
            s.mTimestamp.Construct(timestamp);
            s.mId.Construct(remoteId);
            s.mType.Construct(RTCStatsType::Inboundrtp);
            if (ssrc.Length()) {
              s.mSsrc.Construct(ssrc);
            }
            s.mJitter.Construct(double(jitterMs)/1000);
            s.mRemoteId.Construct(localId);
            s.mIsRemote = true;
            s.mPacketsReceived.Construct(packetsReceived);
            s.mBytesReceived.Construct(bytesReceived);
            report->mInboundRTPStreamStats.Value().AppendElement(s);
          }
        }
        
        {
          RTCOutboundRTPStreamStats s;
          s.mTimestamp.Construct(now);
          s.mId.Construct(localId);
          s.mType.Construct(RTCStatsType::Outboundrtp);
          if (ssrc.Length()) {
            s.mSsrc.Construct(ssrc);
          }
          s.mRemoteId.Construct(remoteId);
          s.mIsRemote = false;
          s.mPacketsSent.Construct(mp.rtp_packets_sent());
          s.mBytesSent.Construct(mp.rtp_bytes_sent());
          report->mOutboundRTPStreamStats.Value().AppendElement(s);
        }
        break;
      }
      case MediaPipeline::RECEIVE: {
        nsString localId = NS_LITERAL_STRING("inbound_rtp_") + idstr;
        nsString remoteId;
        nsString ssrc;
        unsigned int ssrcval;
        if (mp.Conduit()->GetRemoteSSRC(&ssrcval)) {
          ssrc.AppendInt(ssrcval);
        }
        {
          
          DOMHighResTimeStamp timestamp;
          uint32_t packetsSent;
          uint64_t bytesSent;
          if (mp.Conduit()->GetRTCPSenderReport(&timestamp,
                                                &packetsSent, &bytesSent)) {
            remoteId = NS_LITERAL_STRING("inbound_rtcp_") + idstr;
            RTCOutboundRTPStreamStats s;
            s.mTimestamp.Construct(timestamp);
            s.mId.Construct(remoteId);
            s.mType.Construct(RTCStatsType::Outboundrtp);
            if (ssrc.Length()) {
              s.mSsrc.Construct(ssrc);
            }
            s.mRemoteId.Construct(localId);
            s.mIsRemote = true;
            s.mPacketsSent.Construct(packetsSent);
            s.mBytesSent.Construct(bytesSent);
            report->mOutboundRTPStreamStats.Value().AppendElement(s);
          }
        }
        
        RTCInboundRTPStreamStats s;
        s.mTimestamp.Construct(now);
        s.mId.Construct(localId);
        s.mType.Construct(RTCStatsType::Inboundrtp);
        if (ssrc.Length()) {
          s.mSsrc.Construct(ssrc);
        }
        unsigned int jitterMs;
        if (mp.Conduit()->GetRTPJitter(&jitterMs)) {
          s.mJitter.Construct(double(jitterMs)/1000);
        }
        if (remoteId.Length()) {
          s.mRemoteId.Construct(remoteId);
        }
        s.mIsRemote = false;
        s.mPacketsReceived.Construct(mp.rtp_packets_received());
        s.mBytesReceived.Construct(mp.rtp_bytes_received());
        report->mInboundRTPStreamStats.Value().AppendElement(s);
        break;
      }
    }
  }

  
  for (auto s = streams.begin(); s != streams.end(); ++s) {
    FillStatsReport_s(**s, internalStats, now, report);
  }

  return NS_OK;
}

static void ToRTCIceCandidateStats(
    const std::vector<NrIceCandidate>& candidates,
    RTCStatsType candidateType,
    const nsString& componentId,
    DOMHighResTimeStamp now,
    RTCStatsReportInternal* report) {

  MOZ_ASSERT(report);
  for (auto c = candidates.begin(); c != candidates.end(); ++c) {
    RTCIceCandidateStats cand;
    cand.mType.Construct(candidateType);
    NS_ConvertASCIItoUTF16 codeword(c->codeword.c_str());
    cand.mComponentId.Construct(componentId);
    cand.mId.Construct(codeword);
    cand.mTimestamp.Construct(now);
    cand.mCandidateType.Construct(
        RTCStatsIceCandidateType(c->type));
    cand.mIpAddress.Construct(
        NS_ConvertASCIItoUTF16(c->cand_addr.host.c_str()));
    cand.mPortNumber.Construct(c->cand_addr.port);
    report->mIceCandidateStats.Value().AppendElement(cand);
  }
}

void PeerConnectionImpl::FillStatsReport_s(
    NrIceMediaStream& mediaStream,
    bool internalStats,
    DOMHighResTimeStamp now,
    RTCStatsReportInternal* report) {

  NS_ConvertASCIItoUTF16 componentId(mediaStream.name().c_str());
  if (internalStats) {
    std::vector<NrIceCandidatePair> candPairs;
    nsresult res = mediaStream.GetCandidatePairs(&candPairs);
    if (NS_FAILED(res)) {
      CSFLogError(logTag, "%s: Error getting candidate pairs", __FUNCTION__);
      return;
    }

    for (auto p = candPairs.begin(); p != candPairs.end(); ++p) {
      NS_ConvertASCIItoUTF16 codeword(p->codeword.c_str());
      NS_ConvertASCIItoUTF16 localCodeword(p->local.codeword.c_str());
      NS_ConvertASCIItoUTF16 remoteCodeword(p->remote.codeword.c_str());
      
      

      RTCIceCandidatePairStats s;
      s.mId.Construct(codeword);
      s.mComponentId.Construct(componentId);
      s.mTimestamp.Construct(now);
      s.mType.Construct(RTCStatsType::Candidatepair);
      s.mLocalCandidateId.Construct(localCodeword);
      s.mRemoteCandidateId.Construct(remoteCodeword);
      s.mNominated.Construct(p->nominated);
      s.mMozPriority.Construct(p->priority);
      s.mSelected.Construct(p->selected);
      s.mState.Construct(RTCStatsIceCandidatePairState(p->state));
      report->mIceCandidatePairStats.Value().AppendElement(s);
    }
  }

  std::vector<NrIceCandidate> candidates;
  if (NS_SUCCEEDED(mediaStream.GetLocalCandidates(&candidates))) {
    ToRTCIceCandidateStats(candidates,
                           RTCStatsType::Localcandidate,
                           componentId,
                           now,
                           report);
  }
  candidates.clear();

  if (NS_SUCCEEDED(mediaStream.GetRemoteCandidates(&candidates))) {
    ToRTCIceCandidateStats(candidates,
                           RTCStatsType::Remotecandidate,
                           componentId,
                           now,
                           report);
  }
}

void PeerConnectionImpl::GetStats_s(
    const std::string& pcHandle, 
    const std::string& pcName, 
    nsCOMPtr<nsIThread> callbackThread,
    bool internalStats,
    const std::vector<RefPtr<MediaPipeline>>& pipelines,
    const RefPtr<NrIceCtx>& iceCtx,
    const std::vector<RefPtr<NrIceMediaStream>>& streams,
    DOMHighResTimeStamp now) {

  ASSERT_ON_THREAD(iceCtx->thread());

  
  nsAutoPtr<RTCStatsReportInternal> report(
      new RTCStatsReportInternalConstruct(
          NS_ConvertASCIItoUTF16(pcName.c_str()),
          now));

  nsresult rv = GetStatsImpl_s(internalStats,
                               pipelines,
                               iceCtx,
                               streams,
                               now,
                               report);

  RUN_ON_THREAD(callbackThread,
                WrapRunnableNM(&PeerConnectionImpl::OnStatsReport_m,
                               pcHandle,
                               rv,
                               pipelines, 
                               report),
                NS_DISPATCH_NORMAL);
}

void PeerConnectionImpl::OnStatsReport_m(
    const std::string& pcHandle,
    nsresult result,
    const std::vector<RefPtr<MediaPipeline>>& pipelines, 
    nsAutoPtr<RTCStatsReportInternal> report) {

  
  PeerConnectionWrapper pcw(pcHandle);
  if (pcw.impl()) {
    nsRefPtr<PeerConnectionObserver> pco =
        do_QueryObjectReferent(pcw.impl()->mPCObserver);
    if (pco) {
      JSErrorResult rv;
      if (NS_SUCCEEDED(result)) {
        pco->OnGetStatsSuccess(*report, rv);
      } else {
        pco->OnGetStatsError(kInternalError,
            ObString("Failed to fetch statistics"),
            rv);
      }

      if (rv.Failed()) {
        CSFLogError(logTag, "Error firing stats observer callback");
      }
    }
  }
}

void PeerConnectionImpl::GetLogging_s(const std::string& pcHandle,
                                      nsCOMPtr<nsIThread> callbackThread,
                                      const std::string& pattern) {
  RLogRingBuffer* logs = RLogRingBuffer::GetInstance();
  nsAutoPtr<std::deque<std::string>> result(new std::deque<std::string>);
  logs->Filter(pattern, 0, result);
  RUN_ON_THREAD(callbackThread,
                WrapRunnableNM(&PeerConnectionImpl::OnGetLogging_m,
                               pcHandle,
                               pattern,
                               result),
                NS_DISPATCH_NORMAL);
}

void PeerConnectionImpl::OnGetLogging_m(
    const std::string& pcHandle,
    const std::string& pattern,
    nsAutoPtr<std::deque<std::string>> logging) {

  
  PeerConnectionWrapper pcw(pcHandle);
  if (pcw.impl()) {
    nsRefPtr<PeerConnectionObserver> pco =
        do_QueryObjectReferent(pcw.impl()->mPCObserver);
    if (pco) {
      JSErrorResult rv;
      if (!logging->empty()) {
        Sequence<nsString> nsLogs;
        for (auto l = logging->begin(); l != logging->end(); ++l) {
          nsLogs.AppendElement(ObString(l->c_str()));
        }
        pco->OnGetLoggingSuccess(nsLogs, rv);
      } else {
        pco->OnGetLoggingError(kInternalError,
            ObString(("No logging matching pattern " + pattern).c_str()), rv);
      }

      if (rv.Failed()) {
        CSFLogError(logTag, "Error firing stats observer callback");
      }
    }
  }
}
#endif

void
PeerConnectionImpl::IceStreamReady(NrIceMediaStream *aStream)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
  MOZ_ASSERT(aStream);

  CSFLogDebug(logTag, "%s: %s", __FUNCTION__, aStream->name().c_str());
}

void
PeerConnectionImpl::OnSdpParseError(const char *message) {
  CSFLogError(logTag, "%s SDP Parse Error: %s", __FUNCTION__, message);
  
  mSDPParseErrorMessages.push_back(message);
}

void
PeerConnectionImpl::ClearSdpParseErrorMessages() {
  mSDPParseErrorMessages.clear();
}

const std::vector<std::string> &
PeerConnectionImpl::GetSdpParseErrors() {
  return mSDPParseErrorMessages;
}

#ifdef MOZILLA_INTERNAL_API

void
PeerConnectionImpl::startCallTelem() {
  
  mStartTime = TimeStamp::Now();

  
#ifdef MOZILLA_INTERNAL_API
  int &cnt = PeerConnectionCtx::GetInstance()->mConnectionCounter;
  Telemetry::GetHistogramById(Telemetry::WEBRTC_CALL_COUNT)->Subtract(cnt);
  cnt++;
  Telemetry::GetHistogramById(Telemetry::WEBRTC_CALL_COUNT)->Add(cnt);
#endif
}
#endif

NS_IMETHODIMP
PeerConnectionImpl::GetLocalStreams(nsTArray<nsRefPtr<DOMMediaStream > >& result)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
#ifdef MOZILLA_INTERNAL_API
  for(uint32_t i=0; i < media()->LocalStreamsLength(); i++) {
    LocalSourceStreamInfo *info = media()->GetLocalStream(i);
    NS_ENSURE_TRUE(info, NS_ERROR_UNEXPECTED);
    result.AppendElement(info->GetMediaStream());
  }
  return NS_OK;
#else
  return NS_ERROR_FAILURE;
#endif
}

NS_IMETHODIMP
PeerConnectionImpl::GetRemoteStreams(nsTArray<nsRefPtr<DOMMediaStream > >& result)
{
  PC_AUTO_ENTER_API_CALL_NO_CHECK();
#ifdef MOZILLA_INTERNAL_API
  for(uint32_t i=0; i < media()->RemoteStreamsLength(); i++) {
    RemoteSourceStreamInfo *info = media()->GetRemoteStream(i);
    NS_ENSURE_TRUE(info, NS_ERROR_UNEXPECTED);
    result.AppendElement(info->GetMediaStream());
  }
  return NS_OK;
#else
  return NS_ERROR_FAILURE;
#endif
}

}  
