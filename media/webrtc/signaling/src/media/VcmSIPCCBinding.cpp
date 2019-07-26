



#include "CSFLog.h"

#include "CC_Common.h"

#include "CSFMediaProvider.h"
#include "CSFAudioTermination.h"
#include "CSFVideoTermination.h"
#include "MediaConduitErrors.h"
#include "MediaConduitInterface.h"
#include "MediaPipeline.h"
#include "MediaPipelineFilter.h"
#include "VcmSIPCCBinding.h"
#include "csf_common.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionMedia.h"
#include "nsThreadUtils.h"
#include "transportflow.h"
#include "transportlayer.h"
#include "transportlayerdtls.h"
#include "transportlayerice.h"
#include "runnable_utils.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "mozilla/SyncRunnable.h"
#include "mozilla/Services.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsIPrincipal.h"
#include "nsIDocument.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ssl.h>
#include <sslproto.h>
#include <algorithm>

#ifdef MOZ_WEBRTC_OMX
#include "OMXVideoCodec.h"
#endif

extern "C" {
#include "ccsdp.h"
#include "vcm.h"
#include "cc_call_feature.h"
#include "cip_mmgr_mediadefinitions.h"
#include "cip_Sipcc_CodecMask.h"

extern void lsm_start_multipart_tone_timer (vcm_tones_t tone,
                                            uint32_t delay,
                                            cc_call_handle_t callId);
extern void lsm_start_continuous_tone_timer (vcm_tones_t tone,
                                             uint32_t delay,
                                             cc_call_handle_t callId);
extern void lsm_update_active_tone(vcm_tones_t tone, cc_call_handle_t call_handle);
extern void lsm_stop_multipart_tone_timer(void);
extern void lsm_stop_continuous_tone_timer(void);

static int vcmEnsureExternalCodec(
    const mozilla::RefPtr<mozilla::VideoSessionConduit>& conduit,
    mozilla::VideoCodecConfig* config,
    bool send);

}

static const char* logTag = "VcmSipccBinding";


typedef enum {
    CC_AUDIO_1,
    CC_VIDEO_1,
    CC_DATACHANNEL_1
} cc_media_cap_name;

#define SIPSDP_ILBC_MODE20 20



using namespace mozilla;
using namespace CSF;

VcmSIPCCBinding * VcmSIPCCBinding::gSelf = nullptr;
int VcmSIPCCBinding::gAudioCodecMask = 0;
int VcmSIPCCBinding::gVideoCodecMask = 0;
nsIThread *VcmSIPCCBinding::gMainThread = nullptr;
nsIEventTarget *VcmSIPCCBinding::gSTSThread = nullptr;
nsCOMPtr<nsIPrefBranch> VcmSIPCCBinding::gBranch = nullptr;

static mozilla::RefPtr<TransportFlow> vcmCreateTransportFlow(
    sipcc::PeerConnectionImpl *pc,
    int level,
    bool rtcp,
    sdp_setup_type_e setup_type,
    const char *fingerprint_alg,
    const char *fingerprint);



#define ENSURE_PC(pc, errval) \
  do { \
    if (!pc.impl()) {                                                 \
      CSFLogDebug(logTag, "%s: couldn't acquire peerconnection %s", __FUNCTION__, peerconnection); \
      return errval; \
    }         \
  } while(0)

#define ENSURE_PC_NO_RET(pc, peerconnection) \
  do { \
    if (!pc.impl()) {                                                 \
      CSFLogDebug(logTag, "%s: couldn't acquire peerconnection %s", __FUNCTION__, peerconnection); \
      return; \
    }         \
  } while(0)

VcmSIPCCBinding::VcmSIPCCBinding ()
  : streamObserver(nullptr)
{
    delete gSelf;
    gSelf = this;
  nsresult rv;

  nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    gBranch = do_QueryInterface(prefs);
  }
}

class VcmIceOpaque : public NrIceOpaque {
 public:
  VcmIceOpaque(cc_call_handle_t call_handle,
               uint16_t level) :
      call_handle_(call_handle),
      level_(level) {}

  virtual ~VcmIceOpaque() {}

  cc_call_handle_t call_handle_;
  uint16_t level_;
};


VcmSIPCCBinding::~VcmSIPCCBinding ()
{
    assert(gSelf);
    gSelf = nullptr;
    
    
    
    
    SyncRunnable::DispatchToThread(
      gSTSThread,
      WrapRunnable(this, &VcmSIPCCBinding::disconnect_all),
      true);

  gBranch = nullptr;
}

void VcmSIPCCBinding::CandidateReady(NrIceMediaStream* stream,
                                     const std::string& candidate)
{
    
    NrIceOpaque *opaque = stream->opaque();
    MOZ_ASSERT(opaque);

    VcmIceOpaque *vcm_opaque = static_cast<VcmIceOpaque *>(opaque);
    CSFLogDebug(logTag, "Candidate ready on call %u, level %u: %s",
                vcm_opaque->call_handle_, vcm_opaque->level_, candidate.c_str());

    char *candidate_tmp = (char *)malloc(candidate.size() + 1);
    if (!candidate_tmp)
        return;
    sstrncpy(candidate_tmp, candidate.c_str(), candidate.size() + 1);
    
    CC_CallFeature_FoundICECandidate(vcm_opaque->call_handle_,
                                     candidate_tmp,
                                     nullptr,
                                     vcm_opaque->level_,
                                     nullptr);
}

void VcmSIPCCBinding::setStreamObserver(StreamObserver* obs)
{
        streamObserver = obs;
}


StreamObserver * VcmSIPCCBinding::getStreamObserver()
{
    if (gSelf != nullptr)
        return gSelf->streamObserver;

    return nullptr;
}

void VcmSIPCCBinding::setMediaProviderObserver(MediaProviderObserver* obs)
{
        mediaProviderObserver = obs;
}


MediaProviderObserver * VcmSIPCCBinding::getMediaProviderObserver()
{
    if (gSelf != nullptr)
        return gSelf->mediaProviderObserver;

    return nullptr;
}

void VcmSIPCCBinding::setAudioCodecs(int codecMask)
{
  CSFLogDebug(logTag, "SETTING AUDIO: %d", codecMask);
  VcmSIPCCBinding::gAudioCodecMask = codecMask;
}

void VcmSIPCCBinding::setVideoCodecs(int codecMask)
{
  CSFLogDebug(logTag, "SETTING VIDEO: %d", codecMask);
  VcmSIPCCBinding::gVideoCodecMask = codecMask;
}

int VcmSIPCCBinding::getAudioCodecs()
{
  return VcmSIPCCBinding::gAudioCodecMask;
}

int VcmSIPCCBinding::getVideoCodecs()
{
  return VcmSIPCCBinding::gVideoCodecMask;
}

void VcmSIPCCBinding::setMainThread(nsIThread *thread)
{
  gMainThread = thread;
}

void VcmSIPCCBinding::setSTSThread(nsIEventTarget *thread)
{
  gSTSThread = thread;
}

nsIThread* VcmSIPCCBinding::getMainThread()
{
  return gMainThread;
}

nsIEventTarget* VcmSIPCCBinding::getSTSThread()
{
  return gSTSThread;
}

void VcmSIPCCBinding::connectCandidateSignal(
    NrIceMediaStream *stream)
{
  stream->SignalCandidate.connect(gSelf,
                                  &VcmSIPCCBinding::CandidateReady);
}

nsCOMPtr<nsIPrefBranch> VcmSIPCCBinding::getPrefBranch()
{
  return gBranch;
}


AudioTermination * VcmSIPCCBinding::getAudioTermination()
{
    
    return nullptr;
}


VideoTermination * VcmSIPCCBinding::getVideoTermination()
{
    
    return nullptr;
}


AudioControl * VcmSIPCCBinding::getAudioControl()
{
    
    return nullptr;
}


VideoControl * VcmSIPCCBinding::getVideoControl()
{
    
    return nullptr;
}




#define BUSY_VERIFICATION_DELAY       (10000)





#define TOH_DELAY                (10000)/*
 * Used to play msg waiting and stutter dialtones.
 * Both tones are 100ms on/off repeating 10 and
 * 3 times respectively, followed by steady dialtone.
 * Due to DSP limitations we first tell the DSP to
 * play the 100ms on/off pairs the correct number of
 * times, set a timer, and then tell it to play dialtone.
 */
#define MSG_WAITING_DELAY   (2050)
#define STUTTER_DELAY       (650)






#define BUSY_VERIFY_DELAY   (12000)


#define VCM_MIN_VOLUME_BYTE       0
#define VCM_MAX_VOLUME_BYTE       255
#define VCM_MIN_VOLUME_LEVEL      0
#define VCM_MAX_VOLUME_LEVEL      240
#define VCM_RNG_MAX_VOLUME_LEVEL  248
#define VCM_DEFAULT_AUDIO_VOLUME  144
#define VCM_DEFAULT_RINGER_VOLUME 80
#define VCM_VOLUME_ADJUST_LEVEL   8


