



#include <string>
#include <iostream>

#include "vcm.h"
#include "CSFLog.h"
#include "CSFLogStream.h"
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

#include "nsThreadUtils.h"
#include "nsProxyRelease.h"

#include "runnable_utils.h"
#include "PeerConnectionCtx.h"
#include "PeerConnectionImpl.h"

#include "nsPIDOMWindow.h"
#include "nsDOMDataChannel.h"
#ifdef MOZILLA_INTERNAL_API
#include "MediaStreamList.h"
#include "nsIScriptGlobalObject.h"
#include "jsapi.h"
#endif

#ifndef USE_FAKE_MEDIA_STREAMS
#include "MediaSegment.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {
  class DataChannel;
}

class nsIDOMDataChannel;

static const char* logTag = "PeerConnectionImpl";
static const mozilla::TrackID TRACK_AUDIO = 0;
static const mozilla::TrackID TRACK_VIDEO = 1;
static const int DTLS_FINGERPRINT_LENGTH = 64;
static const int MEDIA_STREAM_MUTE = 0x80;

namespace sipcc {

void MediaConstraints::setBooleanConstraint(const std::string& constraint, bool enabled, bool mandatory) {

  ConstraintInfo booleanconstraint;
  booleanconstraint.mandatory = mandatory;

  if (enabled)
    booleanconstraint.value = "TRUE";
  else
    booleanconstraint.value = "FALSE";

  mConstraints[constraint] = booleanconstraint;
}

void MediaConstraints::buildArray(cc_media_constraints_t** constraintarray) {

  if (0 == mConstraints.size())
    return;

  short i = 0;
  std::string tmpStr;
  *constraintarray = (cc_media_constraints_t*) cpr_malloc(sizeof(cc_media_constraints_t));
  int tmpStrAllocLength;

  (*constraintarray)->constraints = (cc_media_constraint_t**) cpr_malloc(mConstraints.size() * sizeof(cc_media_constraint_t));

  for (constraints_map::iterator it = mConstraints.begin();
          it != mConstraints.end(); ++it) {
    (*constraintarray)->constraints[i] = (cc_media_constraint_t*) cpr_malloc(sizeof(cc_media_constraint_t));

    tmpStr = it->first;
    tmpStrAllocLength = tmpStr.size() + 1;
    (*constraintarray)->constraints[i]->name = (char*) cpr_malloc(tmpStrAllocLength);
    sstrncpy((*constraintarray)->constraints[i]->name, tmpStr.c_str(), tmpStrAllocLength);

    tmpStr = it->second.value;
    tmpStrAllocLength = tmpStr.size() + 1;
    (*constraintarray)->constraints[i]->value = (char*) cpr_malloc(tmpStrAllocLength);
    sstrncpy((*constraintarray)->constraints[i]->value, tmpStr.c_str(), tmpStrAllocLength);

    (*constraintarray)->constraints[i]->mandatory = it->second.mandatory;
    i++;
  }
  (*constraintarray)->constraint_count = i;
}

typedef enum {
  PC_OBSERVER_CALLBACK,
  PC_OBSERVER_CONNECTION,
  PC_OBSERVER_CLOSEDCONNECTION,
  PC_OBSERVER_DATACHANNEL,
  PC_OBSERVER_ICE,
  PC_OBSERVER_READYSTATE
} PeerConnectionObserverType;


class PeerConnectionObserverDispatch : public nsRunnable {

public:
  PeerConnectionObserverDispatch(CSF::CC_CallInfoPtr aInfo,
                                 nsRefPtr<PeerConnectionImpl> aPC,
                                 IPeerConnectionObserver* aObserver) :
    mType(PC_OBSERVER_CALLBACK), mInfo(aInfo), mChannel(nullptr), mPC(aPC), mObserver(aObserver) {}

  PeerConnectionObserverDispatch(PeerConnectionObserverType aType,
                                 nsRefPtr<nsIDOMDataChannel> aChannel,
                                 nsRefPtr<PeerConnectionImpl> aPC,
                                 IPeerConnectionObserver* aObserver) :
    mType(aType), mInfo(nullptr), mChannel(aChannel), mPC(aPC), mObserver(aObserver) {}

  PeerConnectionObserverDispatch(PeerConnectionObserverType aType,
                                 nsRefPtr<PeerConnectionImpl> aPC,
                                 IPeerConnectionObserver* aObserver) :
    mType(aType), mInfo(nullptr), mPC(aPC), mObserver(aObserver) {}

  ~PeerConnectionObserverDispatch(){}

