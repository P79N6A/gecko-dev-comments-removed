



#include "CC_Common.h"

#include "CSFLogStream.h"

#include "CSFMediaProvider.h"
#include "CSFAudioTermination.h"
#include "CSFVideoTermination.h"
#include "MediaConduitErrors.h"
#include "MediaConduitInterface.h"
#include "MediaPipeline.h"
#include "VcmSIPCCBinding.h"
#include "csf_common.h"
#include "PeerConnectionImpl.h"
#include "nsThreadUtils.h"
#include "transportflow.h"
#include "transportlayer.h"
#include "transportlayerdtls.h"
#include "transportlayerice.h"
#include "runnable_utils.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"

#include <stdlib.h>
#include <stdio.h>
#include <ssl.h>
#include <sslproto.h>

extern "C" {
#include "ccsdp.h"
#include "vcm.h"
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

}

static const char* logTag = "VcmSipccBinding";

typedef enum {
    CC_AUDIO_1,
    CC_VIDEO_1
} cc_media_cap_name;

#define SIPSDP_ILBC_MODE20 20



using namespace CSF;

VcmSIPCCBinding * VcmSIPCCBinding::_pSelf = NULL;
int VcmSIPCCBinding::mAudioCodecMask = 0;
int VcmSIPCCBinding::mVideoCodecMask = 0;









static int vcmPayloadType2AudioCodec(vcm_media_payload_type_t payload,
                                     mozilla::AudioCodecConfig **config);

static int vcmPayloadType2VideoCodec(vcm_media_payload_type_t payload,
                                     mozilla::VideoCodecConfig **config);
static mozilla::RefPtr<TransportFlow> vcmCreateTransportFlow(sipcc::PeerConnectionImpl *pc,
                                                             int level, bool rtcp,
                                                             const char *fingerprint_alg,
                                                             const char *fingerprint
                                                             );

VcmSIPCCBinding::VcmSIPCCBinding ()
  : streamObserver(NULL)
{
    delete _pSelf;
    _pSelf = this;
}

VcmSIPCCBinding::~VcmSIPCCBinding ()
{
    assert(_pSelf != NULL);
    _pSelf = NULL;
}

void VcmSIPCCBinding::setStreamObserver(StreamObserver* obs)
{
	streamObserver = obs;
}


StreamObserver * VcmSIPCCBinding::getStreamObserver()
{
    if (_pSelf != NULL)
    	return _pSelf->streamObserver;

    return NULL;
}

void VcmSIPCCBinding::setMediaProviderObserver(MediaProviderObserver* obs)
{
	mediaProviderObserver = obs;
}


MediaProviderObserver * VcmSIPCCBinding::getMediaProviderObserver()
{
    if (_pSelf != NULL)
    	return _pSelf->mediaProviderObserver;

    return NULL;
}

void VcmSIPCCBinding::setAudioCodecs(int codecMask)
{
  CSFLogDebugS(logTag, "SETTING AUDIO: " << codecMask);
  VcmSIPCCBinding::mAudioCodecMask = codecMask;
}

void VcmSIPCCBinding::setVideoCodecs(int codecMask)
{
  CSFLogDebugS(logTag, "SETTING VIDEO: " << codecMask);
  VcmSIPCCBinding::mVideoCodecMask = codecMask;
}

int VcmSIPCCBinding::getAudioCodecs()
{
  return VcmSIPCCBinding::mAudioCodecMask;
}

int VcmSIPCCBinding::getVideoCodecs()
{
  return VcmSIPCCBinding::mVideoCodecMask;
}



AudioTermination * VcmSIPCCBinding::getAudioTermination()
{
    
    return NULL;
}


VideoTermination * VcmSIPCCBinding::getVideoTermination()
{
    
    return NULL;
}


AudioControl * VcmSIPCCBinding::getAudioControl()
{
    
    return NULL;
}