extern "C" {




#define CREATE_MT_MAP(a,b)        ((a << 16) | b)
#define DYNAMIC_PAYLOAD_TYPE(x)    ((x >> 16) & 0xFFFF)

#define    MAX_SPROP_LEN    32

#define VCM_ERROR -1

struct h264_video
{
    char       sprop_parameter_set[MAX_SPROP_LEN];
    int        packetization_mode;
    int        profile_level_id;
    int        max_mbps;
    int        max_fs;
    int        max_cpb;
    int        max_dpb;
    int        max_br;
    int        tias_bw;
};












void vcmControlRinger (vcm_ring_mode_t ringMode,
                       short once,
                       cc_boolean alert_info,
                       int line,
                       cc_callid_t call_id)
{
    const char fname[] = "vcmControlRinger";

    CSFLogDebug( logTag, "%s: ringMode=%d once=%d", fname, ringMode, once);

    

#if 0
     
    if ( ringMode == VCM_RING_OFF )
    {
        
        if ( VcmSIPCCBinding::getAudioTermination()->ringStop( line ) != 0 )
        {
            CSFLogDebug( logTag, "%s: mediaRingStop failed", fname);
        }
    }
    else if ( VcmSIPCCBinding::getAudioTermination()->ringStart( line, map_ring_mode(ringMode), (once != 0) ) != 0 )
    {
        CSFLogDebug( logTag, "%s: mediaRingStart failed", fname);
    }
#endif
}












void vcmEnableSidetone(cc_uint16_t side_tone)
{
    
    CSFLogDebug( logTag, "vcmEnableSidetone: vcmEnableSidetone(): called");
}











static EncryptionAlgorithm
map_algorithmID( vcm_crypto_algorithmID algorithmID )
{
    switch ( algorithmID )
    {
    case VCM_AES_128_COUNTER:
        return EncryptionAlgorithm_AES_128_COUNTER;

    default:
        return EncryptionAlgorithm_NONE;
    }
}











void vcmInit (void)
{
    CSFLogDebug( logTag, "vcmInit() called.");
}




void vcmUnload (void)
{
    CSFLogDebug( logTag, "vcmUnload() called.");
}















void vcmRxAllocPort(cc_mcapid_t mcap_id,
                    cc_groupid_t group_id,
                    cc_streamid_t stream_id,
                    cc_call_handle_t  call_handle,
                    cc_uint16_t port_requested,
                    int *port_allocated)
{
    *port_allocated = -1;
    CSFLogDebug( logTag, "vcmRxAllocPort(): group_id=%d stream_id=%d call_handle=%d port_requested = %d",
        group_id, stream_id, call_handle, port_requested);

    
    int port = -1;
    bool isVideo = false;
    if(CC_IS_AUDIO(mcap_id))
    {
      isVideo = false;
      if ( VcmSIPCCBinding::getAudioTermination() != nullptr )
        port = VcmSIPCCBinding::getAudioTermination()->rxAlloc( group_id, stream_id, port_requested );
    }
    else if(CC_IS_VIDEO(mcap_id))
    {
      isVideo = true;
      if ( VcmSIPCCBinding::getVideoTermination() != nullptr )
        port = VcmSIPCCBinding::getVideoTermination()->rxAlloc( group_id, stream_id, port_requested );
    }

    StreamObserver* obs = VcmSIPCCBinding::getStreamObserver();
    if(obs != nullptr)
      obs->registerStream(call_handle, stream_id, isVideo);

    CSFLogDebug( logTag, "vcmRxAllocPort(): allocated port %d", port);
    *port_allocated = port;
}















static short vcmGetIceStream_m(cc_mcapid_t mcap_id,
                               cc_groupid_t group_id,
                               cc_streamid_t stream_id,
                               cc_call_handle_t  call_handle,
                               const char *peerconnection,
                               uint16_t level,
                               mozilla::RefPtr<NrIceCtx> *ctx,
                               mozilla::RefPtr<NrIceMediaStream> *stream)
{
  CSFLogDebug( logTag, "%s: group_id=%d stream_id=%d call_handle=%d PC = %s",
    __FUNCTION__, group_id, stream_id, call_handle, peerconnection);

  
  
  
  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  *ctx = pc.impl()->media()->ice_ctx();
  MOZ_ASSERT(*ctx);
  if (!*ctx)
    return VCM_ERROR;

  CSFLogDebug( logTag, "%s: Getting stream %d", __FUNCTION__, level);
  *stream = pc.impl()->media()->ice_media_stream(level-1);
  MOZ_ASSERT(*stream);
  if (!*stream) {
    return VCM_ERROR;
  }

  return 0;
}
















static short vcmRxAllocICE_s(TemporaryRef<NrIceCtx> ctx_in,
                             TemporaryRef<NrIceMediaStream> stream_in,
                             cc_call_handle_t  call_handle,
                             cc_streamid_t stream_id,
                             uint16_t level,
                             char **default_addrp, 
                             int *default_portp, 
                             char ***candidatesp, 
                             int *candidate_ctp 
)
{
  
  
  RefPtr<NrIceCtx> ctx(ctx_in);
  RefPtr<NrIceMediaStream> stream(stream_in);

  *default_addrp = nullptr;
  *default_portp = -1;
  *candidatesp = nullptr;
  *candidate_ctp = 0;

  
  
  if (!stream->opaque()) {
    
    stream->SetOpaque(new VcmIceOpaque(call_handle, level));

    
    VcmSIPCCBinding::connectCandidateSignal(stream);
  }

  std::vector<std::string> candidates = stream->GetCandidates();
  CSFLogDebug( logTag, "%s: Got %lu candidates", __FUNCTION__, (unsigned long) candidates.size());

  std::string default_addr;
  int default_port;

  nsresult res = stream->GetDefaultCandidate(1, &default_addr, &default_port);
  MOZ_ASSERT(NS_SUCCEEDED(res));
  if (!NS_SUCCEEDED(res)) {
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s: Got default candidates %s:%d", __FUNCTION__,
    default_addr.c_str(), default_port);

  
  *candidatesp = (char **) cpr_malloc(candidates.size() * sizeof(char *));
  if (!(*candidatesp))
    return VCM_ERROR;

  for (size_t i=0; i<candidates.size(); i++) {
    (*candidatesp)[i] = (char *) cpr_malloc(candidates[i].size() + 1);
    sstrncpy((*candidatesp)[i], candidates[i].c_str(), candidates[i].size() + 1);
  }
  *candidate_ctp = candidates.size();

  
  *default_addrp = (char *) cpr_malloc(default_addr.size() + 1);
  if (!*default_addrp)
    return VCM_ERROR;
  sstrncpy(*default_addrp, default_addr.c_str(), default_addr.size() + 1);
  *default_portp = default_port; 
  return 0;
}



















short vcmRxAllocICE(cc_mcapid_t mcap_id,
                   cc_groupid_t group_id,
                   cc_streamid_t stream_id,
                   cc_call_handle_t  call_handle,
                   const char *peerconnection,
                   uint16_t level,
                   char **default_addrp, 
                   int *default_portp, 
                   char ***candidatesp, 
                   int *candidate_ctp 
                   )
{
  int ret;

  mozilla::RefPtr<NrIceCtx> ctx;
  mozilla::RefPtr<NrIceMediaStream> stream;

  
  
  mozilla::SyncRunnable::DispatchToThread(
      VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmGetIceStream_m,
                        mcap_id,
                        group_id,
                        stream_id,
                        call_handle,
                        peerconnection,
                        level,
                        &ctx,
                        &stream,
                        &ret));
  if (ret)
    return ret;

  
  
  
  mozilla::SyncRunnable::DispatchToThread(
      VcmSIPCCBinding::getSTSThread(),
                        WrapRunnableNMRet(&vcmRxAllocICE_s,
                        ctx.forget(),
                        stream.forget(),
                        call_handle,
                        stream_id,
                        level,
                        default_addrp,
                        default_portp,
                        candidatesp,
                        candidate_ctp,
                        &ret));

  return ret;
}








static short vcmGetIceParams_m(const char *peerconnection,
                               char **ufragp,
                               char **pwdp)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  *ufragp = *pwdp = nullptr;

 
  
  
  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  std::vector<std::string> attrs = pc.impl()->media()->
    ice_ctx()->GetGlobalAttributes();

  
  char *ufrag = nullptr;
  char *pwd = nullptr;

  for (size_t i=0; i<attrs.size(); i++) {
    if (attrs[i].compare(0, strlen("ice-ufrag:"), "ice-ufrag:") == 0) {
      if (!ufrag) {
        ufrag = (char *) cpr_malloc(attrs[i].size() + 1);
        if (!ufrag)
          return VCM_ERROR;
        sstrncpy(ufrag, attrs[i].c_str(), attrs[i].size() + 1);
        ufrag[attrs[i].size()] = 0;
      }
    }

    if (attrs[i].compare(0, strlen("ice-pwd:"), "ice-pwd:") == 0) {
      pwd = (char *) cpr_malloc(attrs[i].size() + 1);
      if (!pwd)
        return VCM_ERROR;
      sstrncpy(pwd, attrs[i].c_str(), attrs[i].size() + 1);
      pwd[attrs[i].size()] = 0;
    }

  }
  if (!ufrag || !pwd) {
    MOZ_ASSERT(PR_FALSE);
    cpr_free(ufrag);
    cpr_free(pwd);
    CSFLogDebug( logTag, "%s: no ufrag or password", __FUNCTION__);
    return VCM_ERROR;
  }

  *ufragp = ufrag;
  *pwdp = pwd;

  return 0;
}











short vcmGetIceParams(const char *peerconnection,
                     char **ufragp,
                     char **pwdp)
{
  int ret;
  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmGetIceParams_m,
                        peerconnection,
                        ufragp,
                        pwdp,
                        &ret));
  return ret;
}











