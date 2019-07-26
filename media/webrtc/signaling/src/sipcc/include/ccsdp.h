















































































#ifndef __CCSDP_H__
#define __CCSDP_H__

#include "cpr_types.h"

#define SIPSDP_ILBC_MODE20 20




typedef enum rtp_ptype_
{
    RTP_NONE         = -1,
    RTP_PCMU         = 0,
    RTP_CELP         = 1,
    RTP_G726         = 2,
    RTP_GSM          = 3,
    RTP_G723         = 4,
    RTP_DVI4         = 5,
    RTP_DVI4_II      = 6,
    RTP_LPC          = 7,
    RTP_PCMA         = 8,
    RTP_G722         = 9,
    RTP_G728         = 15,
    RTP_G729         = 18,
    RTP_JPEG         = 26,
    RTP_NV           = 28,
    RTP_H261         = 31,
    RTP_H264_P0      = 97,
    RTP_H264_P1      = 126,
    RTP_AVT          = 101,
    RTP_L16          = 102,
    RTP_H263         = 103,
    RTP_ILBC         = 116, 
    RTP_OPUS         = 109,
    RTP_VP8          = 120,
    RTP_I420         = 124,
    RTP_ISAC         = 124
} rtp_ptype;

typedef struct {
    const char *name;
    int         value;
} ccsdp_key_table_entry_t;

typedef enum max_coded_audio_bandwidth_ {
    opus_nb  = 0,    
    opus_mb  = 1,    
    opus_wb  = 2,    
    opus_swb = 3,    
    opus_fb  = 4   
} max_coded_audio_bandwidth;

static const ccsdp_key_table_entry_t max_coded_audio_bandwidth_table[] = {
    {"nb",         opus_nb},
    {"mb",         opus_mb},
    {"wb",         opus_wb},
    {"swb",        opus_swb},
    {"fb",         opus_fb}
};

typedef enum {
    SDP_SUCCESS, 
    SDP_FAILURE,
    SDP_INVALID_SDP_PTR,
    SDP_NOT_SDP_DESCRIPTION,
    SDP_INVALID_TOKEN_ORDERING,
    SDP_INVALID_PARAMETER,
    SDP_INVALID_MEDIA_LEVEL,
    SDP_INVALID_CAPABILITY,
    SDP_NO_RESOURCE,
    SDP_UNRECOGNIZED_TOKEN,
    SDP_NULL_BUF_PTR,
    SDP_POTENTIAL_SDP_OVERFLOW,
    SDP_MAX_RC
} sdp_result_e;




#define SDP_INVALID_VALUE          (-2)




typedef enum {
    SDP_BW_MODIFIER_INVALID = -1,
    SDP_BW_MODIFIER_AS, 
    SDP_BW_MODIFIER_CT, 
    SDP_BW_MODIFIER_TIAS, 
    SDP_MAX_BW_MODIFIER_VAL,
    SDP_BW_MODIFIER_UNSUPPORTED
} sdp_bw_modifier_e;





typedef enum {
    SDP_ATTR_BEARER = 0,
    SDP_ATTR_CALLED,
    SDP_ATTR_CONN_TYPE,
    SDP_ATTR_DIALED,
    SDP_ATTR_DIALING,
    SDP_ATTR_DIRECTION,
    SDP_ATTR_EECID,
    SDP_ATTR_FMTP,
    SDP_ATTR_FRAMING,
    SDP_ATTR_INACTIVE,
    SDP_ATTR_PTIME,
    SDP_ATTR_QOS,
    SDP_ATTR_CURR,
    SDP_ATTR_DES,
    SDP_ATTR_CONF,
    SDP_ATTR_RECVONLY,
    SDP_ATTR_RTPMAP,
    SDP_ATTR_SECURE,
    SDP_ATTR_SENDONLY,
    SDP_ATTR_SENDRECV,
    SDP_ATTR_SUBNET,
    SDP_ATTR_T38_VERSION,
    SDP_ATTR_T38_MAXBITRATE,
    SDP_ATTR_T38_FILLBITREMOVAL,
    SDP_ATTR_T38_TRANSCODINGMMR,
    SDP_ATTR_T38_TRANSCODINGJBIG,
    SDP_ATTR_T38_RATEMGMT,
    SDP_ATTR_T38_MAXBUFFER,
    SDP_ATTR_T38_MAXDGRAM,
    SDP_ATTR_T38_UDPEC,
    SDP_ATTR_X_CAP,
    SDP_ATTR_X_CPAR,
    SDP_ATTR_X_PC_CODEC,
    SDP_ATTR_X_PC_QOS,
    SDP_ATTR_X_QOS,
    SDP_ATTR_X_SQN,
    SDP_ATTR_TMRGWXID,
    SDP_ATTR_TC1_PAYLOAD_BYTES,
    SDP_ATTR_TC1_WINDOW_SIZE,
    SDP_ATTR_TC2_PAYLOAD_BYTES,
    SDP_ATTR_TC2_WINDOW_SIZE,
    SDP_ATTR_RTCP,
    SDP_ATTR_RTR,
    SDP_ATTR_SILENCESUPP,
    SDP_ATTR_SRTP_CONTEXT, 
    SDP_ATTR_MPTIME,
    SDP_ATTR_X_SIDIN,
    SDP_ATTR_X_SIDOUT,
    SDP_ATTR_X_CONFID,
    SDP_ATTR_GROUP,
    SDP_ATTR_MID,
    SDP_ATTR_SOURCE_FILTER,
    SDP_ATTR_RTCP_UNICAST,
    SDP_ATTR_MAXPRATE,
    SDP_ATTR_SQN,
    SDP_ATTR_CDSC,
    SDP_ATTR_CPAR,
    SDP_ATTR_SPRTMAP,
    SDP_ATTR_SDESCRIPTIONS,  
    SDP_ATTR_LABEL,
    SDP_ATTR_FRAMERATE,
    SDP_ATTR_ICE_CANDIDATE,
    SDP_ATTR_ICE_UFRAG,
    SDP_ATTR_ICE_PWD,
    SDP_ATTR_RTCP_MUX,
    SDP_ATTR_DTLS_FINGERPRINT,
    SDP_ATTR_MAXPTIME,
    SDP_MAX_ATTR_TYPES,
    SDP_ATTR_INVALID
} sdp_attr_e;












