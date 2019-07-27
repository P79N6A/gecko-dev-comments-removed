


















#ifndef _VCM_H_
#define _VCM_H_

#include "cpr_types.h"
#include "cc_constants.h"
#include "ccsdp.h"



#define CC_IS_AUDIO(id) ((id == CC_AUDIO_1) ? TRUE:FALSE)

#define CC_IS_VIDEO(id) ((id == CC_VIDEO_1) ? TRUE:FALSE)

#define CC_IS_DATACHANNEL(id) ((id == CC_DATACHANNEL_1) ? TRUE:FALSE)



#define VCM_PLAY_TONE_TO_EAR      1

#define VCM_PLAY_TONE_TO_NET      2

#define VCM_PLAY_TONE_TO_ALL      3


#define VCM_ALERT_INFO_OFF        0

#define VCM_ALERT_INFO_ON         1


#define VCM_CODEC_RESOURCE_G711     0x00000001
#define VCM_CODEC_RESOURCE_G729A    0x00000002
#define VCM_CODEC_RESOURCE_G729B    0x00000004
#define VCM_CODEC_RESOURCE_LINEAR   0x00000008
#define VCM_CODEC_RESOURCE_G722     0x00000010
#define VCM_CODEC_RESOURCE_iLBC     0x00000020
#define VCM_CODEC_RESOURCE_iSAC     0x00000040
#define VCM_CODEC_RESOURCE_H264     0x00000080
#define VCM_CODEC_RESOURCE_H263     0x00000002
#define VCM_CODEC_RESOURCE_VP8      0x00000100
#define VCM_CODEC_RESOURCE_I420     0x00000200
#define VCM_CODEC_RESOURCE_OPUS     0x00000400

#define VCM_DSP_DECODEONLY  0
#define VCM_DSP_ENCODEONLY  1
#define VCM_DSP_FULLDUPLEX  2
#define VCM_DSP_IGNORE      3
#define VCM_DSP_FULLDUPLEX_HW 4 // HW codecs
#define VCM_DSP_FULLDUPLEX_GMP 5 // GMP-loaded codecs


#define VCM_H264_MODE_0     1
#define VCM_H264_MODE_1     2
#define VCM_H264_MODE_2     4

#define CC_KFACTOR_STAT_LEN   (256)


#define MAX_SSRCS_PER_MEDIA_LINE 16
#define MAX_PTS_PER_MEDIA_LINE 16






typedef enum
{
    VCM_INSIDE_DIAL_TONE,
    VCM_OUTSIDE_DIAL_TONE,
    VCM_DEFAULT_TONE = 1,
    VCM_LINE_BUSY_TONE,
    VCM_ALERTING_TONE,
    VCM_BUSY_VERIFY_TONE,
    VCM_STUTTER_TONE,
    VCM_MSG_WAITING_TONE,
    VCM_REORDER_TONE,
    VCM_CALL_WAITING_TONE,
    VCM_CALL_WAITING_2_TONE,
    VCM_CALL_WAITING_3_TONE,
    VCM_CALL_WAITING_4_TONE,
    VCM_HOLD_TONE,
    VCM_CONFIRMATION_TONE,
    VCM_PERMANENT_SIGNAL_TONE,
    VCM_REMINDER_RING_TONE,
    VCM_NO_TONE,
    VCM_ZIP_ZIP,
    VCM_ZIP,
    VCM_BEEP_BONK,





    VCM_RECORDERWARNING_TONE,
    VCM_RECORDERDETECTED_TONE,
    VCM_MONITORWARNING_TONE,
    VCM_SECUREWARNING_TONE,
    VCM_NONSECUREWARNING_TONE,
    VCM_MAX_TONE,
    VCM_MAX_DIALTONE = VCM_BEEP_BONK
} vcm_tones_t;














typedef enum
{
    VCM_RING_OFF     = 0x1,
    VCM_INSIDE_RING  = 0x2,
    VCM_OUTSIDE_RING = 0x3,
    VCM_FEATURE_RING = 0x4,
    VCM_BELLCORE_DR1 = 0x5,
    VCM_RING_OFFSET  = 0x5,
    VCM_BELLCORE_DR2 = 0x6,
    VCM_BELLCORE_DR3 = 0x7,
    VCM_BELLCORE_DR4 = 0x8,
    VCM_BELLCORE_DR5 = 0x9,
    VCM_BELLCORE_MAX = VCM_BELLCORE_DR5,
    VCM_FLASHONLY_RING = 0xA,
    VCM_STATION_PRECEDENCE_RING = 0xB,
    VCM_MAX_RING = 0xC
} vcm_ring_mode_t;