  NS_IMETHOD Run()
  {
    switch (mType) {
      case PC_OBSERVER_CALLBACK:
        {
          StatusCode code;
          std::string s_sdpstr;
          MediaStreamTable *streams = NULL;

          cc_call_state_t state = mInfo->getCallState();
          std::string statestr = mInfo->callStateToString(state);

          nsDOMMediaStream* stream;
          uint32_t hint;

          switch (state) {
            case CREATEOFFER:
              s_sdpstr = mInfo->getSDP();
              mObserver->OnCreateOfferSuccess(s_sdpstr.c_str());
              break;

            case CREATEANSWER:
              s_sdpstr = mInfo->getSDP();
              mObserver->OnCreateAnswerSuccess(s_sdpstr.c_str());
              break;

            case CREATEOFFERERROR:
              code = (StatusCode)mInfo->getStatusCode();
              mObserver->OnCreateOfferError(code);
              break;

            case CREATEANSWERERROR:
              code = (StatusCode)mInfo->getStatusCode();
              mObserver->OnCreateAnswerError(code);
              break;

            case SETLOCALDESC:
              code = (StatusCode)mInfo->getStatusCode();
              mObserver->OnSetLocalDescriptionSuccess(code);
              break;

            case SETREMOTEDESC:
              code = (StatusCode)mInfo->getStatusCode();
              mObserver->OnSetRemoteDescriptionSuccess(code);
              break;

            case SETLOCALDESCERROR:
              code = (StatusCode)mInfo->getStatusCode();
              mObserver->OnSetLocalDescriptionError(code);
              break;

            case SETREMOTEDESCERROR:
              code = (StatusCode)mInfo->getStatusCode();
              mObserver->OnSetRemoteDescriptionError(code);
              break;

            case REMOTESTREAMADD:
            {
              streams = mInfo->getMediaStreams();
              nsRefPtr<RemoteSourceStreamInfo> remoteStream = mPC->GetRemoteStream(streams->media_stream_id);

              MOZ_ASSERT(remoteStream);
              if (!remoteStream)
              {
                CSFLogErrorS(logTag, __FUNCTION__ << " GetRemoteStream returned NULL");
              }
              else
              {
                stream = remoteStream->GetMediaStream();
                hint = stream->GetHintContents();
                if (hint == nsDOMMediaStream::HINT_CONTENTS_AUDIO) {
                  mObserver->OnAddStream(stream, "audio");
                } else if (hint == nsDOMMediaStream::HINT_CONTENTS_VIDEO) {
                  mObserver->OnAddStream(stream, "video");
                } else {
                  CSFLogErrorS(logTag, __FUNCTION__ << "Audio & Video not supported");
                  MOZ_ASSERT(PR_FALSE);
                }
              }
              break;
            }
            default:
              CSFLogDebugS(logTag, ": **** UNHANDLED CALL STATE : " << statestr);
              break;
          }
          break;
        }
      case PC_OBSERVER_CONNECTION:
        CSFLogDebugS(logTag, __FUNCTION__ << ": Delivering PeerConnection onconnection");
        mObserver->NotifyConnection();
        break;
      case PC_OBSERVER_CLOSEDCONNECTION:
        CSFLogDebugS(logTag, __FUNCTION__ << ": Delivering PeerConnection onclosedconnection");
        mObserver->NotifyClosedConnection();
        break;
      case PC_OBSERVER_DATACHANNEL:
        CSFLogDebugS(logTag, __FUNCTION__ << ": Delivering PeerConnection ondatachannel");
        mObserver->NotifyDataChannel(mChannel);
#ifdef MOZILLA_INTERNAL_API
        NS_DataChannelAppReady(mChannel);
#endif
        break;
      case PC_OBSERVER_ICE:
        CSFLogDebugS(logTag, __FUNCTION__ << ": Delivering PeerConnection ICE callback ");
        mObserver->OnStateChange(IPeerConnectionObserver::kIceState);
        break;
      case PC_OBSERVER_READYSTATE:
        CSFLogDebugS(logTag, __FUNCTION__ << ": Delivering PeerConnection Ready State callback ");
        mObserver->OnStateChange(IPeerConnectionObserver::kReadyState);
    }
    return NS_OK;
  }

private:
  PeerConnectionObserverType mType;
  CSF::CC_CallInfoPtr mInfo;
  nsRefPtr<nsIDOMDataChannel> mChannel;
  nsRefPtr<PeerConnectionImpl> mPC;
  nsCOMPtr<IPeerConnectionObserver> mObserver;
};





void
LocalSourceStreamInfo::NotifyQueuedTrackChanges(
  mozilla::MediaStreamGraph* aGraph,
  mozilla::TrackID aID,
  mozilla::TrackRate aTrackRate,
  mozilla::TrackTicks aTrackOffset,
  uint32_t aTrackEvents,
  const mozilla::MediaSegment& aQueuedMedia)
{
  
}





void
LocalSourceStreamInfo::ExpectAudio(const mozilla::TrackID aID)
{
  mAudioTracks.AppendElement(aID);
}



void
LocalSourceStreamInfo::ExpectVideo(const mozilla::TrackID aID)
{
  mVideoTracks.AppendElement(aID);
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

PeerConnectionImpl* PeerConnectionImpl::CreatePeerConnection()
{
  PeerConnectionImpl *pc = new PeerConnectionImpl();

  CSFLogDebugS(logTag, "Created PeerConnection: " << static_cast<void*>(pc));

  return pc;
}

std::map<const std::string, PeerConnectionImpl *>
  PeerConnectionImpl::peerconnections;

NS_IMPL_THREADSAFE_ISUPPORTS1(PeerConnectionImpl, IPeerConnection)

PeerConnectionImpl::PeerConnectionImpl()
: mRole(kRoleUnknown)
  , mCall(NULL)
  , mReadyState(kNew)
  , mPCObserver(NULL)
  , mWindow(NULL)
  , mFingerprint("TempFingerprint")
  , mLocalSourceStreamsLock(PR_NewLock())
  , mIceCtx(NULL)
  , mIceState(kIceGathering)
  , mIdentity(NULL)
  , mSTSThread(NULL)
 {}

PeerConnectionImpl::~PeerConnectionImpl()
{
  peerconnections.erase(mHandle);
  Close();
  PR_DestroyLock(mLocalSourceStreamsLock);

  




}


nsresult
PeerConnectionImpl::MakeMediaStream(uint32_t aHint, nsIDOMMediaStream** aRetval)
{
  MOZ_ASSERT(aRetval);

  nsRefPtr<nsDOMMediaStream> stream = nsDOMMediaStream::CreateInputStream(aHint);
  NS_ADDREF(*aRetval = stream);

  CSFLogDebugS(logTag, "PeerConnection " << static_cast<void*>(this)
    << ": Created media stream " << static_cast<void*>(stream)
    << " inner: " << static_cast<void*>(stream->GetStream()));

  return NS_OK;
}

nsresult
PeerConnectionImpl::MakeRemoteSource(nsDOMMediaStream* aStream, RemoteSourceStreamInfo** aInfo)
{
  MOZ_ASSERT(aInfo);
  MOZ_ASSERT(aStream);

  
  nsRefPtr<RemoteSourceStreamInfo> remote = new RemoteSourceStreamInfo(aStream);
  NS_ADDREF(*aInfo = remote);
  return NS_OK;
}

nsresult
PeerConnectionImpl::CreateRemoteSourceStreamInfo(uint32_t aHint, RemoteSourceStreamInfo** aInfo)
{
  MOZ_ASSERT(aInfo);

  nsIDOMMediaStream* stream;

  nsresult res;
  if (!mThread || NS_IsMainThread()) {
    res = MakeMediaStream(aHint, &stream);
  } else {
    mThread->Dispatch(WrapRunnableRet(
      this, &PeerConnectionImpl::MakeMediaStream, aHint, &stream, &res
    ), NS_DISPATCH_SYNC);
  }

  if (NS_FAILED(res)) {
    return res;
  }

  nsDOMMediaStream* comstream = static_cast<nsDOMMediaStream*>(stream);
  static_cast<mozilla::SourceMediaStream*>(comstream->GetStream())->SetPullEnabled(true);

  nsRefPtr<RemoteSourceStreamInfo> remote;
  if (!mThread || NS_IsMainThread()) {
    remote = new RemoteSourceStreamInfo(comstream);
    NS_ADDREF(*aInfo = remote);
    return NS_OK;
  }

  mThread->Dispatch(WrapRunnableRet(
    this, &PeerConnectionImpl::MakeRemoteSource, comstream, aInfo, &res
  ), NS_DISPATCH_SYNC);

  if (NS_FAILED(res)) {
    return res;
  }

  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::Initialize(IPeerConnectionObserver* aObserver,
                               nsIDOMWindow* aWindow,
                               nsIThread* aThread) {
  MOZ_ASSERT(aObserver);
  mPCObserver = aObserver;

#ifdef MOZILLA_INTERNAL_API
  
  
  MOZ_ASSERT(aWindow);
  mWindow = do_QueryInterface(aWindow);
  NS_ENSURE_STATE(mWindow);
#endif

  
  mThread = aThread;

  PeerConnectionCtx *pcctx = PeerConnectionCtx::GetInstance();
  MOZ_ASSERT(pcctx);

  mCall = pcctx->createCall();
  if(!mCall.get()) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": Couldn't Create Call Object");
    return NS_ERROR_FAILURE;
  }

  
  unsigned char handle_bin[8];
  PK11_GenerateRandom(handle_bin, sizeof(handle_bin));

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

  mHandle += hex;


  
  
  mIceCtx = NrIceCtx::Create("PC:" + mHandle, true);
  if(!mIceCtx) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": Failed to create Ice Context");
    return NS_ERROR_FAILURE;
  }

  mIceCtx->SignalGatheringCompleted.connect(this, &PeerConnectionImpl::IceGatheringCompleted);
  mIceCtx->SignalCompleted.connect(this, &PeerConnectionImpl::IceCompleted);

  
  
  
  RefPtr<NrIceMediaStream> audioStream = mIceCtx->CreateStream("stream1", 2);
  RefPtr<NrIceMediaStream> videoStream = mIceCtx->CreateStream("stream2", 2);
  RefPtr<NrIceMediaStream> dcStream = mIceCtx->CreateStream("stream3", 2);

  if (!audioStream) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": audio stream is NULL");
    return NS_ERROR_FAILURE;
  } else {
    mIceStreams.push_back(audioStream);
  }

  if (!videoStream) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": video stream is NULL");
    return NS_ERROR_FAILURE;
  } else {
    mIceStreams.push_back(videoStream);
  }

  if (!dcStream) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": datachannel stream is NULL");
    return NS_ERROR_FAILURE;
  } else {
    mIceStreams.push_back(dcStream);
  }

  for (std::size_t i=0; i<mIceStreams.size(); i++) {
    mIceStreams[i]->SignalReady.connect(this, &PeerConnectionImpl::IceStreamReady);
  }

  
  nsresult res;
  mIceCtx->thread()->Dispatch(WrapRunnableRet(
    mIceCtx, &NrIceCtx::StartGathering, &res), NS_DISPATCH_SYNC
  );

  if (NS_FAILED(res)) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": StartGathering failed: " <<
        static_cast<uint32_t>(res));
    return res;
  }

  
  mCall->setPeerConnection(mHandle);
  peerconnections[mHandle] = this;

  
  mIdentity = DtlsIdentity::Generate();

  if (!mIdentity) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": Generate returned NULL");
    return NS_ERROR_FAILURE;
  }

  
  
  unsigned char fingerprint[DTLS_FINGERPRINT_LENGTH];
  size_t fingerprint_length;
  res = mIdentity->ComputeFingerprint("sha-1",
                                      fingerprint,
                                      sizeof(fingerprint),
                                      &fingerprint_length);

  if (NS_FAILED(res)) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": ComputeFingerprint failed: " <<
        static_cast<uint32_t>(res));
    return res;
  }

  mFingerprint = "sha-1 " + mIdentity->FormatFingerprint(fingerprint,
                                                         fingerprint_length);

  
  mSTSThread = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &res);

  if (NS_FAILED(res)) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": do_GetService failed: " <<
        static_cast<uint32_t>(res));
    return res;
  }