static short vcmSetIceSessionParams_m(const char *peerconnection,
                                      char *ufrag,
                                      char *pwd,
                                      cc_boolean icelite)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  std::vector<std::string> attributes;

  if (ufrag)
    attributes.push_back(ufrag);
  if (pwd)
    attributes.push_back(pwd);
  if (icelite) {
    attributes.push_back("ice-lite");
  }
  nsresult res = pc.impl()->media()->ice_ctx()->
    ParseGlobalAttributes(attributes);

  if (!NS_SUCCEEDED(res)) {
    CSFLogError( logTag, "%s: couldn't parse global parameters", __FUNCTION__ );
    return VCM_ERROR;
  }

  return 0;
}












short vcmSetIceSessionParams(const char *peerconnection,
                             char *ufrag,
                             char *pwd,
                             cc_boolean icelite)
{
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmSetIceSessionParams_m,
                        peerconnection,
                        ufrag,
                        pwd,
                        icelite,
                        &ret));

  return ret;
}









static short vcmSetIceCandidate_m(const char *peerconnection,
                                  const char *icecandidate,
                                  uint16_t level)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  CSFLogDebug( logTag, "%s(): Getting stream %d", __FUNCTION__, level);
  mozilla::RefPtr<NrIceMediaStream> stream = pc.impl()->media()->
    ice_media_stream(level-1);
  if (!stream)
    return VCM_ERROR;

  nsresult rv = RUN_ON_THREAD(pc.impl()->media()->ice_ctx()->thread(),
                              WrapRunnable(stream,
                                           &NrIceMediaStream::ParseTrickleCandidate,
                                           std::string(icecandidate)),
                              NS_DISPATCH_NORMAL);

  if (!NS_SUCCEEDED(rv)) {
    CSFLogError( logTag, "%s(): Could not dispatch to ICE thread, level %u",
      __FUNCTION__, level);
    return VCM_ERROR;
  }

  
  

  return 0;
}












short vcmSetIceCandidate(const char *peerconnection,
                         const char *icecandidate,
                         uint16_t level)
{
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmSetIceCandidate_m,
                        peerconnection,
                        icecandidate,
                        level,
                        &ret));

  return ret;
}






static short vcmStartIceChecks_m(const char *peerconnection, cc_boolean isControlling)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  nsresult res;
  res = pc.impl()->media()->ice_ctx()->SetControlling(
      isControlling ? NrIceCtx::ICE_CONTROLLING : NrIceCtx::ICE_CONTROLLED);
  if (!NS_SUCCEEDED(res)) {
    CSFLogError( logTag, "%s: couldn't set controlling", __FUNCTION__ );
    return VCM_ERROR;
  }
  
  
  nsresult rv = pc.impl()->media()->ice_ctx()->thread()->Dispatch(
    WrapRunnable(pc.impl()->media()->ice_ctx(), &NrIceCtx::StartChecks),
      NS_DISPATCH_NORMAL);

  if (!NS_SUCCEEDED(rv)) {
    CSFLogError( logTag, "%s(): Could not dispatch to ICE thread", __FUNCTION__);
    return VCM_ERROR;
  }

  return 0;
}








short vcmStartIceChecks(const char *peerconnection, cc_boolean isControlling)
{
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmStartIceChecks_m,
                        peerconnection,
                        isControlling,
                        &ret));

  return ret;
}











static short vcmSetIceMediaParams_m(const char *peerconnection,
                                    int level,
                                    char *ufrag,
                                    char *pwd,
                                    char **candidates,
                                    int candidate_ct)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  CSFLogDebug( logTag, "%s(): Getting stream %d", __FUNCTION__, level);
  mozilla::RefPtr<NrIceMediaStream> stream = pc.impl()->media()->
    ice_media_stream(level-1);
  if (!stream)
    return VCM_ERROR;

  std::vector<std::string> attributes;

  if (ufrag)
    attributes.push_back(ufrag);
  if (pwd)
    attributes.push_back(pwd);

  for (int i = 0; i<candidate_ct; i++) {
    attributes.push_back(candidates[i]);
  }

  nsresult res = stream->ParseAttributes(attributes);

  if (!NS_SUCCEEDED(res)) {
    CSFLogError( logTag, "%s: couldn't parse global parameters", __FUNCTION__ );
    return VCM_ERROR;
  }

  return 0;
}













short vcmSetIceMediaParams(const char *peerconnection,
                           int level,
                           char *ufrag,
                           char *pwd,
                           char **candidates,
                           int candidate_ct)
{
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmSetIceMediaParams_m,
                     peerconnection,
                     level,
                     ufrag,
                     pwd,
                     candidates,
                     candidate_ct,
                     &ret));

  return ret;
}














static short vcmCreateRemoteStream_m(
  cc_mcapid_t mcap_id,
  const char *peerconnection,
  int *pc_stream_id) {
  nsresult res;

  *pc_stream_id = -1;

  CSFLogDebug( logTag, "%s", __FUNCTION__);
  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  nsRefPtr<sipcc::RemoteSourceStreamInfo> info;
  res = pc.impl()->CreateRemoteSourceStreamInfo(&info);
  if (NS_FAILED(res)) {
    return VCM_ERROR;
  }

  res = pc.impl()->media()->AddRemoteStream(info, pc_stream_id);
  if (NS_FAILED(res)) {
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s: created remote stream with index %d",
    __FUNCTION__, *pc_stream_id);

  return 0;
}















short vcmCreateRemoteStream(cc_mcapid_t mcap_id,
                            const char *peerconnection,
                            int *pc_stream_id)
{
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmCreateRemoteStream_m,
                        mcap_id,
                        peerconnection,
                        pc_stream_id,
                        &ret));

  return ret;
}













static short vcmAddRemoteStreamHint_m(
  const char *peerconnection,
  int pc_stream_id,
  cc_boolean is_video) {
  nsresult res;

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  res = pc.impl()->media()->AddRemoteStreamHint(pc_stream_id,
    is_video ? TRUE : FALSE);
  if (NS_FAILED(res)) {
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s: added remote stream hint %u with index %d",
    __FUNCTION__, is_video, pc_stream_id);

  return 0;
}








short vcmAddRemoteStreamHint(
  const char *peerconnection,
  int pc_stream_id,
  cc_boolean is_video) {
  short ret = 0;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmAddRemoteStreamHint_m,
                        peerconnection,
                        pc_stream_id,
                        is_video,
                        &ret));

  return ret;
}












static short vcmGetDtlsIdentity_m(const char *peerconnection,
                                  char *digest_algp,
                                  size_t max_digest_alg_len,
                                  char *digestp,
                                  size_t max_digest_len) {

  digest_algp[0]='\0';
  digestp[0]='\0';

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  std::string algorithm = pc.impl()->GetFingerprintAlgorithm();
  sstrncpy(digest_algp, algorithm.c_str(), max_digest_alg_len);
  std::string value = pc.impl()->GetFingerprintHexValue();
  sstrncpy(digestp, value.c_str(), max_digest_len);

  return 0;
}














short vcmGetDtlsIdentity(const char *peerconnection,
                         char *digest_algp,
                         size_t max_digest_alg_len,
                         char *digestp,
                         size_t max_digest_len) {
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmGetDtlsIdentity_m,
                        peerconnection,
                        digest_algp,
                        max_digest_alg_len,
                        digestp,
                        max_digest_len,
                        &ret));

  return ret;
}











static short vcmInitializeDataChannel_m(const char *peerconnection,
  int track_id, cc_uint16_t streams,
  int local_datachannel_port, int remote_datachannel_port, const char* protocol)
{
  nsresult res;

  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  res = pc.impl()->InitializeDataChannel(track_id, local_datachannel_port,
                                         remote_datachannel_port, streams);
  if (NS_FAILED(res)) {
    return VCM_ERROR;
  }

  return 0;
}











short vcmInitializeDataChannel(const char *peerconnection, int track_id,
  cc_uint16_t streams,
  int local_datachannel_port, int remote_datachannel_port, const char* protocol)
{
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmInitializeDataChannel_m,
                        peerconnection,
                        track_id,
                        streams,
                        local_datachannel_port,
                        remote_datachannel_port,
                        protocol,
                        &ret));

  return ret;
}




















short vcmRxOpen(cc_mcapid_t mcap_id,
                cc_groupid_t group_id,
                cc_streamid_t stream_id,
                cc_call_handle_t call_handle,
                cc_uint16_t port_requested,
                cpr_ip_addr_t *listen_ip,
                cc_boolean is_multicast,
                int *port_allocated)
{
    char fname[] = "vcmRxOpen";

    char dottedIP[20] = "";
    *port_allocated = -1;
    if(listen_ip)
    {
        csf_sprintf(dottedIP, sizeof(dottedIP), "%u.%u.%u.%u",
                (listen_ip->u.ip4 >> 24) & 0xff, (listen_ip->u.ip4 >> 16) & 0xff,
                (listen_ip->u.ip4 >> 8) & 0xff, listen_ip->u.ip4 & 0xff );
    }

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d listen=%s:%d is_mcast=%d",
                      fname, group_id, call_handle, dottedIP, port_requested, is_multicast);

    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        CSFLogDebug( logTag, "%s: audio stream", fname);
        if ( VcmSIPCCBinding::getAudioTermination() != nullptr )
            *port_allocated = VcmSIPCCBinding::getAudioTermination()->rxOpen( group_id, stream_id,
                                                    port_requested, listen_ip ? listen_ip->u.ip4 : 0,
                                                    (is_multicast != 0) );
        break;
    case CC_VIDEO_1:
        CSFLogDebug( logTag, "%s: video stream", fname);
        if ( VcmSIPCCBinding::getVideoTermination() != nullptr )
            *port_allocated = VcmSIPCCBinding::getVideoTermination()->rxOpen( group_id, stream_id,
                                                    port_requested, listen_ip ? listen_ip->u.ip4 : 0,
                                                    (is_multicast != 0) );
        break;

    default:
        break;
    }
    return 0;
}
