VideoControl * VcmSIPCCBinding::getVideoControl()
{
    
    return NULL;
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

#if 0
static RingMode
map_ring_mode (vcm_ring_mode_t mode)
{
    switch ( mode )
    {
    case VCM_INSIDE_RING:
        return RingMode_INSIDE_RING;
    case VCM_OUTSIDE_RING:
        return RingMode_OUTSIDE_RING;
    case VCM_FEATURE_RING:
        return RingMode_FEATURE_RING;
    case VCM_BELLCORE_DR1:
        return RingMode_BELLCORE_DR1;
    case VCM_BELLCORE_DR2:
        return RingMode_BELLCORE_DR2;
    case VCM_BELLCORE_DR3:
        return RingMode_BELLCORE_DR3;
    case VCM_BELLCORE_DR4:
        return RingMode_BELLCORE_DR4;
    case VCM_BELLCORE_DR5:
        return RingMode_BELLCORE_DR5;
    case VCM_FLASHONLY_RING:
        return RingMode_FLASHONLY_RING;
    case VCM_STATION_PRECEDENCE_RING:
        return RingMode_PRECEDENCE_RING;
    default:
        CSFLogDebugS( logTag, "map_ring_mode(): Wrong ringmode passed");
        return RingMode_INSIDE_RING;
    }
}
#endif












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












#define MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCMPayloadItem, CIPPayloadItem)\
  vcmToCIP_Mappings[VCMPayloadItem] = CIPPayloadItem

static int
map_VCM_Media_Payload_type( vcm_media_payload_type_t payload )
{
    static bool mappingInitialised = false;
    static int vcmToCIP_Mappings[VCM_Media_Payload_Max] = { };

    if (!mappingInitialised)
    {
        int numElements = csf_countof(vcmToCIP_Mappings);

        std::fill_n(vcmToCIP_Mappings, numElements, cip_mmgr_MediaDefinitions_MEDIA_TYPE_NONSTANDARD);

        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_NonStandard, cip_mmgr_MediaDefinitions_MEDIA_TYPE_NONSTANDARD);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G711Alaw64k, cip_mmgr_MediaDefinitions_MEDIA_TYPE_G711ALAW64K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G711Alaw56k, cip_mmgr_MediaDefinitions_MEDIA_TYPE_G711ALAW56K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G711Ulaw64k, cip_mmgr_MediaDefinitions_MEDIA_TYPE_G711ULAW64K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G711Ulaw56k, cip_mmgr_MediaDefinitions_MEDIA_TYPE_G711ULAW56K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G722_64k,    cip_mmgr_MediaDefinitions_MEDIA_TYPE_G722_64K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G722_56k,    cip_mmgr_MediaDefinitions_MEDIA_TYPE_G722_56K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G722_48k,    cip_mmgr_MediaDefinitions_MEDIA_TYPE_G722_48K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_ILBC20,      cip_mmgr_MediaDefinitions_MEDIA_TYPE_ILBC20 | (VCM_Media_Payload_ILBC20 & 0XFFFF0000));
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_ILBC30,      cip_mmgr_MediaDefinitions_MEDIA_TYPE_ILBC30 | (cip_mmgr_MediaDefinitions_MEDIA_TYPE_ILBC30 & 0XFFFF0000));
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G7231,       cip_mmgr_MediaDefinitions_MEDIA_TYPE_G7231_5P3K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G728,        cip_mmgr_MediaDefinitions_MEDIA_TYPE_G728);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G729,        cip_mmgr_MediaDefinitions_MEDIA_TYPE_G729);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G729AnnexA,  cip_mmgr_MediaDefinitions_MEDIA_TYPE_G729ANNEXA);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_Is11172AudioCap, cip_mmgr_MediaDefinitions_MEDIA_TYPE_IS11172AUDIOCAP);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_Is13818AudioCap, cip_mmgr_MediaDefinitions_MEDIA_TYPE_IS13818AUDIOCAP);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G729AnnexB,             cip_mmgr_MediaDefinitions_MEDIA_TYPE_G729ANNEXB);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_GSM_Full_Rate,          cip_mmgr_MediaDefinitions_MEDIA_TYPE_GSM_FULL_RATE);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_GSM_Half_Rate,          cip_mmgr_MediaDefinitions_MEDIA_TYPE_GSM_HALF_RATE);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_GSM_Enhanced_Full_Rate, cip_mmgr_MediaDefinitions_MEDIA_TYPE_GSM_ENHANCED_FULL_RATE);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_Wide_Band_256k,         cip_mmgr_MediaDefinitions_MEDIA_TYPE_WIDE_BAND_256K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_Data64,                 cip_mmgr_MediaDefinitions_MEDIA_TYPE_DATA64);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_Data56,                 cip_mmgr_MediaDefinitions_MEDIA_TYPE_DATA56);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_GSM,             cip_mmgr_MediaDefinitions_MEDIA_TYPE_GSM);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_ActiveVoice,             cip_mmgr_MediaDefinitions_MEDIA_TYPE_ACTIVEVOICE);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G726_32K,             cip_mmgr_MediaDefinitions_MEDIA_TYPE_G726_32K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G726_24K,             cip_mmgr_MediaDefinitions_MEDIA_TYPE_G726_24K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_G726_16K,             cip_mmgr_MediaDefinitions_MEDIA_TYPE_G726_16K);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_VP8,                  cip_mmgr_MediaDefinitions_MEDIA_TYPE_VP8);
        MAKE_VCM_MEDIA_PAYLOAD_MAP_ENTRY(VCM_Media_Payload_OPUS,                 cip_mmgr_MediaDefinitions_MEDIA_TYPE_OPUS);

        mappingInitialised = true;
    }

    int vcmIndex = payload & 0XFFFF;

    switch (vcmIndex)
    {
    case VCM_Media_Payload_ILBC20:
        return ((payload & 0XFFFF0000) | cip_mmgr_MediaDefinitions_MEDIA_TYPE_ILBC20);
    case VCM_Media_Payload_ILBC30:
        return ((payload & 0XFFFF0000) | cip_mmgr_MediaDefinitions_MEDIA_TYPE_ILBC30);
    case VCM_Media_Payload_H263:
        return ((payload & 0XFFFF0000) | cip_mmgr_MediaDefinitions_MEDIA_TYPE_H263);
    case VCM_Media_Payload_H264:
        return ((payload & 0XFFFF0000) | RTP_H264_P0);
    case VCM_Media_Payload_ISAC:
        return ((payload & 0XFFFF0000) | cip_mmgr_MediaDefinitions_MEDIA_TYPE_ISAC);
    case VCM_Media_Payload_VP8:
        return ((payload & 0XFFFF0000) | cip_mmgr_MediaDefinitions_MEDIA_TYPE_VP8);
    default:
        
        if (vcmIndex < VCM_Media_Payload_Max)
        {
            return vcmToCIP_Mappings[vcmIndex];
        }

        return cip_mmgr_MediaDefinitions_MEDIA_TYPE_NONSTANDARD;
    }
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
    CSFLogDebug( logTag, "vcmRxAllocPort(): group_id=%d stream_id=%d call_handle=%d port_requested = %d",
        group_id, stream_id, call_handle, port_requested);

    
    int port = -1;
    bool isVideo = false;
    if(CC_IS_AUDIO(mcap_id))
    {
      isVideo = false;
      if ( VcmSIPCCBinding::getAudioTermination() != NULL )
        port = VcmSIPCCBinding::getAudioTermination()->rxAlloc( group_id, stream_id, port_requested );
    }
    else if(CC_IS_VIDEO(mcap_id))
    {
      isVideo = true;
      if ( VcmSIPCCBinding::getVideoTermination() != NULL )
        port = VcmSIPCCBinding::getVideoTermination()->rxAlloc( group_id, stream_id, port_requested );
    }

    StreamObserver* obs = VcmSIPCCBinding::getStreamObserver();
    if(obs != NULL)
      obs->registerStream(call_handle, stream_id, isVideo);

    CSFLogDebug( logTag, "vcmRxAllocPort(): allocated port %d", port);
    *port_allocated = port;
}


