#ifndef MOZILLA_INTERNAL_API
  
  CSFLogDebugS(logTag, __FUNCTION__ << ": Sleeping until kStarted");
  while(PeerConnectionCtx::GetInstance()->sipcc_state() != kStarted) {
    PR_Sleep(100);
  }
#endif

  return NS_OK;
}

nsresult
PeerConnectionImpl::CreateFakeMediaStream(uint32_t aHint, nsIDOMMediaStream** aRetval)
{
  MOZ_ASSERT(aRetval);

  bool mute = false;

  
  if (aHint & MEDIA_STREAM_MUTE) {
    mute = true;
    aHint &= ~MEDIA_STREAM_MUTE;
  }

  nsresult res;
  if (!mThread || NS_IsMainThread()) {
    res = MakeMediaStream(aHint, aRetval);
  } else {
    mThread->Dispatch(WrapRunnableRet(
      this, &PeerConnectionImpl::MakeMediaStream, aHint, aRetval, &res
    ), NS_DISPATCH_SYNC);
  }

  if (NS_FAILED(res)) {
    return res;
  }

  if (!mute) {
    if (aHint & nsDOMMediaStream::HINT_CONTENTS_AUDIO) {
      new Fake_AudioGenerator(static_cast<nsDOMMediaStream*>(*aRetval));
    } else {
#ifdef MOZILLA_INTERNAL_API
    new Fake_VideoGenerator(static_cast<nsDOMMediaStream*>(*aRetval));
#endif
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
PeerConnectionImpl::ConnectDataConnection(uint16_t aLocalport,
                                          uint16_t aRemoteport,
                                          uint16_t aNumstreams)
{
#ifdef MOZILLA_INTERNAL_API
  mDataConnection = new mozilla::DataChannelConnection(this);
  NS_ENSURE_TRUE(mDataConnection,NS_ERROR_FAILURE);
  if (!mDataConnection->Init(aLocalport, aNumstreams, true)) {
    CSFLogError(logTag,"%s DataConnection Init Failed",__FUNCTION__);
    return NS_ERROR_FAILURE;
  }
  
  nsRefPtr<TransportFlow> flow = GetTransportFlow(1,false).get();
  CSFLogDebugS(logTag, "Transportflow[1] = " << flow.get());
  if (!mDataConnection->ConnectDTLS(flow, aLocalport, aRemoteport)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

NS_IMETHODIMP
PeerConnectionImpl::CreateDataChannel(const nsACString& aLabel,
                                      uint16_t aType,
                                      bool outOfOrderAllowed,
                                      uint16_t aMaxTime,
                                      uint16_t aMaxNum,
                                      nsIDOMDataChannel** aRetval)
{
  MOZ_ASSERT(aRetval);

#ifdef MOZILLA_INTERNAL_API
  mozilla::DataChannel* dataChannel;
  mozilla::DataChannelConnection::Type theType =
    static_cast<mozilla::DataChannelConnection::Type>(aType);

  if (!mDataConnection) {
    return NS_ERROR_FAILURE;
  }
  dataChannel = mDataConnection->Open(
    aLabel, theType, !outOfOrderAllowed,
    aType == mozilla::DataChannelConnection::PARTIAL_RELIABLE_REXMIT ? aMaxNum :
    (aType == mozilla::DataChannelConnection::PARTIAL_RELIABLE_TIMED ? aMaxTime : 0),
    nullptr, nullptr
  );
  NS_ENSURE_TRUE(dataChannel,NS_ERROR_FAILURE);

  CSFLogDebugS(logTag, __FUNCTION__ << ": making DOMDataChannel");

  return NS_NewDOMDataChannel(dataChannel, mWindow, aRetval);
#else
  return NS_OK;
#endif
}

void
PeerConnectionImpl::NotifyConnection()
{
  MOZ_ASSERT(NS_IsMainThread());

  CSFLogDebugS(logTag, __FUNCTION__);

#ifdef MOZILLA_INTERNAL_API
  if (mPCObserver) {
    PeerConnectionObserverDispatch* runnable =
      new PeerConnectionObserverDispatch(PC_OBSERVER_CONNECTION, nullptr,
                                         this, mPCObserver);
    if (mThread) {
      mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      return;
    }
    runnable->Run();
  }
#endif
}

void
PeerConnectionImpl::NotifyClosedConnection()
{
  MOZ_ASSERT(NS_IsMainThread());

  CSFLogDebugS(logTag, __FUNCTION__);

#ifdef MOZILLA_INTERNAL_API
  if (mPCObserver) {
    PeerConnectionObserverDispatch* runnable =
      new PeerConnectionObserverDispatch(PC_OBSERVER_CLOSEDCONNECTION, nullptr,
                                         this, mPCObserver);
    if (mThread) {
      mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      return;
    }
    runnable->Run();
  }
#endif
}

void
PeerConnectionImpl::NotifyDataChannel(mozilla::DataChannel *aChannel)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aChannel);

  CSFLogDebugS(logTag, __FUNCTION__ << ": channel: " << static_cast<void*>(aChannel));

#ifdef MOZILLA_INTERNAL_API
  nsCOMPtr<nsIDOMDataChannel> domchannel;
  nsresult rv = NS_NewDOMDataChannel(aChannel, mWindow,
                                     getter_AddRefs(domchannel));
  NS_ENSURE_SUCCESS(rv,);
  if (mPCObserver) {
    PeerConnectionObserverDispatch* runnable =
      new PeerConnectionObserverDispatch(PC_OBSERVER_DATACHANNEL, domchannel.get(),
                                         this, mPCObserver);
    if (mThread) {
      mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      return;
    }
    runnable->Run();
  }
#endif
}












nsresult
PeerConnectionImpl::ConvertConstraints(
  const JS::Value& aConstraints, MediaConstraints* aObj, JSContext* aCx)
{
  size_t i;
  jsval mandatory, optional;
  JSObject& constraints = aConstraints.toObject();

  
  if (JS_GetProperty(aCx, &constraints, "mandatory", &mandatory)) {
    if (JSVAL_IS_PRIMITIVE(mandatory) && mandatory.isObject() && !JSVAL_IS_NULL(mandatory)) {
      JSObject* opts = JSVAL_TO_OBJECT(mandatory);
      JS::AutoIdArray mandatoryOpts(aCx, JS_Enumerate(aCx, opts));

      
      for (i = 0; i < mandatoryOpts.length(); i++) {
        jsval option, optionName;
        if (JS_GetPropertyById(aCx, opts, mandatoryOpts[i], &option)) {
          if (JS_IdToValue(aCx, mandatoryOpts[i], &optionName)) {
            
            if (JSVAL_IS_BOOLEAN(option)) {
              JSString* optionNameString = JS_ValueToString(aCx, optionName);
              NS_ConvertUTF16toUTF8 stringVal(JS_GetStringCharsZ(aCx, optionNameString));
              aObj->setBooleanConstraint(stringVal.get(), JSVAL_TO_BOOLEAN(option), true);
            }
          }
        }
      }
    }
  }

  
  if (JS_GetProperty(aCx, &constraints, "optional", &optional)) {
    if (JSVAL_IS_PRIMITIVE(optional) && optional.isObject() && !JSVAL_IS_NULL(optional)) {
      JSObject* opts = JSVAL_TO_OBJECT(optional);
      if (JS_IsArrayObject(aCx, opts)) {
        uint32_t length;
        if (!JS_GetArrayLength(aCx, opts, &length)) {
          return NS_ERROR_FAILURE;
        }
        for (i = 0; i < length; i++) {
          jsval val;
          JS_GetElement(aCx, opts, i, &val);
          if (JSVAL_IS_PRIMITIVE(val)) {
            
            
          }
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::CreateOffer(const JS::Value& aConstraints, JSContext* aCx)
{
  CheckIceState();
  mRole = kRoleOfferer;  

  MediaConstraints* cs = new MediaConstraints();
  nsresult rv = ConvertConstraints(aConstraints, cs, aCx);
  if (rv != NS_OK) {
    return rv;
  }

  return CreateOffer(*cs);
}


NS_IMETHODIMP
PeerConnectionImpl::CreateOffer(MediaConstraints& constraints)
{
  cc_media_constraints_t* cc_constraints = nullptr;
  constraints.buildArray(&cc_constraints);

  mCall->createOffer(cc_constraints);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::CreateAnswer(const JS::Value& aConstraints, JSContext* aCx)
{
  CheckIceState();
  mRole = kRoleAnswerer;  

  MediaConstraints* cs = new MediaConstraints();
  nsresult rv = ConvertConstraints(aConstraints, cs, aCx);
  if (rv != NS_OK) {
    return rv;
  }

  CreateAnswer(*cs);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::CreateAnswer(MediaConstraints& constraints)
{
  cc_media_constraints_t* cc_constraints = nullptr;
  constraints.buildArray(&cc_constraints);

  mCall->createAnswer(cc_constraints);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::SetLocalDescription(int32_t aAction, const char* aSDP)
{
  if (!aSDP) {
    CSFLogError(logTag, "%s - aSDP is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  CheckIceState();
  mLocalRequestedSDP = aSDP;
  mCall->setLocalDescription((cc_jsep_action_t)aAction, mLocalRequestedSDP);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::SetRemoteDescription(int32_t action, const char* aSDP)
{
  if (!aSDP) {
    CSFLogError(logTag, "%s - aSDP is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  CheckIceState();
  mRemoteRequestedSDP = aSDP;
  mCall->setRemoteDescription((cc_jsep_action_t)action, mRemoteRequestedSDP);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::AddStream(nsIDOMMediaStream* aMediaStream)
{
  if (!aMediaStream) {
    CSFLogError(logTag, "%s - aMediaStream is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  nsDOMMediaStream* stream = static_cast<nsDOMMediaStream*>(aMediaStream);

  CSFLogDebugS(logTag, __FUNCTION__ << ": MediaStream: " << static_cast<void*>(aMediaStream));

  
  
  uint32_t hints = stream->GetHintContents();

  if (!(hints & (nsDOMMediaStream::HINT_CONTENTS_AUDIO |
        nsDOMMediaStream::HINT_CONTENTS_VIDEO))) {
    CSFLogDebug(logTag, "Empty Stream !!");
    return NS_OK;
  }

  
  
  
  
  PR_Lock(mLocalSourceStreamsLock);
  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    nsRefPtr<LocalSourceStreamInfo> localSourceStream = mLocalSourceStreams[u];

    if (localSourceStream->GetMediaStream()->GetHintContents() & hints) {
      CSFLogError(logTag, "Only one stream of any given type allowed");
      PR_Unlock(mLocalSourceStreamsLock);
      return NS_ERROR_FAILURE;
    }
  }

  
  nsRefPtr<LocalSourceStreamInfo> localSourceStream =
    new LocalSourceStreamInfo(stream);
  cc_media_track_id_t media_stream_id = mLocalSourceStreams.Length();

  
  if (hints & nsDOMMediaStream::HINT_CONTENTS_AUDIO) {
    localSourceStream->ExpectAudio(TRACK_AUDIO);
    mCall->addStream(media_stream_id, 0, AUDIO);
  }

  if (hints & nsDOMMediaStream::HINT_CONTENTS_VIDEO) {
    localSourceStream->ExpectVideo(TRACK_VIDEO);
    mCall->addStream(media_stream_id, 1, VIDEO);
  }

  
  mozilla::MediaStream *plainMediaStream = stream->GetStream();

  if (plainMediaStream) {
    plainMediaStream->AddListener(localSourceStream);
  }

  mLocalSourceStreams.AppendElement(localSourceStream);

  PR_Unlock(mLocalSourceStreamsLock);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::RemoveStream(nsIDOMMediaStream* aMediaStream)
{
  MOZ_ASSERT(aMediaStream);

  nsDOMMediaStream* stream = static_cast<nsDOMMediaStream*>(aMediaStream);

  CSFLogDebugS(logTag, __FUNCTION__ << ": MediaStream: " << static_cast<void*>(aMediaStream));

  PR_Lock(mLocalSourceStreamsLock);
  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    nsRefPtr<LocalSourceStreamInfo> localSourceStream = mLocalSourceStreams[u];
    if (localSourceStream->GetMediaStream() == stream) {
      uint32_t hints = stream->GetHintContents();
      if (hints & nsDOMMediaStream::HINT_CONTENTS_AUDIO) {
        
        
        mCall->removeStream(u, 0, AUDIO);
      }
      if (hints & nsDOMMediaStream::HINT_CONTENTS_VIDEO) {
        mCall->removeStream(u, 1, VIDEO);
      }
      break;
    }
  }

  PR_Unlock(mLocalSourceStreamsLock);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::AddIceCandidate(const char* aCandidate, const char* aMid, unsigned short aLevel) {
  CheckIceState();
  mCall->addICECandidate(aCandidate, aMid, aLevel);
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::CloseStreams() {
  if (mReadyState != PeerConnectionImpl::kClosed)  {
    ChangeReadyState(PeerConnectionImpl::kClosing);
  }

  mCall->endCall();
  return NS_OK;
}




































NS_IMETHODIMP
PeerConnectionImpl::GetLocalDescription(char** aSDP)
{
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
  MOZ_ASSERT(aSDP);

  char* tmp = new char[mRemoteSDP.size() + 1];
  std::copy(mRemoteSDP.begin(), mRemoteSDP.end(), tmp);
  tmp[mRemoteSDP.size()] = '\0';

  *aSDP = tmp;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::GetReadyState(uint32_t* aState)
{
  MOZ_ASSERT(aState);

  *aState = mReadyState;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::GetSipccState(uint32_t* aState)
{
  MOZ_ASSERT(aState);

  PeerConnectionCtx* pcctx = PeerConnectionCtx::GetInstance();
  *aState = pcctx ? pcctx->sipcc_state() : kIdle;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::GetIceState(uint32_t* aState)
{
  MOZ_ASSERT(aState);

  *aState = mIceState;
  return NS_OK;
}

NS_IMETHODIMP
PeerConnectionImpl::Close()
{
  if (mCall != NULL)
    mCall->endCall();
#ifdef MOZILLA_INTERNAL_API
  if (mDataConnection != NULL)
    mDataConnection->CloseAll();
#endif

  ShutdownMedia();

  

  return NS_OK;
}

 void
 PeerConnectionImpl::ShutdownMedia()
 {
   CSFLogDebugS(logTag, __FUNCTION__ << " Disconnecting media streams from PC");
   
   RUN_ON_THREAD(mThread, WrapRunnable(this,
       &PeerConnectionImpl::DisconnectMediaStreams), NS_DISPATCH_SYNC);

   CSFLogDebugS(logTag, __FUNCTION__ << " Disconnecting transport");
   
   RUN_ON_THREAD(mSTSThread, WrapRunnable(
       this, &PeerConnectionImpl::ShutdownMediaTransport), NS_DISPATCH_SYNC);

  CSFLogDebugS(logTag, __FUNCTION__ << " Media shut down");
}

void
PeerConnectionImpl::DisconnectMediaStreams()
{
  
  for (uint32_t i=0; i < mLocalSourceStreams.Length(); ++i) {
    mLocalSourceStreams[i]->Detach();
  }

  for (uint32_t i=0; i < mRemoteSourceStreams.Length(); ++i) {
    mRemoteSourceStreams[i]->Detach();
  }

  mLocalSourceStreams.Clear();
  mRemoteSourceStreams.Clear();
}

void
PeerConnectionImpl::ShutdownMediaTransport()
{
  mTransportFlows.clear();
  mIceStreams.clear();
  mIceCtx = NULL;
}

void
PeerConnectionImpl::Shutdown()
{
  PeerConnectionCtx::Destroy();
}

void
PeerConnectionImpl::onCallEvent(ccapi_call_event_e aCallEvent,
                                CSF::CC_CallPtr aCall, CSF::CC_CallInfoPtr aInfo)
{
  MOZ_ASSERT(aCall.get());
  MOZ_ASSERT(aInfo.get());

  cc_call_state_t event = aInfo->getCallState();
  std::string statestr = aInfo->callStateToString(event);

  if (CCAPI_CALL_EV_CREATED != aCallEvent && CCAPI_CALL_EV_STATE != aCallEvent) {
    CSFLogDebugS(logTag, ": **** CALL HANDLE IS: " << mHandle <<
      ": **** CALL STATE IS: " << statestr);
    return;
  }

  switch (event) {
    case SETLOCALDESC:
      mLocalSDP = mLocalRequestedSDP;
      break;
    case SETREMOTEDESC:
      mRemoteSDP = mRemoteRequestedSDP;
      break;
    case CONNECTED:
      CSFLogDebugS(logTag, "Setting PeerConnnection state to kActive");
      ChangeReadyState(kActive);
      break;
    default:
      break;
  }

  if (mPCObserver) {
    PeerConnectionObserverDispatch* runnable =
        new PeerConnectionObserverDispatch(aInfo, this, mPCObserver);

    if (mThread) {
      mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      return;
    }
    runnable->Run();
  }
}

void
PeerConnectionImpl::ChangeReadyState(PeerConnectionImpl::ReadyState aReadyState)
{
  mReadyState = aReadyState;
  
  if (mPCObserver) {
    PeerConnectionObserverDispatch* runnable =
        new PeerConnectionObserverDispatch(PC_OBSERVER_READYSTATE, this, mPCObserver);

    if (mThread) {
      mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      return;
    }
    runnable->Run();
  }
}

PeerConnectionWrapper *PeerConnectionImpl::AcquireInstance(const std::string& aHandle)
{
  if (peerconnections.find(aHandle) == peerconnections.end()) {
    return NULL;
  }

  PeerConnectionImpl *impl = peerconnections[aHandle];
  impl->AddRef();

  return new PeerConnectionWrapper(impl);
}

void
PeerConnectionImpl::ReleaseInstance()
{
  Release();
}

const std::string&
PeerConnectionImpl::GetHandle()
{
  return mHandle;
}

void
PeerConnectionImpl::IceGatheringCompleted(NrIceCtx *aCtx)
{
  MOZ_ASSERT(aCtx);

  CSFLogDebugS(logTag, __FUNCTION__ << ": ctx: " << static_cast<void*>(aCtx));

  mIceState = kIceWaiting;

#ifdef MOZILLA_INTERNAL_API
  if (mPCObserver) {
    PeerConnectionObserverDispatch* runnable =
      new PeerConnectionObserverDispatch(PC_OBSERVER_ICE, this, mPCObserver);
    if (mThread) {
      mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      return;
    }
    runnable->Run();
  }
#endif
}

void
PeerConnectionImpl::IceCompleted(NrIceCtx *aCtx)
{
  MOZ_ASSERT(aCtx);

  CSFLogDebugS(logTag, __FUNCTION__ << ": ctx: " << static_cast<void*>(aCtx));

  mIceState = kIceConnected;

#ifdef MOZILLA_INTERNAL_API
  if (mPCObserver) {
    PeerConnectionObserverDispatch* runnable =
      new PeerConnectionObserverDispatch(PC_OBSERVER_ICE, this, mPCObserver);
    if (mThread) {
      mThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
      return;
    }
    runnable->Run();
  }
#endif
}

void
PeerConnectionImpl::IceStreamReady(NrIceMediaStream *aStream)
{
  MOZ_ASSERT(aStream);

  CSFLogDebugS(logTag, __FUNCTION__ << ": "  << aStream->name().c_str());
}

LocalSourceStreamInfo*
PeerConnectionImpl::GetLocalStream(int aIndex)
{
  if(aIndex < 0 || aIndex >= (int) mLocalSourceStreams.Length()) {
    return NULL;
  }

  MOZ_ASSERT(mLocalSourceStreams[aIndex]);
  return mLocalSourceStreams[aIndex];
}

RemoteSourceStreamInfo*
PeerConnectionImpl::GetRemoteStream(int aIndex)
{
  if(aIndex < 0 || aIndex >= (int) mRemoteSourceStreams.Length()) {
    return NULL;
  }

  MOZ_ASSERT(mRemoteSourceStreams[aIndex]);
  return mRemoteSourceStreams[aIndex];
}

nsresult
PeerConnectionImpl::AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo,
  int *aIndex)
{
  MOZ_ASSERT(aIndex);

  *aIndex = mRemoteSourceStreams.Length();

  mRemoteSourceStreams.AppendElement(aInfo);

  return NS_OK;
}

#ifdef MOZILLA_INTERNAL_API
static nsresult
GetStreams(JSContext* cx, PeerConnectionImpl* peerConnection,
           MediaStreamList::StreamType type, JS::Value* streams)
{
  nsAutoPtr<MediaStreamList> list(new MediaStreamList(peerConnection, type));

  ErrorResult rv;
  JSObject* obj = list->WrapObject(cx, rv);
  if (rv.Failed()) {
    streams->setNull();
    return rv.ErrorCode();
  }

  
  streams->setObject(*obj);
  list.forget();
  return NS_OK;
}
#endif

NS_IMETHODIMP
PeerConnectionImpl::GetLocalStreams(JSContext* cx, JS::Value* streams)
{
#ifdef MOZILLA_INTERNAL_API
  return GetStreams(cx, this, MediaStreamList::Local, streams);
#else
  return NS_ERROR_FAILURE;
#endif
}

NS_IMETHODIMP
PeerConnectionImpl::GetRemoteStreams(JSContext* cx, JS::Value* streams)
{
#ifdef MOZILLA_INTERNAL_API
  return GetStreams(cx, this, MediaStreamList::Remote, streams);
#else
  return NS_ERROR_FAILURE;
#endif
}

void
LocalSourceStreamInfo::StorePipeline(int aTrack,
  mozilla::RefPtr<mozilla::MediaPipeline> aPipeline)
{
  MOZ_ASSERT(mPipelines.find(aTrack) == mPipelines.end());
  if (mPipelines.find(aTrack) != mPipelines.end()) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": Storing duplicate track");
    return;
  }
  
  
  mPipelines[aTrack] = aPipeline;
}

void
RemoteSourceStreamInfo::StorePipeline(int aTrack,
  mozilla::RefPtr<mozilla::MediaPipeline> aPipeline)
{
  MOZ_ASSERT(mPipelines.find(aTrack) == mPipelines.end());
  if (mPipelines.find(aTrack) != mPipelines.end()) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": Storing duplicate track");
    return;
  }
  
  
  mPipelines[aTrack] = aPipeline;
}

}  