const char* ccsdpAttrGetFmtpParamSets(void *sdp_handle, uint16_t level, 
                                            uint8_t cap_num, uint16_t inst_num);












sdp_result_e ccsdpAttrGetFmtpPackMode(void *sdp_handle, uint16_t level, 
                         uint8_t cap_num, uint16_t inst_num, uint16_t *val);











sdp_result_e ccsdpAttrGetFmtpLevelAsymmetryAllowed(void *sdp_handle, uint16_t level, 
                         uint8_t cap_num, uint16_t inst_num, uint16_t *val);












const char* ccsdpAttrGetFmtpProfileLevelId (void *sdp_handle, uint16_t level,
                                          uint8_t cap_num, uint16_t inst_num);














sdp_result_e ccsdpAttrGetFmtpMaxMbps (void *sdp_handle, uint16_t level,
                                uint8_t cap_num, uint16_t inst_num, uint32_t *val);













sdp_result_e ccsdpAttrGetFmtpMaxFs (void *sdp_handle, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint32_t *val);











sdp_result_e ccsdpAttrGetFmtpMaxCpb (void *sdp_handle, uint16_t level,
                                 uint8_t cap_num, uint16_t inst_num, uint32_t *val);












sdp_result_e ccsdpAttrGetFmtpMaxBr (void *sdp_handle, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint32_t* val);











int ccsdpGetBandwidthValue (void *sdp_handle, uint16_t level, uint16_t inst_num);
















sdp_result_e ccsdpAddNewAttr (void *sdp_handle, uint16_t level, uint8_t cap_num,
                               sdp_attr_e attr_type, uint16_t *inst_num);














sdp_result_e ccsdpAttrGetFmtpMaxDpb (void *sdp_handle, uint16_t level,
                               uint8_t cap_num, uint16_t inst_num, uint32_t *val);














sdp_result_e ccsdpAttrSetFmtpPayloadType (void *sdp_handle, uint16_t level,
                              uint8_t cap_num, uint16_t inst_num, uint16_t payload_num);














sdp_result_e ccsdpAttrSetFmtpPackMode (void *sdp_handle, uint16_t level,
                                          uint8_t cap_num, uint16_t inst_num, uint16_t pack_mode);













sdp_result_e ccsdpAttrSetFmtpLevelAsymmetryAllowed (void *sdp_handle, uint16_t level,
                                          uint8_t cap_num, uint16_t inst_num, uint16_t level_asymmetry_allowed);














sdp_result_e ccsdpAttrSetFmtpProfileLevelId (void *sdp_handle, uint16_t level,
                               uint8_t cap_num, uint16_t inst_num, const char *profile_level_id);














sdp_result_e ccsdpAttrSetFmtpParameterSets (void *sdp_handle, uint16_t level,
                                     uint8_t cap_num, uint16_t inst_num, const char *parameter_sets);















sdp_result_e ccsdpAttrSetFmtpMaxBr (void *sdp_handle, uint16_t level,
                              uint8_t cap_num, uint16_t inst_num, uint32_t max_br);














sdp_result_e ccsdpAttrSetFmtpMaxMbps (void *sdp_handle, uint16_t level,
                              uint8_t cap_num, uint16_t inst_num, uint32_t max_mbps);












sdp_result_e ccsdpAttrSetFmtpMaxFs (void *sdp_handle, uint16_t level,
                        uint8_t cap_num, uint16_t inst_num, uint32_t max_fs);












sdp_result_e ccsdpAttrSetFmtpMaxCpb (void *sdp_handle, uint16_t level,
                            uint8_t cap_num, uint16_t inst_num, uint32_t max_cpb);












sdp_result_e ccsdpAttrSetFmtpMaxDbp (void *sdp_handle, uint16_t level,
                                  uint8_t cap_num, uint16_t inst_num, uint32_t max_dpb);













sdp_result_e ccsdpAttrSetFmtpQcif  (void *sdp_handle, uint16_t level,
                             uint8_t cap_num, uint16_t inst_num, uint16_t qcif);













sdp_result_e ccsdpAttrSetFmtpSqcif  (void *sdp_handle, uint16_t level,
                            uint8_t cap_num, uint16_t inst_num, uint16_t sqcif);






































sdp_result_e ccsdpAddNewBandwidthLine (void *sdp_handle, uint16_t level, sdp_bw_modifier_e bw_modifier, uint16_t *inst_num);


















sdp_result_e ccsdpSetBandwidth (void *sdp_handle, uint16_t level, uint16_t inst_num,
                         sdp_bw_modifier_e bw_modifier, uint32_t bw_val);

#endif