void vcmRxAllocICE(cc_mcapid_t mcap_id,
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
  *default_portp = -1;

  CSFLogDebug( logTag, "%s: group_id=%d stream_id=%d call_handle=%d PC = %s",
    __FUNCTION__, group_id, stream_id, call_handle, peerconnection);

  
  
  
  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    CSFLogError(logTag, "%s: AcquireInstance returned NULL", __FUNCTION__);
    return;
  }

  CSFLogDebug( logTag, "%s: Getting stream %d", __FUNCTION__, level);
  mozilla::RefPtr<NrIceMediaStream> stream = pc->impl()->ice_media_stream(level-1);
  MOZ_ASSERT(stream);
  if (!stream) {
    return;
  }

  std::vector<std::string> candidates = stream->GetCandidates();
  CSFLogDebug( logTag, "%s: Got %d candidates", __FUNCTION__, candidates.size());

  std::string default_addr;
  int default_port;

  nsresult res = stream->GetDefaultCandidate(1, &default_addr, &default_port);
  MOZ_ASSERT(NS_SUCCEEDED(res));
  if (!NS_SUCCEEDED(res)) {
    return;
  }

  CSFLogDebug( logTag, "%s: Got default candidates %s:%d", __FUNCTION__,
    default_addr.c_str(), default_port);

  
  *candidatesp = (char **) cpr_malloc(candidates.size() * sizeof(char *));
  if (!(*candidatesp))
    return;

  for (size_t i=0; i<candidates.size(); i++) {
    (*candidatesp)[i] = (char *) cpr_malloc(candidates[i].size() + 1);
    sstrncpy((*candidatesp)[i], candidates[i].c_str(), candidates[i].size() + 1);
  }
  *candidate_ctp = candidates.size();

  
  *default_addrp = (char *) cpr_malloc(default_addr.size() + 1);
  if (!*default_addrp)
    return;
  sstrncpy(*default_addrp, default_addr.c_str(), default_addr.size() + 1);
  *default_portp = default_port; 
}









void vcmGetIceParams(const char *peerconnection, char **ufragp, char **pwdp)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  *ufragp = *pwdp = NULL;

 
  
  
  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    return;
  }

  std::vector<std::string> attrs = pc->impl()->ice_ctx()->GetGlobalAttributes();

  
  char *ufrag = NULL;
  char *pwd = NULL;

  for (size_t i=0; i<attrs.size(); i++) {
    if (attrs[i].compare(0, strlen("ice-ufrag:"), "ice-ufrag:") == 0) {
      if (!ufrag) {
        ufrag = (char *) cpr_malloc(attrs[i].size() + 1);
        if (!ufrag)
          return;
        sstrncpy(ufrag, attrs[i].c_str(), attrs[i].size() + 1);
        ufrag[attrs[i].size()] = 0;
      }
    }

    if (attrs[i].compare(0, strlen("ice-pwd:"), "ice-pwd:") == 0) {
      pwd = (char *) cpr_malloc(attrs[i].size() + 1);
      if (!pwd)
        return;
      sstrncpy(pwd, attrs[i].c_str(), attrs[i].size() + 1);
      pwd[attrs[i].size()] = 0;
    }

  }
  if (!ufrag || !pwd) {
    MOZ_ASSERT(false);
    cpr_free(ufrag);
    cpr_free(pwd);
    CSFLogDebug( logTag, "%s: no ufrag or password", __FUNCTION__);
    return;
  }

  *ufragp = ufrag;
  *pwdp = pwd;

  return;
}











short vcmSetIceSessionParams(const char *peerconnection, char *ufrag, char *pwd)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }

  std::vector<std::string> attributes;

  if (ufrag)
    attributes.push_back(ufrag);
  if (pwd)
    attributes.push_back(pwd);

  nsresult res = pc->impl()->ice_ctx()->ParseGlobalAttributes(attributes);

  if (!NS_SUCCEEDED(res)) {
    CSFLogError( logTag, "%s: couldn't parse global parameters", __FUNCTION__ );
    return VCM_ERROR;
  }

  return 0;
}