int vcmRxStart(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        const vcm_payload_info_t *payload,
        cpr_ip_addr_t *local_addr,
        cc_uint16_t port,
        vcm_crypto_algorithmID algorithmID,
        vcm_crypto_key_t *rx_key,
        vcm_mediaAttrs_t *attrs)
{
    uint8_t    *key;
    uint8_t    *salt;
    cc_uint16_t    key_len;
    cc_uint16_t    salt_len;
    char        fname[] = "vcmRxStart";

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d payload=%d port=%d"
        " algID=%d", fname, group_id, call_handle, payload->remote_rtp_pt,
        port, algorithmID);

    if (call_handle == CC_NO_CALL_ID) {
        
        return VCM_ERROR;
    }

    
    switch ( algorithmID )
    {
    case VCM_AES_128_COUNTER:
        if ( rx_key == nullptr )
        {
            
            CSFLogDebug( logTag, "vcmRxStart(): No key for algorithm ID %d",
                      algorithmID);
            return VCM_ERROR;
        }
        
        key_len = rx_key->key_len;
        key = &rx_key->key[0];
        salt_len = rx_key->salt_len;
        salt = &rx_key->salt[0];
        break;

    default:
        
        key_len = 0;
        key = (uint8_t *)"";
        salt_len = 0;
        salt = (uint8_t *)"";
        break;
    }

    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != nullptr )
            return VcmSIPCCBinding::getAudioTermination()->rxStart(
                group_id, stream_id, payload->remote_rtp_pt,
                attrs->audio.packetization_period, port,
                attrs->audio.avt_payload_type, map_algorithmID(algorithmID),
                key, key_len, salt, salt_len, attrs->audio.mixing_mode,
                attrs->audio.mixing_party );
        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != nullptr )
            return VcmSIPCCBinding::getVideoTermination()->rxStart(
                group_id, stream_id, payload->remote_rtp_pt,
                0, port, 0, map_algorithmID(algorithmID), key, key_len,
                salt, salt_len, 0, 0);
        break;

    default:
        break;
    }
    return VCM_ERROR;
}
























static int vcmRxStartICE_m(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        int level,
        int pc_stream_id,
        int pc_track_id,
        cc_call_handle_t  call_handle,
        const char *peerconnection,
        int num_payloads,
        const vcm_payload_info_t* payloads,
        sdp_setup_type_e setup_type,
        const char *fingerprint_alg,
        const char *fingerprint,
        vcm_mediaAttrs_t *attrs)
{
  CSFLogDebug( logTag, "%s(%s) track = %d, stream = %d, level = %d",
              __FUNCTION__,
              peerconnection,
              pc_track_id,
              pc_stream_id,
              level);

  
  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  
  mozilla::RefPtr<TransportFlow> rtp_flow =
    vcmCreateTransportFlow(pc.impl(), level, false, setup_type,
                           fingerprint_alg, fingerprint);
  if (!rtp_flow) {
    CSFLogError( logTag, "Could not create RTP flow");
    return VCM_ERROR;
  }

  if (CC_IS_DATACHANNEL(mcap_id)) {
    
    CSFLogDebug( logTag, "%s success", __FUNCTION__);
    return 0;
  }

  if (!payloads) {
    CSFLogError( logTag, "Unitialized payload list");
    return VCM_ERROR;
  }

  
  nsRefPtr<sipcc::RemoteSourceStreamInfo> stream =
    pc.impl()->media()->GetRemoteStream(pc_stream_id);
  if (!stream) {
    
    PR_ASSERT(PR_FALSE);
    return VCM_ERROR;
  }

  mozilla::RefPtr<TransportFlow> rtcp_flow = nullptr;
  if (!attrs->rtcp_mux) {
    rtcp_flow = vcmCreateTransportFlow(pc.impl(), level, true, setup_type,
                                       fingerprint_alg, fingerprint);
    if (!rtcp_flow) {
      CSFLogError( logTag, "Could not create RTCP flow");
      return VCM_ERROR;
    }
  }

  
  
  
  nsAutoPtr<mozilla::MediaPipelineFilter> filter;
  RefPtr<TransportFlow> bundle_rtp_flow;
  RefPtr<TransportFlow> bundle_rtcp_flow;
  if (attrs->bundle_level) {
    filter = new MediaPipelineFilter;
    
    filter->SetCorrelator(attrs->bundle_stream_correlator);

    
    
    for (int s = 0; s < attrs->num_ssrcs; ++s) {
      filter->AddLocalSSRC(attrs->ssrcs[s]);
    }

    
    for (int p = 0; p < attrs->num_unique_payload_types; ++p) {
      filter->AddUniquePT(attrs->unique_payload_types[p]);
    }

    
    
    if (attrs->bundle_level != (unsigned int)level) {
      
      mozilla::RefPtr<TransportFlow> bundle_rtp_flow =
        vcmCreateTransportFlow(pc.impl(),
                               attrs->bundle_level,
                               false,
                               setup_type,
                               fingerprint_alg,
                               fingerprint);

      if (!bundle_rtp_flow) {
        CSFLogError( logTag, "Could not create bundle RTP flow");
        return VCM_ERROR;
      }

      if (!attrs->rtcp_mux) {
        bundle_rtcp_flow = vcmCreateTransportFlow(pc.impl(),
                                                  attrs->bundle_level,
                                                  true,
                                                  setup_type,
                                                  fingerprint_alg,
                                                  fingerprint);
        if (!bundle_rtcp_flow) {
          CSFLogError( logTag, "Could not create bundle RTCP flow");
          return VCM_ERROR;
        }
      }
    }
  }

  if (CC_IS_AUDIO(mcap_id)) {
    std::vector<mozilla::AudioCodecConfig *> configs;

    
    mozilla::RefPtr<mozilla::MediaSessionConduit> tx_conduit =
      pc.impl()->media()->GetConduit(level, false);
    MOZ_ASSERT_IF(tx_conduit, tx_conduit->type() == MediaSessionConduit::AUDIO);

    
    
    mozilla::RefPtr<mozilla::AudioSessionConduit> conduit =
      mozilla::AudioSessionConduit::Create(static_cast<AudioSessionConduit *>(tx_conduit.get()));
    if(!conduit)
      return VCM_ERROR;

    pc.impl()->media()->AddConduit(level, true, conduit);

    mozilla::AudioCodecConfig *config_raw;

    for(int i=0; i <num_payloads ; i++)
    {
      config_raw = new mozilla::AudioCodecConfig(
        payloads[i].remote_rtp_pt,
        ccsdpCodecName(payloads[i].codec_type),
        payloads[i].audio.frequency,
        payloads[i].audio.packet_size,
        payloads[i].audio.channels,
        payloads[i].audio.bitrate,
        pc.impl()->load_manager());
      configs.push_back(config_raw);
    }

    if (conduit->ConfigureRecvMediaCodecs(configs))
      return VCM_ERROR;

    
    mozilla::RefPtr<mozilla::MediaPipelineReceiveAudio> pipeline =
      new mozilla::MediaPipelineReceiveAudio(
        pc.impl()->GetHandle(),
        pc.impl()->GetMainThread().get(),
        pc.impl()->GetSTSThread(),
        stream->GetMediaStream()->GetStream(),
        pc_track_id,
        level,
        conduit,
        rtp_flow,
        rtcp_flow,
        bundle_rtp_flow,
        bundle_rtcp_flow,
        filter);

    nsresult res = pipeline->Init();
    if (NS_FAILED(res)) {
      CSFLogError(logTag, "Failure initializing audio pipeline");
      return VCM_ERROR;
    }

    CSFLogDebug(logTag, "Created audio pipeline %p, conduit=%p, pc_stream=%d pc_track=%d",
                pipeline.get(), conduit.get(), pc_stream_id, pc_track_id);

    stream->StorePipeline(pc_track_id, false, pipeline);
  } else if (CC_IS_VIDEO(mcap_id)) {

    std::vector<mozilla::VideoCodecConfig *> configs;
    
    mozilla::RefPtr<mozilla::MediaSessionConduit> tx_conduit =
      pc.impl()->media()->GetConduit(level, false);
    MOZ_ASSERT_IF(tx_conduit, tx_conduit->type() == MediaSessionConduit::VIDEO);

    
    
    mozilla::RefPtr<mozilla::VideoSessionConduit> conduit =
      mozilla::VideoSessionConduit::Create(static_cast<VideoSessionConduit *>(tx_conduit.get()));
    if(!conduit)
      return VCM_ERROR;

    pc.impl()->media()->AddConduit(level, true, conduit);

    mozilla::VideoCodecConfig *config_raw;

    for(int i=0; i <num_payloads; i++)
    {
      config_raw = new mozilla::VideoCodecConfig(
        payloads[i].remote_rtp_pt,
        ccsdpCodecName(payloads[i].codec_type),
        payloads[i].video.rtcp_fb_types,
        pc.impl()->load_manager());
      if (vcmEnsureExternalCodec(conduit, config_raw, false)) {
        continue;
      }
      configs.push_back(config_raw);
    }

    if (conduit->ConfigureRecvMediaCodecs(configs))
      return VCM_ERROR;

    
    mozilla::RefPtr<mozilla::MediaPipelineReceiveVideo> pipeline =
        new mozilla::MediaPipelineReceiveVideo(
            pc.impl()->GetHandle(),
            pc.impl()->GetMainThread().get(),
            pc.impl()->GetSTSThread(),
            stream->GetMediaStream()->GetStream(),
            pc_track_id,
            level,
            conduit,
            rtp_flow,
            rtcp_flow,
            bundle_rtp_flow,
            bundle_rtcp_flow,
            filter);

    nsresult res = pipeline->Init();
    if (NS_FAILED(res)) {
      CSFLogError(logTag, "Failure initializing video pipeline");
      return VCM_ERROR;
    }

    CSFLogDebug(logTag, "Created video pipeline %p, conduit=%p, pc_stream=%d pc_track=%d",
                pipeline.get(), conduit.get(), pc_stream_id, pc_track_id);

    stream->StorePipeline(pc_track_id, true, pipeline);
  } else {
    CSFLogError(logTag, "%s: mcap_id unrecognized", __FUNCTION__);
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s success", __FUNCTION__);
  return 0;
}


























int vcmRxStartICE(cc_mcapid_t mcap_id,
                  cc_groupid_t group_id,
                  cc_streamid_t stream_id,
                  int level,
                  int pc_stream_id,
                  int pc_track_id,
                  cc_call_handle_t  call_handle,
                  const char *peerconnection,
                  int num_payloads,
                  const vcm_payload_info_t* payloads,
                  sdp_setup_type_e setup_type,
                  const char *fingerprint_alg,
                  const char *fingerprint,
                  vcm_mediaAttrs_t *attrs)
{
  int ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmRxStartICE_m,
                        mcap_id,
                        group_id,
                        stream_id,
                        level,
                        pc_stream_id,
                        pc_track_id,
                        call_handle,
                        peerconnection,
                        num_payloads,
                        payloads,
                        setup_type,
                        fingerprint_alg,
                        fingerprint,
                        attrs,
                        &ret));

  return ret;
}