typedef enum {
    vcm_station_normal_ring = 0x1,
    vcm_station_single_ring = 0x2
} vcm_ring_duration_t;




typedef struct
{
  rtp_ptype codec_type;

  







  
  int remote_rtp_pt;

  
  int local_rtp_pt;

  
  union
  {
    struct
    {
      int frequency;
      int packet_size; 
      int channels;
      int bitrate;     
    } audio;

    struct
    {
      int width;
      int height;
      uint32_t rtcp_fb_types;
      uint32_t max_fs; 
      uint32_t max_fr; 
    } video;
  };

  
  union
  {
    struct {
        uint16_t mode;
    } ilbc;

    

    struct {
        uint32_t max_average_bitrate;
        const char *maxcodedaudiobandwidth;
        boolean usedtx;
        boolean stereo;
        boolean useinbandfec;
        boolean cbr;
    } opus;
  };
} vcm_payload_info_t;





typedef enum vcm_vad_t_ {
    VCM_VAD_OFF = 0,
    VCM_VAD_ON = 1
} vcm_vad_t;





typedef enum vcm_audio_bits_ {
    VCM_AUDIO_NONE,
    VCM_AUDIO_HANDSET,
    VCM_AUDIO_HEADSET,
    VCM_AUDIO_SPEAKER
} vcm_audio_bits_t;





typedef enum {
    VCM_INVLID_ALGORITM_ID = -1,    
    VCM_NO_ENCRYPTION = 0,          
    VCM_AES_128_COUNTER             
} vcm_crypto_algorithmID;





typedef enum {
    VCM_NO_MIX,
    VCM_MIX
} vcm_mixing_mode_t;





typedef enum {
    PRIMARY_SESSION,
    MIX_SESSION,
    NO_SESSION
} vcm_session_t;








typedef enum {
    VCM_PARTY_NONE,
    VCM_PARTY_LOCAL,
    VCM_PARTY_REMOTE,
    VCM_PARTY_BOTH,
    VCM_PARTY_TxBOTH_RxNONE
} vcm_mixing_party_t;






typedef enum {
    VCM_MEDIA_CONTROL_PICTURE_FAST_UPDATE
} vcm_media_control_to_encoder_t;





#define VCM_SRTP_MAX_KEY_SIZE  16   /* maximum key in bytes (128 bits) */




#define VCM_SRTP_MAX_SALT_SIZE 14   /* maximum salt in bytes (112 bits)*/


#define VCM_AES_128_COUNTER_KEY_SIZE  16

#define VCM_AES_128_COUNTER_SALT_SIZE 14


typedef struct vcm_crypto_key_t_ {
    cc_uint8_t key_len; 
    cc_uint8_t key[VCM_SRTP_MAX_KEY_SIZE]; 
    cc_uint8_t salt_len; 
    cc_uint8_t salt[VCM_SRTP_MAX_SALT_SIZE]; 
} vcm_crypto_key_t;





typedef struct vcm_videoAttrs_t_ {
  void * opaque; 
} vcm_videoAttrs_t;


typedef struct vcm_audioAttrs_t_ {
  cc_uint16_t packetization_period; 
  cc_uint16_t max_packetization_period; 
  cc_int32_t avt_payload_type; 
  vcm_vad_t vad; 
  vcm_mixing_party_t mixing_party; 
  vcm_mixing_mode_t  mixing_mode;  
} vcm_audioAttrs_t;






typedef struct vcm_attrs_t_ {
  cc_boolean         mute;
  cc_boolean         is_video;
  cc_boolean         rtcp_mux;
  cc_boolean         audio_level;
  uint8_t            audio_level_id;
  vcm_audioAttrs_t audio; 
  vcm_videoAttrs_t video; 
  uint32_t bundle_level; 
  
  

  cc_uint32_t        bundle_stream_correlator;
  cc_uint32_t        ssrcs[MAX_SSRCS_PER_MEDIA_LINE];
  cc_uint8_t         num_ssrcs;
  

  cc_uint8_t         unique_payload_types[MAX_PTS_PER_MEDIA_LINE];
  cc_uint8_t         num_unique_payload_types;
} vcm_mediaAttrs_t;