short vcmSetIceCandidate(const char *peerconnection, const char *icecandidate, uint16_t level)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s(): Getting stream %d", __FUNCTION__, level);
  mozilla::RefPtr<NrIceMediaStream> stream = pc->impl()->ice_media_stream(level-1);
  if (!stream)
    return VCM_ERROR;

  nsresult res;
  pc->impl()->ice_ctx()->thread()->Dispatch(
    WrapRunnableRet(stream, &NrIceMediaStream::ParseTrickleCandidate, icecandidate, &res),
    NS_DISPATCH_SYNC);

  if (!NS_SUCCEEDED(res)) {
    CSFLogError( logTag, "%s(): Could not parse trickle candidate for stream %d", __FUNCTION__, level);
    return VCM_ERROR;
  }

  return 0;
}





short vcmStartIceChecks(const char *peerconnection)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }

  nsresult res;
  pc->impl()->ice_ctx()->thread()->Dispatch(
      WrapRunnableRet(pc->impl()->ice_ctx(), &NrIceCtx::StartChecks, &res),
      NS_DISPATCH_SYNC);

  if (!NS_SUCCEEDED(res)) {
    CSFLogError( logTag, "%s: couldn't start ICE checks", __FUNCTION__ );
    return VCM_ERROR;
  }

  return 0;
}












short vcmSetIceMediaParams(const char *peerconnection, int level, char *ufrag, char *pwd,
                      char **candidates, int candidate_ct)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s(): Getting stream %d", __FUNCTION__, level);
  mozilla::RefPtr<NrIceMediaStream> stream = pc->impl()->ice_media_stream(level-1);
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













short vcmCreateRemoteStream(
  cc_mcapid_t mcap_id,
  const char *peerconnection,
  int *pc_stream_id,
  vcm_media_payload_type_t payload) {
  uint32_t hints = 0;
  nsresult res;

  CSFLogDebug( logTag, "%s", __FUNCTION__);

  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }

  if (CC_IS_AUDIO(mcap_id)) {
    hints |= nsDOMMediaStream::HINT_CONTENTS_AUDIO;
  }
  if (CC_IS_VIDEO(mcap_id)) {
    hints |= nsDOMMediaStream::HINT_CONTENTS_VIDEO;
  }

  sipcc::RemoteSourceStreamInfo* info;
  res = pc->impl()->CreateRemoteSourceStreamInfo(hints, &info);
  if (NS_FAILED(res)) {
    return VCM_ERROR;
  }

  res = pc->impl()->AddRemoteStream(info, pc_stream_id);
  if (NS_FAILED(res)) {
    return VCM_ERROR;
  }

  if (CC_IS_AUDIO(mcap_id)) {
    mozilla::AudioSegment *segment = new mozilla::AudioSegment();
    segment->Init(1); 
    
    info->GetMediaStream()->GetStream()->AsSourceStream()->AddTrack(1, 16000, 0, segment);

    
    info->GetMediaStream()->GetStream()->AsSourceStream()->
        AdvanceKnownTracksTime(mozilla::STREAM_TIME_MAX);
  }
  if (CC_IS_VIDEO(mcap_id)) {
    
  }

  CSFLogDebug( logTag, "%s: created remote stream with index %d hints=%d",
    __FUNCTION__, *pc_stream_id, hints);

  return 0;
}













short vcmGetDtlsIdentity(const char *peerconnection,
                char *digest_algp,
                size_t max_digest_alg_len,
                char *digestp,
                size_t max_digest_len) {
  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  MOZ_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }

  unsigned char digest[TransportLayerDtls::kMaxDigestLength];
  size_t digest_len;

  nsresult res = pc->impl()->GetIdentity()->ComputeFingerprint("sha-256", digest,
                                                               sizeof(digest),
                                                               &digest_len);
  if (!NS_SUCCEEDED(res)) {
    CSFLogError( logTag, "%s: Could not compute identity fingerprint", __FUNCTION__);
    return VCM_ERROR;
  }

  
  PR_ASSERT(digest_len == 32);
  std::string fingerprint_txt = DtlsIdentity::FormatFingerprint(digest, digest_len);
  if (max_digest_len <= fingerprint_txt.size()) {
    CSFLogError( logTag, "%s: Formatted digest will not fit in provided buffer",
                 __FUNCTION__);
    return VCM_ERROR;
  }

  sstrncpy(digest_algp, "sha-256", max_digest_alg_len);
  sstrncpy(digestp, fingerprint_txt.c_str(), max_digest_len);

  return 0;
}