short vcmRxClose(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle)
{
    char fname[] = "vcmRxClose";

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d", fname, group_id, call_handle);

    if (call_handle == CC_NO_CALL_ID) {
        CSFLogDebug( logTag, "No CALL ID");
        
        return VCM_ERROR;
    }
    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != nullptr )
            VcmSIPCCBinding::getAudioTermination()->rxClose( group_id, stream_id );
        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != nullptr )
            VcmSIPCCBinding::getVideoTermination()->rxClose( group_id, stream_id );
        break;

    default:
        break;
    }
    return 0;
}












void vcmRxReleasePort  (cc_mcapid_t mcap_id,
                        cc_groupid_t group_id,
                        cc_streamid_t stream_id,
                        cc_call_handle_t  call_handle,
                        int port)
{
    CSFLogDebug( logTag, "vcmRxReleasePort(): group_id=%d stream_id=%d call_handle=%d port=%d",
                      group_id, stream_id, call_handle, port);

    if(CC_IS_AUDIO(mcap_id))
    {
        if ( VcmSIPCCBinding::getAudioTermination() != nullptr )
            VcmSIPCCBinding::getAudioTermination()->rxRelease( group_id, stream_id, port );
    }
    else if(CC_IS_VIDEO(mcap_id))
    {
        if ( VcmSIPCCBinding::getVideoTermination() != nullptr )
           VcmSIPCCBinding::getVideoTermination()->rxRelease( group_id, stream_id, port );
    }

    StreamObserver* obs = VcmSIPCCBinding::getStreamObserver();
    if(obs != nullptr)
        obs->deregisterStream(call_handle, stream_id);
}











static ToneType
map_tone_type( vcm_tones_t tone )
{
    switch ( tone )
    {
    case VCM_INSIDE_DIAL_TONE:
        return ToneType_INSIDE_DIAL_TONE;
    case VCM_OUTSIDE_DIAL_TONE:
        return ToneType_OUTSIDE_DIAL_TONE;
    case VCM_LINE_BUSY_TONE:
        return ToneType_LINE_BUSY_TONE;
    case VCM_ALERTING_TONE:
        return ToneType_ALERTING_TONE;
    case VCM_BUSY_VERIFY_TONE:
        return ToneType_BUSY_VERIFY_TONE;
    case VCM_STUTTER_TONE:
        return ToneType_STUTTER_TONE;
    case VCM_MSG_WAITING_TONE:
        return ToneType_MSG_WAITING_TONE;
    case VCM_REORDER_TONE:
        return ToneType_REORDER_TONE;
    case VCM_CALL_WAITING_TONE:
        return ToneType_CALL_WAITING_TONE;
    case VCM_CALL_WAITING_2_TONE:
        return ToneType_CALL_WAITING_2_TONE;
    case VCM_CALL_WAITING_3_TONE:
        return ToneType_CALL_WAITING_3_TONE;
    case VCM_CALL_WAITING_4_TONE:
        return ToneType_CALL_WAITING_4_TONE;
    case VCM_HOLD_TONE:
        return ToneType_HOLD_TONE;
    case VCM_CONFIRMATION_TONE:
        return ToneType_CONFIRMATION_TONE;
    case VCM_PERMANENT_SIGNAL_TONE:
        return ToneType_PERMANENT_SIGNAL_TONE;
    case VCM_REMINDER_RING_TONE:
        return ToneType_REMINDER_RING_TONE;
    case VCM_NO_TONE:
        return ToneType_NO_TONE;
    case VCM_ZIP_ZIP:
        return ToneType_ZIP_ZIP;
    case VCM_ZIP:
        return ToneType_ZIP;
    case VCM_BEEP_BONK:
        return ToneType_BEEP_BONK;
    case VCM_RECORDERWARNING_TONE:
        return ToneType_RECORDERWARNING_TONE;
    case VCM_RECORDERDETECTED_TONE:
        return ToneType_RECORDERDETECTED_TONE;
    case VCM_MONITORWARNING_TONE:
        return ToneType_MONITORWARNING_TONE;
    case VCM_SECUREWARNING_TONE:
        return ToneType_SECUREWARNING_TONE;
    default:
        CSFLogDebug( logTag, "map_tone_type(): WARNING..tone type not mapped.");
        return ToneType_NO_TONE;
    }
}
















void vcmToneStart(vcm_tones_t tone,
        short alert_info,
        cc_call_handle_t  call_handle,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_uint16_t direction)
{
    const char fname[] = "vcmToneStart";

    CSFLogDebug( logTag, "%s:tone=%d call_handle=%d dir=%d", fname, tone, call_handle, direction);

    VcmSIPCCBinding::getAudioTermination()->toneStart( map_tone_type(tone), (ToneDirection)direction,
                                alert_info, group_id, stream_id, FALSE );
    








    















    



    
}















void vcmToneStartWithSpeakerAsBackup(vcm_tones_t tone,
        short alert_info,
        cc_call_handle_t  call_handle,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_uint16_t direction)
{
    const char fname[] = "vcmToneStartWithSpeakerAsBackup";

    CSFLogDebug( logTag, "%s:tone=%d call_handle=%d dir=%d", fname, tone, call_handle, direction);

    VcmSIPCCBinding::getAudioTermination()->toneStart( map_tone_type(tone), (ToneDirection)direction,
                                alert_info, group_id, stream_id, TRUE );
    



    
}
















void vcmToneStop(vcm_tones_t tone,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t call_handle)
{
    const char fname[] = "vcmToneStop";

    CSFLogDebug( logTag, "%s:tone=%d stream_id=%d", fname, tone, stream_id);





    VcmSIPCCBinding::getAudioTermination()->toneStop( map_tone_type(tone), group_id, stream_id );
}













short vcmTxOpen(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle)
{
    char        fname[] = "vcmTxOpen";

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d", fname, group_id, call_handle);

    if (call_handle == CC_NO_CALL_ID) {
        
        return VCM_ERROR;
    }
    return 0;
}




static int vcmEnsureExternalCodec(
    const mozilla::RefPtr<mozilla::VideoSessionConduit>& conduit,
    mozilla::VideoCodecConfig* config,
    bool send)
{
  if (config->mName == "VP8") {
    
    return 0;
#ifdef MOZ_WEBRTC_OMX
  } else if (config->mName == "H264_P0" || config->mName == "H264_P1") {
    
    
    
    

    
    if (send) {
      VideoEncoder* encoder = OMXVideoCodec::CreateEncoder(OMXVideoCodec::CodecType::CODEC_H264);
      if (encoder) {
        return conduit->SetExternalSendCodec(config, encoder);
      } else {
        return kMediaConduitInvalidSendCodec;
      }
    } else {
      VideoDecoder* decoder = OMXVideoCodec::CreateDecoder(OMXVideoCodec::CodecType::CODEC_H264);
      if (decoder) {
        return conduit->SetExternalRecvCodec(config, decoder);
      } else {
        return kMediaConduitInvalidReceiveCodec;
      }
    }
    NS_NOTREACHED("Shouldn't get here!");
#else
  } else if (config->mName == "I420") {
    return 0;
#endif
  } else {
    CSFLogError( logTag, "%s: Invalid video codec configured: %s", __FUNCTION__, config->mName.c_str());
    return send ? kMediaConduitInvalidSendCodec : kMediaConduitInvalidReceiveCodec;
  }

  return 0;
}



























