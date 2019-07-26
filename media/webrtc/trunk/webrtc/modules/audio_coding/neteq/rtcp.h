













#ifndef RTCP_H
#define RTCP_H

#include "typedefs.h"

typedef struct
{
    uint16_t cycles; 
    uint16_t max_seq; 

    uint16_t base_seq; 
    uint32_t received; 
    uint32_t rec_prior; 
    uint32_t exp_prior; 

    uint32_t jitter; 
    int32_t transit; 
} WebRtcNetEQ_RTCP_t;
















int WebRtcNetEQ_RTCPInit(WebRtcNetEQ_RTCP_t *RTCP_inst, uint16_t uw16_seqNo);


















int WebRtcNetEQ_RTCPUpdate(WebRtcNetEQ_RTCP_t *RTCP_inst, uint16_t uw16_seqNo,
                           uint32_t uw32_timeStamp, uint32_t uw32_recTime);



























int WebRtcNetEQ_RTCPGetStats(WebRtcNetEQ_RTCP_t *RTCP_inst,
                             uint16_t *puw16_fraction_lost,
                             uint32_t *puw32_cum_lost, uint32_t *puw32_ext_max,
                             uint32_t *puw32_jitter, int16_t doNotReset);

#endif