short vcmSetDataChannelParameters(const char *peerconnection, cc_uint16_t streams, int sctp_port, const char* protocol)
{
  CSFLogDebug( logTag, "%s: PC = %s", __FUNCTION__, peerconnection);

  CSFLogDebug( logTag, "%s: acquiring peerconnection %s", __FUNCTION__, peerconnection);
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  PR_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }


  return 0;
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
        if ( VcmSIPCCBinding::getAudioTermination() != NULL )
            *port_allocated = VcmSIPCCBinding::getAudioTermination()->rxOpen( group_id, stream_id,
                                                    port_requested, listen_ip ? listen_ip->u.ip4 : 0,
                                                    (is_multicast != 0) );
        break;
    case CC_VIDEO_1:
        CSFLogDebug( logTag, "%s: video stream", fname);
        if ( VcmSIPCCBinding::getVideoTermination() != NULL )
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
        vcm_media_payload_type_t payload,
        cpr_ip_addr_t *local_addr,
        cc_uint16_t port,
        vcm_crypto_algorithmID algorithmID,
        vcm_crypto_key_t *rx_key,
        vcm_mediaAttrs_t *attrs)
{
    int         pt;
    uint8_t    *key;
    uint8_t    *salt;
    cc_uint16_t    key_len;
    cc_uint16_t    salt_len;
    char        fname[] = "vcmRxStart";

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d payload=%d port=%d algID=%d",
        fname, group_id, call_handle, payload, port, algorithmID);

    if (call_handle == CC_NO_CALL_ID) {
        
        return VCM_ERROR;
    }

    
    switch ( algorithmID )
    {
    case VCM_AES_128_COUNTER:
        if ( rx_key == NULL )
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

    pt = map_VCM_Media_Payload_type(payload);

    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != NULL )
            return VcmSIPCCBinding::getAudioTermination()->rxStart( group_id, stream_id, DYNAMIC_PAYLOAD_TYPE(pt),
                                                                    attrs->audio.packetization_period, port, attrs->audio.avt_payload_type,
                                                                    map_algorithmID(algorithmID), key, key_len, salt, salt_len,
                                                                    attrs->audio.mixing_mode, attrs->audio.mixing_party );
        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != NULL )
            return VcmSIPCCBinding::getVideoTermination()->rxStart( group_id, stream_id, DYNAMIC_PAYLOAD_TYPE(pt),
                                                                    0,
                                                                    port,
                                                                    0,
                                                                    map_algorithmID(algorithmID), key, key_len, salt, salt_len,
                                                                    0, 0);
        break;

    default:
        break;
    }
    return VCM_ERROR;
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
        const vcm_media_payload_type_t* payloads,
        const char *fingerprint_alg,
        const char *fingerprint,
        vcm_mediaAttrs_t *attrs)
{
  CSFLogDebug( logTag, "%s(%s)", __FUNCTION__, peerconnection);

  
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  PR_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }

  if(!payloads) {
      CSFLogError( logTag, "Unitialized payload list");
      return VCM_ERROR;
  }

  
  nsRefPtr<sipcc::RemoteSourceStreamInfo> stream =
    pc->impl()->GetRemoteStream(pc_stream_id);
  if (!stream) {
    
    PR_ASSERT(false);
    return VCM_ERROR;
  }
  
  mozilla::RefPtr<TransportFlow> rtp_flow =
      vcmCreateTransportFlow(pc->impl(), level, false,
                             fingerprint_alg, fingerprint);
  if (!rtp_flow) {
      CSFLogError( logTag, "Could not create RTP flow");
      return VCM_ERROR;
  }
  mozilla::RefPtr<TransportFlow> rtcp_flow =
      vcmCreateTransportFlow(pc->impl(), level, true,
                             fingerprint_alg, fingerprint);
  if (!rtcp_flow) {
      CSFLogError( logTag, "Could not create RTCP flow");
      return VCM_ERROR;
  }

  if (CC_IS_AUDIO(mcap_id)) {
    std::vector<mozilla::AudioCodecConfig *> configs;
    
    mozilla::RefPtr<mozilla::AudioSessionConduit> conduit =
                    mozilla::AudioSessionConduit::Create();
    if(!conduit)
      return VCM_ERROR;

    mozilla::AudioCodecConfig *config_raw;

    for(int i=0; i <num_payloads ; i++)
    {
      int ret = vcmPayloadType2AudioCodec(payloads[i], &config_raw);
      if (ret) {
       PR_ASSERT(false);
       return VCM_ERROR;
      }
      configs.push_back(config_raw);
    }

    if (conduit->ConfigureRecvMediaCodecs(configs))
      return VCM_ERROR;

    
    stream->StorePipeline(pc_track_id,
      new mozilla::MediaPipelineReceiveAudio(
        pc->impl()->GetMainThread().get(),
        pc->impl()->GetSTSThread(),
        stream->GetMediaStream(),
        conduit, rtp_flow, rtcp_flow));

  } else if (CC_IS_VIDEO(mcap_id)) {

    std::vector<mozilla::VideoCodecConfig *> configs;
    
    mozilla::RefPtr<mozilla::VideoSessionConduit> conduit =
             mozilla::VideoSessionConduit::Create();
    if(!conduit)
      return VCM_ERROR;

    mozilla::VideoCodecConfig *config_raw;

    for(int i=0; i <num_payloads; i++)
    {
      int ret = vcmPayloadType2VideoCodec(payloads[i], &config_raw);
      if (ret) {
       PR_ASSERT(false);
       return VCM_ERROR;
      }
      configs.push_back(config_raw);
    }

    if (conduit->ConfigureRecvMediaCodecs(configs))
      return VCM_ERROR;

    
    stream->StorePipeline(pc_track_id,
      new mozilla::MediaPipelineReceiveVideo(
        pc->impl()->GetMainThread().get(),
        pc->impl()->GetSTSThread(),
        stream->GetMediaStream(),
        conduit, rtp_flow, rtcp_flow));

  } else {
    CSFLogError(logTag, "%s: mcap_id unrecognized", __FUNCTION__);
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s success", __FUNCTION__);
  return 0;
}














