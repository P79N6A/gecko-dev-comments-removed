



#ifndef _CCSIP_SDP_H_
#define _CCSIP_SDP_H_


#include "cpr_types.h"
#include "pmhutils.h"
#include "sdp.h"
#include "ccapi.h"
#include "mozilla-config.h"


#define CCSIP_SRC_SDP_BIT       0x1
#define CCSIP_DEST_SDP_BIT      0x2





PMH_EXTERN boolean sip_sdp_init(void);
PMH_EXTERN sdp_t *sipsdp_create(const char *peerconnection);
PMH_EXTERN cc_sdp_t *sipsdp_info_create(void);
PMH_EXTERN void sipsdp_src_dest_free(uint16_t flags, cc_sdp_t **sdp_info);
PMH_EXTERN void sipsdp_src_dest_create(const char *peerconnection,
    uint16_t flags, cc_sdp_t **sdp_info);
PMH_EXTERN void sipsdp_free(cc_sdp_t **sip_sdp);










#define SIPSDP_MAX_SESSION_VERSION_LENGTH 32




#define SIPSDP_VERSION              0

#define SIPSDP_ORIGIN_USERNAME      "Mozilla-SIPUA-" MOZ_APP_UA_VERSION
#define SIPSDP_SESSION_NAME         "SIP Call"


#define SIPSDP_ATTR_ENCNAME_PCMU      "PCMU"
#define SIPSDP_ATTR_ENCNAME_PCMA      "PCMA"
#define SIPSDP_ATTR_ENCNAME_G729      "G729"
#define SIPSDP_ATTR_ENCNAME_G723      "G723"
#define SIPSDP_ATTR_ENCNAME_G726      "G726-32"
#define SIPSDP_ATTR_ENCNAME_G728      "G728"
#define SIPSDP_ATTR_ENCNAME_GSM       "GSM"
#define SIPSDP_ATTR_ENCNAME_CN        "CN"
#define SIPSDP_ATTR_ENCNAME_G722      "G722"
#define SIPSDP_ATTR_ENCNAME_ILBC      "iLBC"
#define SIPSDP_ATTR_ENCNAME_H263v2    "H263-1998"
#define SIPSDP_ATTR_ENCNAME_H264      "H264"
#define SIPSDP_ATTR_ENCNAME_VP8       "VP8"
#define SIPSDP_ATTR_ENCNAME_L16_256K  "L16"
#define SIPSDP_ATTR_ENCNAME_ISAC      "ISAC"
#define SIPSDP_ATTR_ENCNAME_OPUS      "opus"


#define SIPSDP_ATTR_ENCNAME_TEL_EVENT "telephone-event"
#define SIPSDP_ATTR_ENCNAME_FRF_DIGIT "frf-dialed-digit"


#define SIPSDP_ATTR_ENCNAME_CLEAR_CH  "X-CCD"
#define SIPSDP_ATTR_ENCNAME_G726R16   "G726-16"
#define SIPSDP_ATTR_ENCNAME_G726R24   "G726-24"
#define SIPSDP_ATTR_ENCNAME_GSMEFR    "GSM-EFR"






#define SIPSDP_ATTR_ENCNAME_G729_A_STR_DOTTED                 "G.729a"
#define SIPSDP_ATTR_ENCNAME_G729_B_STR_DOTTED                 "G.729b"
#define SIPSDP_ATTR_ENCNAME_G729_B_LOW_COMPLEXITY_STR_DOTTED  "G.729b-L"
#define SIPSDP_ATTR_ENCNAME_G729_A_B_STR_DOTTED               "G.729ab"

#define SIPSDP_ATTR_ENCNAME_G7231_HIGH_RATE_STR_DOTTED        "G.723.1-H"
#define SIPSDP_ATTR_ENCNAME_G7231_A_HIGH_RATE_STR_DOTTED      "G.723.1a-H"
#define SIPSDP_ATTR_ENCNAME_G7231_LOW_RATE_STR_DOTTED         "G.723.1-L"
#define SIPSDP_ATTR_ENCNAME_G7231_A_LOW_RATE_STR_DOTTED       "G.723.1a-L"




#define SIPSDP_ATTR_ENCNAME_XNSE       "X-NSE"
#define SIPSDP_ATTR_ENCNAME_NSE        "NSE"



#define RTPMAP_CLOCKRATE  8000
#define RTPMAP_VIDEO_CLOCKRATE  90000
#define RTPMAP_L16_CLOCKRATE  16000
#define RTPMAP_ISAC_CLOCKRATE  16000
#define RTPMAP_OPUS_CLOCKRATE      48000
#define FMTP_MAX_AVERAGE_BIT_RATE  40000
#define ATTR_PTIME                 20
#define ATTR_MAXPTIME              120
#define WEBRTC_DATA_CHANNEL_PROT   "webrtc-datachannel"

#define SIPSDP_CONTENT_TYPE         "application/sdp"

#define MAX_RTP_PAYLOAD_TYPES        7

#define BITRATE_5300_BPS  5300
#define BITRATE_6300_BPS  6300




#define SIPSDP_ATTR_T38_VERSION_DEF             0
#define SIPSDP_ATTR_T38_FILL_BIT_REMOVAL_DEF    FALSE
#define SIPSDP_ATTR_T38_TRANSCODING_MMR_DEF     FALSE
#define SIPSDP_ATTR_T38_TRANSCODING_JBIG_DEF    FALSE

#define SIPSDP_ATTR_T38_MAX_BUFFER_DEF          200
#define SIPSDP_ATTR_T38_MAX_DATAGRAM_DEF        72








#define SIPSDP_NTE_SUPPORTED_LOW      0  /* Min value of DTMF event table */
#define SIPSDP_FRF_SUPPORTED_HIGH     15 /* for dtmf-relay cisco-rtp */
#define SIPSDP_NTE_SUPPORTED_HIGH     16 /* for dtmf-relay rtp-nse */






PMH_EXTERN char *sipsdp_write_to_buf(sdp_t *, uint32_t *);

#define SIPSDP_FREE(x) \
if (x) \
{ \
    sdp_free_description(x); \
}

#define SIPSDP_MAX_PAYLOAD_TYPES 15
#define MAX_RTP_MEDIA_TYPES   6
#define SIPSDP_NTE_DTMF_MIN   0  /* Min value of DTMF event table */
#define SIPSDP_NTE_DTMF_MAX  15  /* Max DTMF event value supported here */


#endif 