int vcmTxStart(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        const vcm_payload_info_t *payload,
        short tos,
        cpr_ip_addr_t *local_addr,
        cc_uint16_t local_port,
        cpr_ip_addr_t *remote_ip_addr,
        cc_uint16_t remote_port,
        vcm_crypto_algorithmID algorithmID,
        vcm_crypto_key_t *tx_key,
        vcm_mediaAttrs_t *attrs)
{
    const char fname[] = "vcmTxStart";
    uint8_t    *key;
    uint8_t    *salt;
    cc_uint16_t    key_len;
    cc_uint16_t    salt_len;

    char dottedIP[20];
    csf_sprintf(dottedIP, sizeof(dottedIP), "%u.%u.%u.%u",
                (remote_ip_addr->u.ip4 >> 24) & 0xff, (remote_ip_addr->u.ip4 >> 16) & 0xff,
                (remote_ip_addr->u.ip4 >> 8) & 0xff, remote_ip_addr->u.ip4 & 0xff );

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d payload=%d tos=%d local_port=%d remote=%s:%d algID=%d",
        fname, group_id, call_handle, payload->remote_rtp_pt, tos, local_port,
        dottedIP, remote_port, algorithmID);

    if (call_handle == CC_NO_CALL_ID) {
        
        return VCM_ERROR;
    }

    
    switch ( algorithmID )
    {
    case VCM_AES_128_COUNTER:
        if ( tx_key == nullptr )
        {
            
            CSFLogDebug( logTag, "%s: No key for algorithm ID %d", fname, algorithmID);
            return VCM_ERROR;
        }
        
        key_len  = tx_key->key_len;
        key      = &tx_key->key[0];
        salt_len = tx_key->salt_len;
        salt     = &tx_key->salt[0];
        break;

    default:
        
        key_len  = 0;
        key      = (uint8_t *)"";
        salt_len = 0;
        salt     = (uint8_t *)"";
        break;
    }

    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != nullptr )
            return VcmSIPCCBinding::getAudioTermination()->txStart(
                group_id, stream_id, payload->remote_rtp_pt,
                attrs->audio.packetization_period, (attrs->audio.vad != 0),
                tos, dottedIP, remote_port, attrs->audio.avt_payload_type,
                map_algorithmID(algorithmID), key, key_len, salt, salt_len,
                attrs->audio.mixing_mode, attrs->audio.mixing_party );

        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != nullptr )
           return VcmSIPCCBinding::getVideoTermination()->txStart(
              group_id, stream_id, payload->remote_rtp_pt,
              0, 0, tos, dottedIP, remote_port, 0,
              map_algorithmID(algorithmID), key, key_len, salt, salt_len, 0, 0);
        break;

    default:
        break;
    }
    return VCM_ERROR;
}










static int vcmTxCreateAudioConduit(int level,
                                   const vcm_payload_info_t *payload,
                                   sipcc::PeerConnectionWrapper &pc,
                                   const vcm_mediaAttrs_t *attrs,
                                   mozilla::RefPtr<mozilla::MediaSessionConduit> &conduit)
{
  mozilla::AudioCodecConfig *config_raw =
    new mozilla::AudioCodecConfig(
      payload->remote_rtp_pt,
      ccsdpCodecName(payload->codec_type),
      payload->audio.frequency,
      payload->audio.packet_size,
      payload->audio.channels,
      payload->audio.bitrate,
      pc.impl()->load_manager());

  
  mozilla::ScopedDeletePtr<mozilla::AudioCodecConfig> config(config_raw);

  
  mozilla::RefPtr<mozilla::MediaSessionConduit> rx_conduit =
    pc.impl()->media()->GetConduit(level, true);
  MOZ_ASSERT_IF(rx_conduit, rx_conduit->type() == MediaSessionConduit::AUDIO);

  
  
  mozilla::RefPtr<mozilla::AudioSessionConduit> tx_conduit =
    mozilla::AudioSessionConduit::Create(
      static_cast<AudioSessionConduit *>(rx_conduit.get()));

  if (!tx_conduit || tx_conduit->ConfigureSendMediaCodec(config) ||
      tx_conduit->EnableAudioLevelExtension(attrs->audio_level,
                                            attrs->audio_level_id)) {
    return VCM_ERROR;
  }
  CSFLogError(logTag, "Created audio pipeline audio level %d %d",
              attrs->audio_level, attrs->audio_level_id);

  conduit = tx_conduit;
  return 0;
}









static int vcmTxCreateVideoConduit(int level,
                                   const vcm_payload_info_t *payload,
                                   sipcc::PeerConnectionWrapper &pc,
                                   mozilla::RefPtr<mozilla::MediaSessionConduit> &conduit)
{
  mozilla::VideoCodecConfig *config_raw;
  config_raw = new mozilla::VideoCodecConfig(
    payload->remote_rtp_pt,
    ccsdpCodecName(payload->codec_type),
    payload->video.rtcp_fb_types,
    payload->video.max_fs,
    payload->video.max_fr,
    pc.impl()->load_manager());

  
  mozilla::ScopedDeletePtr<mozilla::VideoCodecConfig> config(config_raw);

  
  mozilla::RefPtr<mozilla::MediaSessionConduit> rx_conduit =
    pc.impl()->media()->GetConduit(level, true);
  MOZ_ASSERT_IF(rx_conduit, rx_conduit->type() == MediaSessionConduit::VIDEO);

  
  
  mozilla::RefPtr<mozilla::VideoSessionConduit> tx_conduit =
    mozilla::VideoSessionConduit::Create(static_cast<VideoSessionConduit *>(rx_conduit.get()));
  if (!tx_conduit) {
    return VCM_ERROR;
  }
  if (vcmEnsureExternalCodec(tx_conduit, config, true)) {
    return VCM_ERROR;
  }
  if (tx_conduit->ConfigureSendMediaCodec(config)) {
    return VCM_ERROR;
  }
  conduit = tx_conduit;
  return 0;
}























#define EXTRACT_DYNAMIC_PAYLOAD_TYPE(PTYPE) ((PTYPE)>>16)

static int vcmTxStartICE_m(cc_mcapid_t mcap_id,
                           cc_groupid_t group_id,
                           cc_streamid_t stream_id,
                           int level,
                           int pc_stream_id,
                           int pc_track_id,
                           cc_call_handle_t  call_handle,
                           const char *peerconnection,
                           const vcm_payload_info_t *payload,
                           short tos,
                           sdp_setup_type_e setup_type,
                           const char *fingerprint_alg,
                           const char *fingerprint,
                           vcm_mediaAttrs_t *attrs)
{
  CSFLogDebug(logTag, "%s(%s) track = %d, stream = %d, level = %d",
              __FUNCTION__,
              peerconnection,
              pc_track_id,
              pc_stream_id,
              level);

  
  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);
  nsRefPtr<sipcc::LocalSourceStreamInfo> stream =
    pc.impl()->media()->GetLocalStream(pc_stream_id);

  
  mozilla::RefPtr<TransportFlow> rtp_flow =
    vcmCreateTransportFlow(pc.impl(), level, false, setup_type,
                           fingerprint_alg, fingerprint);
  if (!rtp_flow) {
    CSFLogError(logTag, "Could not create RTP flow");
    return VCM_ERROR;
  }

  mozilla::RefPtr<TransportFlow> rtcp_flow = nullptr;
  if (!attrs->rtcp_mux) {
    rtcp_flow = vcmCreateTransportFlow(pc.impl(), level, true, setup_type,
                                       fingerprint_alg, fingerprint);
    if (!rtcp_flow) {
      CSFLogError( logTag, "Could not create RTCP flow");
      return VCM_ERROR;
    }
  }

  const char *mediaType;
  mozilla::RefPtr<mozilla::MediaSessionConduit> conduit;
  int err = VCM_ERROR;
  if (CC_IS_AUDIO(mcap_id)) {
    mediaType = "audio";
    err = vcmTxCreateAudioConduit(level, payload, pc, attrs, conduit);
  } else if (CC_IS_VIDEO(mcap_id)) {
    mediaType = "video";
    err = vcmTxCreateVideoConduit(level, payload, pc, conduit);
  } else {
    CSFLogError(logTag, "%s: mcap_id unrecognized", __FUNCTION__);
  }
  if (err) {
    return err;
  }

  pc.impl()->media()->AddConduit(level, false, conduit);

  
  mozilla::RefPtr<mozilla::MediaPipelineTransmit> pipeline =
    new mozilla::MediaPipelineTransmit(
      pc.impl()->GetHandle(),
      pc.impl()->GetMainThread().get(),
      pc.impl()->GetSTSThread(),
      stream->GetMediaStream(),
      pc_track_id,
      level,
      conduit, rtp_flow, rtcp_flow);

  nsresult res = pipeline->Init();
  if (NS_FAILED(res)) {
    CSFLogError(logTag, "Failure initializing %s pipeline", mediaType);
    return VCM_ERROR;
  }
#ifdef MOZILLA_INTERNAL_API
  
  nsIDocument* doc = pc.impl()->GetWindow()->GetExtantDoc();
  if (doc) {
    pipeline->UpdateSinkIdentity_m(doc->NodePrincipal(), pc.impl()->GetPeerIdentity());
  } else {
    CSFLogError(logTag, "Initializing pipeline without attached doc");
  }
#endif

  CSFLogDebug(logTag,
              "Created %s pipeline %p, conduit=%p, pc_stream=%d pc_track=%d",
              mediaType, pipeline.get(), conduit.get(),
              pc_stream_id, pc_track_id);
  stream->StorePipeline(pc_track_id, pipeline);

  
  
  
  if (attrs->bundle_level) {
    nsAutoPtr<mozilla::MediaPipelineFilter> filter (new MediaPipelineFilter);
    for (int s = 0; s < attrs->num_ssrcs; ++s) {
      filter->AddRemoteSSRC(attrs->ssrcs[s]);
    }
    pc.impl()->media()->SetUsingBundle_m(level, true);
    pc.impl()->media()->UpdateFilterFromRemoteDescription_m(level, filter);
  } else {
    
    pc.impl()->media()->SetUsingBundle_m(level, false);
  }

  CSFLogDebug( logTag, "%s success", __FUNCTION__);
  return 0;
}

