void vcmRxClose(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle)
{
    char fname[] = "vcmRxClose";

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d", fname, group_id, call_handle);

    if (call_handle == CC_NO_CALL_ID) {
        CSFLogDebugS( logTag, "No CALL ID");
        
        return;
    }
    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != NULL )
            VcmSIPCCBinding::getAudioTermination()->rxClose( group_id, stream_id );
        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != NULL )
            VcmSIPCCBinding::getVideoTermination()->rxClose( group_id, stream_id );
        break;

    default:
        break;
    }
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
        if ( VcmSIPCCBinding::getAudioTermination() != NULL )
            VcmSIPCCBinding::getAudioTermination()->rxRelease( group_id, stream_id, port );
    }
    else if(CC_IS_VIDEO(mcap_id))
    {
        if ( VcmSIPCCBinding::getVideoTermination() != NULL )
           VcmSIPCCBinding::getVideoTermination()->rxRelease( group_id, stream_id, port );
    }

    StreamObserver* obs = VcmSIPCCBinding::getStreamObserver();
    if(obs != NULL)
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
        CSFLogDebugS( logTag, "map_tone_type(): WARNING..tone type not mapped.");
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



























int vcmTxStart(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        vcm_media_payload_type_t payload,
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
    int         pt;
    uint8_t    *key;
    uint8_t    *salt;
    cc_uint16_t    key_len;
    cc_uint16_t    salt_len;

    char dottedIP[20];
    csf_sprintf(dottedIP, sizeof(dottedIP), "%u.%u.%u.%u",
                (remote_ip_addr->u.ip4 >> 24) & 0xff, (remote_ip_addr->u.ip4 >> 16) & 0xff,
                (remote_ip_addr->u.ip4 >> 8) & 0xff, remote_ip_addr->u.ip4 & 0xff );

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d payload=%d tos=%d local_port=%d remote=%s:%d algID=%d",
        fname, group_id, call_handle, payload, tos, local_port,
        dottedIP, remote_port, algorithmID);

    if (call_handle == CC_NO_CALL_ID) {
        
        return VCM_ERROR;
    }

    
    switch ( algorithmID )
    {
    case VCM_AES_128_COUNTER:
        if ( tx_key == NULL )
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

    pt = map_VCM_Media_Payload_type(payload);

    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != NULL )
            return VcmSIPCCBinding::getAudioTermination()->txStart( group_id, stream_id, pt,
                                            attrs->audio.packetization_period, (attrs->audio.vad != 0),
                                            tos, dottedIP, remote_port, attrs->audio.avt_payload_type,
                                            map_algorithmID(algorithmID), key, key_len, salt, salt_len,
                                            attrs->audio.mixing_mode, attrs->audio.mixing_party );
        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != NULL )
           return VcmSIPCCBinding::getVideoTermination()->txStart( group_id, stream_id, pt,
                                                                   0, 0, tos, dottedIP, remote_port, 0,
                                                                   map_algorithmID(algorithmID), key, key_len, salt, salt_len, 0, 0 );
        break;

    default:
        break;
    }
    return VCM_ERROR;
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
        vcm_media_payload_type_t payload,
        short tos,
        const char *fingerprint_alg,
        const char *fingerprint,
        vcm_mediaAttrs_t *attrs)
{
  CSFLogDebug( logTag, "%s(%s)", __FUNCTION__, peerconnection);

  
  mozilla::ScopedDeletePtr<sipcc::PeerConnectionWrapper> pc(
      sipcc::PeerConnectionImpl::AcquireInstance(peerconnection));
  PR_ASSERT(pc);
  if (!pc) {
    return VCM_ERROR;
  }
  nsRefPtr<sipcc::LocalSourceStreamInfo> stream = pc->impl()->GetLocalStream(pc_stream_id);

  
  mozilla::RefPtr<TransportFlow> rtp_flow =
      vcmCreateTransportFlow(pc->impl(), level, false,
                             fingerprint_alg, fingerprint);
  if (!rtp_flow) {
      CSFLogError( logTag, "Could not create RTP flow");
      return VCM_ERROR;
  }
  mozilla::RefPtr<TransportFlow> rtcp_flow =
      vcmCreateTransportFlow(pc->impl(), level, true,
                             fingerprint_alg, fingerprint);
  if (!rtcp_flow) {
      CSFLogError( logTag, "Could not create RTCP flow");
      return VCM_ERROR;
  }

  if (CC_IS_AUDIO(mcap_id)) {
    
    mozilla::AudioCodecConfig *config_raw;
    int ret = vcmPayloadType2AudioCodec(payload, &config_raw);
    if (ret) {
      return VCM_ERROR;
    }

    
    mozilla::ScopedDeletePtr<mozilla::AudioCodecConfig> config(config_raw);

    
    mozilla::RefPtr<mozilla::AudioSessionConduit> conduit =
      mozilla::AudioSessionConduit::Create();

    if (conduit->ConfigureSendMediaCodec(config))
      return VCM_ERROR;

    mozilla::RefPtr<mozilla::MediaPipelineTransmit> pipeline =
      new mozilla::MediaPipelineTransmit(
        pc->impl()->GetMainThread().get(),
        pc->impl()->GetSTSThread(),
        stream->GetMediaStream(),
        conduit, rtp_flow, rtcp_flow);

    CSFLogDebug(logTag, "Created audio pipeline %p, conduit=%p, pc_stream=%d pc_track=%d",
                pipeline.get(), conduit.get(), pc_stream_id, pc_track_id);

    
    stream->StorePipeline(pc_track_id, pipeline);

  } else if (CC_IS_VIDEO(mcap_id)) {
    mozilla::VideoCodecConfig *config_raw;
    int ret = vcmPayloadType2VideoCodec(payload, &config_raw);
    if (ret) {
      return VCM_ERROR;
    }

    
    mozilla::ScopedDeletePtr<mozilla::VideoCodecConfig> config(config_raw);

    
    mozilla::RefPtr<mozilla::VideoSessionConduit> conduit =
      mozilla::VideoSessionConduit::Create();

    
    if (conduit->ConfigureSendMediaCodec(config))
      return VCM_ERROR;

    
    mozilla::RefPtr<mozilla::MediaPipeline> pipeline =
        new mozilla::MediaPipelineTransmit(
          pc->impl()->GetMainThread().get(),
          pc->impl()->GetSTSThread(),
          stream->GetMediaStream(),
          conduit, rtp_flow, rtcp_flow);

    CSFLogDebug(logTag, "Created video pipeline %p, conduit=%p, pc_stream=%d pc_track=%d",
                pipeline.get(), conduit.get(), pc_stream_id, pc_track_id);

    
    stream->StorePipeline(pc_track_id, pipeline);
  } else {
    CSFLogError(logTag, "%s: mcap_id unrecognized", __FUNCTION__);
    return VCM_ERROR;
  }

  CSFLogDebug( logTag, "%s success", __FUNCTION__);
  return 0;
}