#ifdef __cplusplus
extern "C" {
#endif






void vcmInit();




void vcmUnload();




















short vcmRxOpen(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t call_handle,
        cc_uint16_t port_requested,
        cpr_ip_addr_t *listen_ip,
        cc_boolean is_multicast,
        int *port_allocated);












short vcmTxOpen(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t call_handle);















void vcmRxAllocPort(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        cc_uint16_t port_requested,
        int *port_allocated);


short vcmRxAllocICE(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        const char *peerconnection,
        uint16_t level,
        char **default_addr, 
        int *default_port, 
        char ***candidates, 
        int *candidate_ct 
);










short vcmGetIceParams(const char *peerconnection, char **ufragp, char **pwdp);










short vcmSetIceSessionParams(const char *peerconnection, char *ufrag, char *pwd,
                             cc_boolean icelite);









short vcmSetIceCandidate(const char *peerconnection, const char *icecandidate, uint16_t level);











short vcmSetIceMediaParams(const char *peerconnection, int level, char *ufrag, char *pwd,
                      char **candidates, int candidate_ct);






short vcmStartIceChecks(const char *peerconnection, cc_boolean isControlling);















short vcmCreateRemoteStream(
             cc_mcapid_t mcap_id,
             const char *peerconnection,
             int *pc_stream_id);













short vcmAddRemoteStreamHint(
            const char *peerconnection,
            int pc_stream_id,
            cc_boolean is_video);











void vcmRxReleasePort(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        int port);

























int vcmRxStart(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        const vcm_payload_info_t *payload,
        cpr_ip_addr_t *local_addr,
        cc_uint16_t port,
        vcm_crypto_algorithmID algorithmID,
        vcm_crypto_key_t *rx_key,
        vcm_mediaAttrs_t *attrs);


struct cc_media_remote_track_table_t_;
typedef struct cc_media_remote_track_table_t_ vcm_media_remote_track_table_t;

void vcmOnRemoteStreamAdded(cc_call_handle_t call_handle,
                            const char* peer_connection_handle,
                            vcm_media_remote_track_table_t *media_tracks);
























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
        vcm_mediaAttrs_t *attrs);




























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
        vcm_mediaAttrs_t *attrs);

























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
        vcm_mediaAttrs_t *attrs);


  short vcmGetDtlsIdentity(const char *peerconnection,
        char *digest_alg,
        size_t max_digest_alg_len,
        char *digest,
        size_t max_digest_len);


  short vcmInitializeDataChannel(const char *peerconnection,
        int track_id,
        cc_uint16_t streams,
        int local_datachannel_port,
        int remote_datachannel_port,
        const char* protocol);













short vcmRxClose(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle);












short vcmTxClose(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle);











void vcmEnableSidetone(cc_uint16_t side_tone);
















void vcmToneStart(vcm_tones_t tone,
        short alert_info,
        cc_call_handle_t  call_handle,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_uint16_t direction);















void vcmToneStartWithSpeakerAsBackup(vcm_tones_t tone,
        short alert_info,
        cc_call_handle_t call_handle,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_uint16_t direction);
















void vcmToneStop(vcm_tones_t tone,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t call_handle);












void vcmControlRinger(vcm_ring_mode_t ringMode,
        short once,
        cc_boolean alert_info,
        int line,
        cc_callid_t call_id);














int vcmGetAudioCodecList(int request_type);













int vcmGetVideoCodecList(int request_type);







int vcmGetH264SupportedPacketizationModes();





uint32_t vcmGetVideoH264ProfileLevelID();



























int vcmGetRtpStats(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t call_handle,
        char *rx_stats,
        char *tx_stats);















cc_boolean vcmAllocateBandwidth(cc_call_handle_t call_handle, int sessions);









void vcmRemoveBandwidth(cc_call_handle_t call_handle);










void vcmActivateWlan(cc_boolean is_active);








void vcmFreeMediaPtr(void *ptr);












void vcmMediaControl(cc_call_handle_t call_handle, vcm_media_control_to_encoder_t to_encoder);










void vcmSetRtcpDscp(cc_groupid_t group_id, int dscp);




















cc_boolean vcmCheckAttribs(cc_uint32_t media_type, void *sdp_p, int level,
                           int remote_pt, void **rcapptr);

















void vcmPopulateAttribs(void *sdp_p, int level, cc_uint32_t media_type,
                          cc_uint16_t payload_number, cc_boolean isOffer);













int vcmDtmfBurst(int digit, int duration, int direction);








int vcmGetILBCMode();








int vcmOnSdpParseError(const char *peercconnection, const char *message);







int vcmDisableRtcpComponent(const char *peerconnection, int level);

short vcmGetVideoLevel(uint16_t codec, int32_t *level);
short vcmGetVideoMaxFs(uint16_t codec, int32_t *max_fs);
short vcmGetVideoMaxFr(uint16_t codec, int32_t *max_fr);
short vcmGetVideoMaxBr(uint16_t codec, int32_t *max_br);
short vcmGetVideoMaxMbps(uint16_t codec, int32_t *max_mbps);
short vcmGetVideoPreferredCodec(int32_t *preferred_codec);


#ifdef __cplusplus
}
#endif




#endif
