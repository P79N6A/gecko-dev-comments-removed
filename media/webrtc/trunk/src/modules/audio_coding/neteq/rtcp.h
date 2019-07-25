













#ifndef RTCP_H
#define RTCP_H

#include "typedefs.h"

typedef struct
{
    WebRtc_UWord16 cycles; 
    WebRtc_UWord16 max_seq; 

    WebRtc_UWord16 base_seq; 
    WebRtc_UWord32 received; 
    WebRtc_UWord32 rec_prior; 
    WebRtc_UWord32 exp_prior; 

    WebRtc_UWord32 jitter; 
    WebRtc_Word32 transit; 
} WebRtcNetEQ_RTCP_t;
















int WebRtcNetEQ_RTCPInit(WebRtcNetEQ_RTCP_t *RTCP_inst, WebRtc_UWord16 uw16_seqNo);


















int WebRtcNetEQ_RTCPUpdate(WebRtcNetEQ_RTCP_t *RTCP_inst, WebRtc_UWord16 uw16_seqNo,
                           WebRtc_UWord32 uw32_timeStamp, WebRtc_UWord32 uw32_recTime);



























int WebRtcNetEQ_RTCPGetStats(WebRtcNetEQ_RTCP_t *RTCP_inst,
                             WebRtc_UWord16 *puw16_fraction_lost,
                             WebRtc_UWord32 *puw32_cum_lost, WebRtc_UWord32 *puw32_ext_max,
                             WebRtc_UWord32 *puw32_jitter, WebRtc_Word16 doNotReset);

#endif