void vcmTxClose(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle)
{
    const char fname[] = "vcmTxClose";

    CSFLogDebug( logTag, "%s: group_id=%d call_handle=%d", fname, group_id, call_handle);

    if (call_handle == CC_NO_CALL_ID) {
        
        return;
    }

    switch ( mcap_id )
    {
    case CC_AUDIO_1:
        if ( VcmSIPCCBinding::getAudioTermination() != NULL )
            VcmSIPCCBinding::getAudioTermination()->txClose( group_id, stream_id);
        break;

    case CC_VIDEO_1:
        if ( VcmSIPCCBinding::getVideoTermination() != NULL )
           VcmSIPCCBinding::getVideoTermination()->txClose( group_id, stream_id);
        break;

    default:
        break;
    }
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
    return 0;
}












void vcmMediaControl(cc_call_handle_t  call_handle, vcm_media_control_to_encoder_t to_encoder)
{
    if ( to_encoder == VCM_MEDIA_CONTROL_PICTURE_FAST_UPDATE )
    {
    	StreamObserver* obs = VcmSIPCCBinding::getStreamObserver();
    	if (obs != NULL)
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

    *rcapptr = NULL;

    switch (media_type)
    {
    case RTP_VP8:
    	return TRUE;

    case RTP_H264_P0:
    case RTP_H264_P1:

        rcap = (struct h264_video *) cpr_malloc( sizeof(struct h264_video) );
        if ( rcap == NULL )
        {
            CSFLogDebugS( logTag, "vcmCheckAttribs(): Malloc Failed for rcap");
            return FALSE;
        }
        memset( rcap, 0, sizeof(struct h264_video) );

        if ( (ptr = ccsdpAttrGetFmtpParamSets(sdp_p, level, 0, 1)) != NULL )
        {
            memset(rcap->sprop_parameter_set, 0, csf_countof(rcap->sprop_parameter_set));
            sstrncpy(rcap->sprop_parameter_set, ptr, csf_countof(rcap->sprop_parameter_set));
        }

        if ( ccsdpAttrGetFmtpPackMode(sdp_p, level, 0, 1, &temp) == SDP_SUCCESS )
        {
            rcap->packetization_mode = temp;
        }

        if ( (ptr = ccsdpAttrGetFmtpProfileLevelId(sdp_p, level, 0, 1)) != NULL )
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
    if(obs != NULL)
    	obs->dtmfBurst(digit, duration, direction);
    return 0;
}








int vcmGetILBCMode()
{
    return 0;
}

} 


#define EXTRACT_DYNAMIC_PAYLOAD_TYPE(PTYPE) ((PTYPE)>>16)
#define CLEAR_DYNAMIC_PAYLOAD_TYPE(PTYPE)   (PTYPE & 0x0000FFFF)
#define CHECK_DYNAMIC_PAYLOAD_TYPE(PTYPE)   (PTYPE & 0xFFFF0000)

static int vcmPayloadType2AudioCodec(vcm_media_payload_type_t payload_in,
                                     mozilla::AudioCodecConfig **config) {
  int wire_payload = -1;
  
  
  
  
  

  int payload = -1;
  *config = NULL;
  if (CHECK_DYNAMIC_PAYLOAD_TYPE(payload_in)) {
    wire_payload = EXTRACT_DYNAMIC_PAYLOAD_TYPE(payload_in);
    payload = CLEAR_DYNAMIC_PAYLOAD_TYPE(payload_in);
  }
  else {
    
    wire_payload = payload_in;
    payload = payload_in;
  }

  switch(payload) {
    case VCM_Media_Payload_G711Alaw64k:
      *config = new mozilla::AudioCodecConfig(wire_payload, "PCMA", 8000, 80, 1, 64000);
      break;
    case VCM_Media_Payload_G711Ulaw64k:
      *config = new mozilla::AudioCodecConfig(wire_payload, "PCMU", 8000, 80, 1, 64000);
      break;
    case VCM_Media_Payload_OPUS:
      *config = new mozilla::AudioCodecConfig(wire_payload, "OPUS", 48000, 480, 1, 64000);
      break;
    case VCM_Media_Payload_ISAC:
      
      *config = new mozilla::AudioCodecConfig(wire_payload, "ISAC", 16000, 480, 1, -1);
      break;
    case VCM_Media_Payload_ILBC20:
      
      *config = new mozilla::AudioCodecConfig(wire_payload, "iLBC", 8000, 160, 1, 15200);
      break;
    case VCM_Media_Payload_ILBC30:
      
      *config = new mozilla::AudioCodecConfig(wire_payload, "iLBC", 8000, 240, 1, 13300);
      break;
    case VCM_Media_Payload_G722_64k:
    case VCM_Media_Payload_G722_56k:
      
      
      CSFLogError(logTag, "vcmPayloadType2AudioCodec Codec Not Implemented !");
      PR_ASSERT(false);
      return VCM_ERROR;
    default:
      CSFLogError(logTag, "vcmPayloadType2AudioCodec unknown codec. Apparent internal error");
      PR_ASSERT(false);
      return VCM_ERROR;
  }

  return 0;
}

static int vcmPayloadType2VideoCodec(vcm_media_payload_type_t payload_in,
                              mozilla::VideoCodecConfig **config) {
  int wire_payload = -1;
  int payload = -1;
  *config = NULL;

  if (CHECK_DYNAMIC_PAYLOAD_TYPE(payload_in)) {
    wire_payload = EXTRACT_DYNAMIC_PAYLOAD_TYPE(payload_in);
    payload = CLEAR_DYNAMIC_PAYLOAD_TYPE(payload_in);
  }
  else {
    
    wire_payload = payload_in;
    payload = payload_in;
  }

  switch(payload)
  {
    case VCM_Media_Payload_I420:
      *config = new mozilla::VideoCodecConfig(wire_payload, "I420", 176, 144);
      break;
    case VCM_Media_Payload_VP8:
      *config = new mozilla::VideoCodecConfig(wire_payload, "VP8", 640, 480);
      break;
    default:
      CSFLogError(logTag, "vcmPayloadType2VideoCodec unknown codec. Apparent internal error");
      PR_ASSERT(false);
      return VCM_ERROR;
  }
  return 0;
}



static mozilla::RefPtr<TransportFlow>
vcmCreateTransportFlow(sipcc::PeerConnectionImpl *pc, int level, bool rtcp,
                       const char *fingerprint_alg,
                       const char *fingerprint) {

  
  
  
  
  
  
  
  mozilla::RefPtr<TransportFlow> flow;
  flow = pc->GetTransportFlow(level, rtcp);

  if (!flow) {
    CSFLogDebug(logTag, "Making new transport flow for level=%d rtcp=%s", level, rtcp ? "true" : "false");

    char id[32];
    PR_snprintf(id, sizeof(id), "%s:%d,%s",
                pc->GetHandle().c_str(), level, rtcp ? "rtcp" : "rtp");
    flow = new TransportFlow(id);

    flow->PushLayer(new TransportLayerIce("flow", pc->ice_ctx(),
                                          pc->ice_media_stream(level-1),
                                          rtcp ? 2 : 1));
    TransportLayerDtls *dtls = new TransportLayerDtls();
    dtls->SetRole(pc->GetRole() == sipcc::PeerConnectionImpl::kRoleOfferer ?
                  TransportLayerDtls::CLIENT : TransportLayerDtls::SERVER);
    dtls->SetIdentity(pc->GetIdentity());

    unsigned char remote_digest[TransportLayerDtls::kMaxDigestLength];
    size_t digest_len;

    nsresult res = DtlsIdentity::ParseFingerprint(fingerprint,
                                                  remote_digest,
                                                  sizeof(remote_digest),
                                                  &digest_len);
    if (!NS_SUCCEEDED(res)) {
      CSFLogError(logTag, "Could not convert fingerprint");
      return NULL;
    }
    res = dtls->SetVerificationDigest(fingerprint_alg, remote_digest, digest_len);
    if (!NS_SUCCEEDED(res)) {
      CSFLogError(logTag, "Could not set remote DTLS digest");
      return NULL;
    }

    std::vector<uint16_t> srtp_ciphers;
    srtp_ciphers.push_back(SRTP_AES128_CM_HMAC_SHA1_80);
    srtp_ciphers.push_back(SRTP_AES128_CM_HMAC_SHA1_32);

    res = dtls->SetSrtpCiphers(srtp_ciphers);
    if (!NS_SUCCEEDED(res)) {
      CSFLogError(logTag, "Couldn't set SRTP ciphers");
      return NULL;
    }

    flow->PushLayer(dtls);

    pc->AddTransportFlow(level, rtcp, flow);
  }

  return flow;
}