#define EXTRACT_DYNAMIC_PAYLOAD_TYPE(PTYPE) ((PTYPE)>>16)

int vcmTxStartICE(cc_mcapid_t mcap_id,
                  cc_groupid_t group_id,
                  cc_streamid_t stream_id,
                  int level,
                  int pc_stream_id,
                  int pc_track_id,
                  cc_call_handle_t  call_handle,
                  const char *peerconnection,
                  const vcm_payload_info_t *payload,
                  short tos,
                  sdp_setup_type_e setup_type,
                  const char *fingerprint_alg,
                  const char *fingerprint,
                  vcm_mediaAttrs_t *attrs)
{
  int ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmTxStartICE_m,
                        mcap_id,
                        group_id,
                        stream_id,
                        level,
                        pc_stream_id,
                        pc_track_id,
                        call_handle,
                        peerconnection,
                        payload,
                        tos,
                        setup_type,
                        fingerprint_alg,
                        fingerprint,
                        attrs,
                        &ret));

  return ret;
}













short vcmTxClose(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle)
{
    const char fname[] = "vcmTxClose";

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d", fname, group_id, call_handle);

    if (call_handle == CC_NO_CALL_ID) {
        
        return VCM_ERROR;
    }

    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != nullptr )
            VcmSIPCCBinding::getAudioTermination()->txClose( group_id, stream_id);
        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != nullptr )
           VcmSIPCCBinding::getVideoTermination()->txClose( group_id, stream_id);
        break;

    default:
        break;
    }
    return 0;
}

#if 0
static CodecRequestType
map_codec_request_type( int request_type )
{
    switch ( request_type )
    {
    default:
    case cip_sipcc_CodecMask_DSP_IGNORE:     return CodecRequestType_IGNORE;
    case cip_sipcc_CodecMask_DSP_DECODEONLY: return CodecRequestType_DECODEONLY;
    case cip_sipcc_CodecMask_DSP_ENCODEONLY: return CodecRequestType_ENCODEONLY;
    case cip_sipcc_CodecMask_DSP_FULLDUPLEX: return CodecRequestType_FULLDUPLEX;
    }
}
#endif













int vcmGetAudioCodecList(int request_type)
{


#if 0
    const char fname[] = "vcmGetAudioCodecList";
    int retVal;
    int codecMask = 0;

    CSFLogDebug( logTag, "%s(request_type = %d)", fname, request_type);

    retVal = VcmSIPCCBinding::getAudioTermination() ? VcmSIPCCBinding::getAudioTermination()->getCodecList( map_codec_request_type(request_type) ) : 0;

    if ( retVal & AudioCodecMask_G711 ) {    codecMask |= cip_sipcc_CodecMask_DSP_RESOURCE_G711; CSFLogDebug( logTag, "%s", " G711"); }
    if ( retVal & AudioCodecMask_LINEAR ) {  codecMask |= cip_sipcc_CodecMask_DSP_RESOURCE_LINEAR; CSFLogDebug( logTag, "%s", " LINEAR" ); }
    if ( retVal & AudioCodecMask_G722 ) {    codecMask |= cip_sipcc_CodecMask_DSP_RESOURCE_G722; CSFLogDebug( logTag, "%s", " G722"); }
    if ( retVal & AudioCodecMask_iLBC )  {   codecMask |= cip_sipcc_CodecMask_DSP_RESOURCE_iLBC; CSFLogDebug( logTag, "%s", " iLBC"); }
    if ( retVal & AudioCodecMask_iSAC )   {  codecMask |= cip_sipcc_CodecMask_DSP_RESOURCE_iSAC; CSFLogDebug( logTag, "%s", " iSAC "); }

    CSFLogDebug( logTag, "%s(codec_mask = %X)", fname, codecMask);
    return codecMask;
#else
  int codecMask = VcmSIPCCBinding::getAudioCodecs();
  CSFLogDebug(logTag, "GetAudioCodecList returning %X", codecMask);

  return codecMask;
#endif
}













#ifndef DSP_H264
#define DSP_H264        0x00000001
#endif

#ifndef DSP_H263
#define DSP_H263        0x00000002
#endif

int vcmGetVideoCodecList(int request_type)
{


#if 0
    const char fname[] = "vcmGetVideoCodecList";
    int retVal = 0;
    int codecMask = 0;

    CSFLogDebug( logTag, "%s(request_type = %d)", fname, request_type);

    retVal = VcmSIPCCBinding::getVideoTermination() ? VcmSIPCCBinding::getVideoTermination()->getCodecList( map_codec_request_type(request_type) ) : 0;

    if ( retVal & VideoCodecMask_H264 )    codecMask |= DSP_H264;
    if ( retVal & VideoCodecMask_H263 )    codecMask |= DSP_H263;

    CSFLogDebug( logTag, "%s(codec_mask = %X)", fname, codecMask);

    
        return VCM_CODEC_RESOURCE_H264;
#else
  int codecMask = VcmSIPCCBinding::getVideoCodecs();
  CSFLogDebug(logTag, "GetVideoCodecList returning %X", codecMask);

  return codecMask;
#endif
}






int vcmGetVideoMaxSupportedPacketizationMode()
{
  
  return 1;
}












void vcmMediaControl(cc_call_handle_t  call_handle, vcm_media_control_to_encoder_t to_encoder)
{
    if ( to_encoder == VCM_MEDIA_CONTROL_PICTURE_FAST_UPDATE )
    {
        StreamObserver* obs = VcmSIPCCBinding::getStreamObserver();
        if (obs != nullptr)
        {
                obs->sendIFrame(call_handle);
        }
    }
}













int vcmGetRtpStats(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        char *rx_stats,
        char *tx_stats)
{
    rx_stats[0] = '\0';
    tx_stats[0] = '\0';
    return 0;
}










void vcmSetRtcpDscp(cc_groupid_t group_id, int dscp)
{
    
}















cc_boolean vcmAllocateBandwidth(cc_call_handle_t  call_handle, int sessions)
{
    return(TRUE);
}









void vcmRemoveBandwidth(cc_call_handle_t  call_handle)
{
    
}










void vcmActivateWlan(cc_boolean is_active)
{
    
}








void vcmFreeMediaPtr(void *ptr)
{
  free(ptr);
}



















cc_boolean vcmCheckAttribs(cc_uint32_t media_type, void *sdp_p, int level, void **rcapptr)
{
    CSFLogDebug( logTag, "vcmCheckAttribs(): media=%d", media_type);

    cc_uint16_t     temp;
    const char      *ptr;
    uint32_t        t_uint;
    struct h264_video *rcap;

    *rcapptr = nullptr;

    switch (media_type)
    {
    case RTP_VP8:
        return TRUE;

    case RTP_H264_P0:
    case RTP_H264_P1:

        rcap = (struct h264_video *) cpr_malloc( sizeof(struct h264_video) );
        if ( rcap == nullptr )
        {
            CSFLogDebug( logTag, "vcmCheckAttribs(): Malloc Failed for rcap");
            return FALSE;
        }
        memset( rcap, 0, sizeof(struct h264_video) );

        if ( (ptr = ccsdpAttrGetFmtpParamSets(sdp_p, level, 0, 1)) != nullptr )
        {
            memset(rcap->sprop_parameter_set, 0, csf_countof(rcap->sprop_parameter_set));
            sstrncpy(rcap->sprop_parameter_set, ptr, csf_countof(rcap->sprop_parameter_set));
        }

        if ( ccsdpAttrGetFmtpPackMode(sdp_p, level, 0, 1, &temp) == SDP_SUCCESS )
        {
            rcap->packetization_mode = temp;
        }

        if ( (ptr = ccsdpAttrGetFmtpProfileLevelId(sdp_p, level, 0, 1)) != nullptr )
        {
#ifdef _WIN32
            sscanf_s(ptr, "%x", &rcap->profile_level_id, sizeof(int*));
#else
            sscanf(ptr, "%x", &rcap->profile_level_id);
#endif
        }

        if ( ccsdpAttrGetFmtpMaxMbps(sdp_p, level, 0, 1, &t_uint) == SDP_SUCCESS )
        {
            rcap->max_mbps = t_uint;
        }

        if ( ccsdpAttrGetFmtpMaxFs(sdp_p, level, 0, 1, &t_uint) == SDP_SUCCESS )
        {
            rcap->max_fs = t_uint;
        }

        if ( ccsdpAttrGetFmtpMaxCpb(sdp_p, level, 0, 1, &t_uint) == SDP_SUCCESS )
        {
            rcap->max_cpb = t_uint;
        }

        if ( ccsdpAttrGetFmtpMaxCpb(sdp_p, level, 0, 1, &t_uint) == SDP_SUCCESS )
        {
            rcap->max_dpb = t_uint;
        }

        if ( ccsdpAttrGetFmtpMaxCpb(sdp_p, level, 0, 1, &t_uint) == SDP_SUCCESS )
        {
            rcap->max_br = t_uint;
        }

        rcap->tias_bw = ccsdpGetBandwidthValue(sdp_p, level, 1);
        if ( rcap->tias_bw == 0 )
        {
            
            free(rcap);
            return FALSE;
        }
        else if ( rcap->tias_bw == SDP_INVALID_VALUE )
        {
            
            rcap->tias_bw = 0;
        }

        CSFLogDebug( logTag, "vcmCheckAttribs(): Negotiated media attrs\nsprop=%s\npack_mode=%d\nprofile_level_id=%X\nmbps=%d\nmax_fs=%d\nmax_cpb=%d\nmax_dpb=%d\nbr=%d bw=%d\n",
            rcap->sprop_parameter_set,
            rcap->packetization_mode,
            rcap->profile_level_id,
            rcap->max_mbps,
            rcap->max_fs,
            rcap->max_cpb,
            rcap->max_dpb,
            rcap->max_br,
            rcap->tias_bw);

        *rcapptr = rcap;
        return TRUE;

    default:
        return FALSE;
    }
}















