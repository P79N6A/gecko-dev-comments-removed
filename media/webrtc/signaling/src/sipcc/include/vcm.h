


















#ifndef _VCM_H_
#define _VCM_H_

#include "cpr_types.h"
#include "cc_constants.h"






#define GET_DYNAMIC_PAY_LOAD_TYPE(payload) (payload>>16)

#define GET_MEDIA_PAYLOAD_TYPE_ENUM(payload) (payload & 0xFFFF)


#define CC_IS_AUDIO(id) ((id == CC_AUDIO_1) ? TRUE:FALSE)

#define CC_IS_VIDEO(id) ((id == CC_VIDEO_1) ? TRUE:FALSE)



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

#define CC_KFACTOR_STAT_LEN   (256)







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





typedef enum
{
    VCM_Media_Payload_NonStandard = 1,
    VCM_Media_Payload_G711Alaw64k = 2,
    VCM_Media_Payload_G711Alaw56k = 3, 
    VCM_Media_Payload_G711Ulaw64k = 4,
    VCM_Media_Payload_G711Ulaw56k = 5, 
    VCM_Media_Payload_G722_64k = 6,
    VCM_Media_Payload_G722_56k = 7,
    VCM_Media_Payload_G722_48k = 8,
    VCM_Media_Payload_G7231 = 9,
    VCM_Media_Payload_G728 = 10,
    VCM_Media_Payload_G729 = 11,
    VCM_Media_Payload_G729AnnexA = 12,
    VCM_Media_Payload_Is11172AudioCap = 13,
    VCM_Media_Payload_Is13818AudioCap = 14,
    VCM_Media_Payload_G729AnnexB = 15,
    VCM_Media_Payload_G729AnnexAwAnnexB = 16,
    VCM_Media_Payload_GSM_Full_Rate = 18,
    VCM_Media_Payload_GSM_Half_Rate = 19,
    VCM_Media_Payload_GSM_Enhanced_Full_Rate = 20,
    VCM_Media_Payload_Wide_Band_256k = 25,
    VCM_Media_Payload_H263 = 31,
    VCM_Media_Payload_H264 = 34,
    VCM_Media_Payload_Data64 = 32,
    VCM_Media_Payload_Data56 = 33,
    VCM_Media_Payload_ILBC20 = 39,
    VCM_Media_Payload_ILBC30 = 40,
    VCM_Media_Payload_ISAC = 41,
    VCM_Media_Payload_GSM = 80,
    VCM_Media_Payload_ActiveVoice = 81,
    VCM_Media_Payload_G726_32K = 82,
    VCM_Media_Payload_G726_24K = 83,
    VCM_Media_Payload_G726_16K = 84,
    VCM_Media_Payload_OPUS = 109,
    VCM_Media_Payload_VP8 = 120,
    VCM_Media_Payload_I420 = 124,
    VCM_Media_Payload_Max           
} vcm_media_payload_type_t;





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
  vcm_audioAttrs_t audio; 
  vcm_videoAttrs_t video; 
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


void vcmRxAllocICE(cc_mcapid_t mcap_id,
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










void vcmGetIceParams(const char *peerconnection, char **ufragp, char **pwdp);









short vcmSetIceSessionParams(const char *peerconnection, char *ufrag, char *pwd);









short vcmSetIceCandidate(const char *peerconnection, const char *icecandidate, uint16_t level);











short vcmSetIceMediaParams(const char *peerconnection, int level, char *ufrag, char *pwd,
                      char **candidates, int candidate_ct);






short vcmStartIceChecks(const char *peerconnection);















short vcmCreateRemoteStream(
             cc_mcapid_t mcap_id,
             const char *peerconnection,
             int *pc_stream_id,
             vcm_media_payload_type_t payload);











void vcmRxReleasePort(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        int port);

























int vcmRxStart(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle,
        vcm_media_payload_type_t payload,
        cpr_ip_addr_t *local_addr,
        cc_uint16_t port,
        vcm_crypto_algorithmID algorithmID,
        vcm_crypto_key_t *rx_key,
        vcm_mediaAttrs_t *attrs);
























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
        vcm_mediaAttrs_t *attrs);




























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
        vcm_mediaAttrs_t *attrs);
























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
        vcm_mediaAttrs_t *attrs);


  short vcmGetDtlsIdentity(const char *peerconnection,
        char *digest_alg,
        size_t max_digest_alg_len,
        char *digest,
        size_t max_digest_len);


  short vcmSetDataChannelParameters(const char *peerconnection,
        cc_uint16_t streams,
        int sctp_port,
        const char* protocol);













void vcmRxClose(cc_mcapid_t mcap_id,
        cc_groupid_t group_id,
        cc_streamid_t stream_id,
        cc_call_handle_t  call_handle);












void vcmTxClose(cc_mcapid_t mcap_id,
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






int vcmGetVideoMaxSupportedPacketizationMode();



























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



















cc_boolean vcmCheckAttribs(cc_uint32_t media_type, void *sdp_p, int level, void **rcapptr);

















void vcmPopulateAttribs(void *sdp_p, int level, cc_uint32_t media_type,
                          cc_uint16_t payload_number, cc_boolean isOffer);













int vcmDtmfBurst(int digit, int duration, int direction);








int vcmGetILBCMode();


#ifdef __cplusplus
}
#endif




#endif