void vcmPopulateAttribs(void *sdp_p, int level, cc_uint32_t media_type,
                          cc_uint16_t payload_number, cc_boolean isOffer)
{
    CSFLogDebug( logTag, "vcmPopulateAttribs(): media=%d PT=%d, isOffer=%d", media_type, payload_number, isOffer);
    uint16_t a_inst;
    int profile;
    char profile_level_id[MAX_SPROP_LEN];

    switch (media_type)
    {
    case RTP_H264_P0:
    case RTP_H264_P1:

        if ( ccsdpAddNewAttr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst) != SDP_SUCCESS ) return;

        (void) ccsdpAttrSetFmtpPayloadType(sdp_p, level, 0, a_inst, payload_number);

        if (media_type == RTP_H264_P1) {
          (void) ccsdpAttrSetFmtpPackMode(sdp_p, level, 0, a_inst, 1 );
        }
        

        
        profile = 0x42E00C;
        csf_sprintf(profile_level_id, MAX_SPROP_LEN, "%X", profile);
        (void) ccsdpAttrSetFmtpProfileLevelId(sdp_p, level, 0, a_inst, profile_level_id);

        
        
        
        
        
        
        

        break;

    default:
        break;
    }
}












int vcmDtmfBurst(int digit, int duration, int direction)
{
    CSFLogDebug( logTag, "vcmDtmfBurst(): digit=%d duration=%d, direction=%d", digit, duration, direction);
    StreamObserver* obs = VcmSIPCCBinding::getStreamObserver();
    if(obs != nullptr)
        obs->dtmfBurst(digit, duration, direction);
    return 0;
}








int vcmGetILBCMode()
{
    return 0;
}

} 

static mozilla::RefPtr<TransportFlow>
vcmCreateTransportFlow(sipcc::PeerConnectionImpl *pc, int level, bool rtcp,
  sdp_setup_type_e setup_type, const char *fingerprint_alg,
  const char *fingerprint) {

  
  
  
  
  
  
  
  mozilla::RefPtr<TransportFlow> flow;
  flow = pc->media()->GetTransportFlow(level, rtcp);

  if (!flow) {
    CSFLogDebug(logTag, "Making new transport flow for level=%d rtcp=%s", level, rtcp ? "true" : "false");

    char id[32];
    PR_snprintf(id, sizeof(id), "%s:%d,%s",
                pc->GetHandle().c_str(), level, rtcp ? "rtcp" : "rtp");
    flow = new TransportFlow(id);


    ScopedDeletePtr<TransportLayerIce> ice(
        new TransportLayerIce(pc->GetHandle(), pc->media()->ice_ctx(),
                              pc->media()->ice_media_stream(level-1),
                              rtcp ? 2 : 1));

    ScopedDeletePtr<TransportLayerDtls> dtls(new TransportLayerDtls());

    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    MOZ_ASSERT(setup_type == SDP_SETUP_PASSIVE ||
               setup_type == SDP_SETUP_ACTIVE);
    dtls->SetRole(
      setup_type == SDP_SETUP_PASSIVE ?
      TransportLayerDtls::SERVER : TransportLayerDtls::CLIENT);
    mozilla::RefPtr<DtlsIdentity> pcid = pc->GetIdentity();
    if (!pcid) {
      return nullptr;
    }
    dtls->SetIdentity(pcid);

    unsigned char remote_digest[TransportLayerDtls::kMaxDigestLength];
    size_t digest_len;

    nsresult res = DtlsIdentity::ParseFingerprint(fingerprint,
                                                  remote_digest,
                                                  sizeof(remote_digest),
                                                  &digest_len);
    if (!NS_SUCCEEDED(res)) {
      CSFLogError(logTag, "Could not convert fingerprint");
      return nullptr;
    }

    std::string fingerprint_str(fingerprint_alg);
    
    std::transform(fingerprint_str.begin(), fingerprint_str.end(),
                   fingerprint_str.begin(), ::tolower);
    res = dtls->SetVerificationDigest(fingerprint_str, remote_digest, digest_len);
    if (!NS_SUCCEEDED(res)) {
      CSFLogError(logTag, "Could not set remote DTLS digest");
      return nullptr;
    }

    std::vector<uint16_t> srtp_ciphers;
    srtp_ciphers.push_back(SRTP_AES128_CM_HMAC_SHA1_80);
    srtp_ciphers.push_back(SRTP_AES128_CM_HMAC_SHA1_32);

    res = dtls->SetSrtpCiphers(srtp_ciphers);
    if (!NS_SUCCEEDED(res)) {
      CSFLogError(logTag, "Couldn't set SRTP ciphers");
      return nullptr;
    }

    nsAutoPtr<std::queue<TransportLayer *> > layers(new std::queue<TransportLayer *>);
    layers->push(ice.forget());
    layers->push(dtls.forget());


    
    
    
    nsresult rv = pc->media()->ice_ctx()->thread()->Dispatch(
        WrapRunnable(flow, &TransportFlow::PushLayers, layers),
        NS_DISPATCH_NORMAL);

    if (NS_FAILED(rv)) {
      return nullptr;
    }

    
    
    pc->media()->AddTransportFlow(level, rtcp, flow);
  }
  return flow;
}











static void vcmOnSdpParseError_m(nsAutoPtr<std::string> peerconnection,
                                 nsAutoPtr<std::string> message) {

  sipcc::PeerConnectionWrapper pc(peerconnection->c_str());
  ENSURE_PC_NO_RET(pc, peerconnection->c_str());

  pc.impl()->OnSdpParseError(message->c_str());
}









int vcmOnSdpParseError(const char *peerconnection, const char *message) {
  MOZ_ASSERT(peerconnection);
  MOZ_ASSERT(message);
  nsAutoPtr<std::string> peerconnectionDuped(new std::string(peerconnection));
  nsAutoPtr<std::string> messageDuped(new std::string(message));

  
  nsresult rv = VcmSIPCCBinding::getMainThread()->Dispatch(
      WrapRunnableNM(&vcmOnSdpParseError_m,
                   peerconnectionDuped,
                   messageDuped),
      NS_DISPATCH_NORMAL);

  if (!NS_SUCCEEDED(rv)) {
    CSFLogError( logTag, "%s(): Could not dispatch to main thread", __FUNCTION__);
    return VCM_ERROR;
  }

  return 0;
}







static int vcmDisableRtcpComponent_m(const char *peerconnection, int level) {
#ifdef MOZILLA_INTERNAL_API
  MOZ_ASSERT(NS_IsMainThread());
#endif
  MOZ_ASSERT(level > 0);

  sipcc::PeerConnectionWrapper pc(peerconnection);
  ENSURE_PC(pc, VCM_ERROR);

  CSFLogDebug( logTag, "%s: disabling rtcp component %d", __FUNCTION__, level);
  mozilla::RefPtr<NrIceMediaStream> stream = pc.impl()->media()->
    ice_media_stream(level-1);
  MOZ_ASSERT(stream);
  if (!stream) {
    return VCM_ERROR;
  }

  
  nsresult res = stream->DisableComponent(2);
  MOZ_ASSERT(NS_SUCCEEDED(res));
  if (!NS_SUCCEEDED(res)) {
    return VCM_ERROR;
  }

  return 0;
}







int vcmDisableRtcpComponent(const char *peerconnection, int level) {
  int ret;
  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmDisableRtcpComponent_m,
                        peerconnection,
                        level,
                        &ret));
  return ret;
}

static short vcmGetVideoMaxFs_m(uint16_t codec,
                                int32_t *max_fs) {
  nsCOMPtr<nsIPrefBranch> branch = VcmSIPCCBinding::getPrefBranch();
  if (branch && NS_SUCCEEDED(branch->GetIntPref("media.navigator.video.max_fs",
                                                max_fs))) {
    return 0;
  }
  return VCM_ERROR;
}

short vcmGetVideoMaxFs(uint16_t codec,
                       int32_t *max_fs) {
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmGetVideoMaxFs_m,
                        codec,
                        max_fs,
                        &ret));
  return ret;
}

static short vcmGetVideoMaxFr_m(uint16_t codec,
                                int32_t *max_fr) {
  nsCOMPtr<nsIPrefBranch> branch = VcmSIPCCBinding::getPrefBranch();
  if (branch && NS_SUCCEEDED(branch->GetIntPref("media.navigator.video.max_fr",
                                                max_fr))) {
    return 0;
  }
  return VCM_ERROR;
}

short vcmGetVideoMaxFr(uint16_t codec,
                       int32_t *max_fr) {
  short ret;

  mozilla::SyncRunnable::DispatchToThread(VcmSIPCCBinding::getMainThread(),
      WrapRunnableNMRet(&vcmGetVideoMaxFr_m,
                        codec,
                        max_fr,
                        &ret));
  return ret;
}
