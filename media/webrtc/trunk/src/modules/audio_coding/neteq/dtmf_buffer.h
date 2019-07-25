













#ifndef DTMF_BUFFER_H
#define DTMF_BUFFER_H

#include "typedefs.h"

#include "neteq_defines.h"


#ifdef NETEQ_ATEVENT_DECODE

#define MAX_DTMF_QUEUE_SIZE 4 

typedef struct dtmf_inst_t_
{
    WebRtc_Word16 MaxPLCtime;
    WebRtc_Word16 CurrentPLCtime;
    WebRtc_Word16 EventQueue[MAX_DTMF_QUEUE_SIZE];
    WebRtc_Word16 EventQueueVolume[MAX_DTMF_QUEUE_SIZE];
    WebRtc_Word16 EventQueueEnded[MAX_DTMF_QUEUE_SIZE];
    WebRtc_UWord32 EventQueueStartTime[MAX_DTMF_QUEUE_SIZE];
    WebRtc_UWord32 EventQueueEndTime[MAX_DTMF_QUEUE_SIZE];
    WebRtc_Word16 EventBufferSize;
    WebRtc_Word16 framelen;
} dtmf_inst_t;















WebRtc_Word16 WebRtcNetEQ_DtmfDecoderInit(dtmf_inst_t *DTMFdec_inst, WebRtc_UWord16 fs,
                                          WebRtc_Word16 MaxPLCtime);
















WebRtc_Word16 WebRtcNetEQ_DtmfInsertEvent(dtmf_inst_t *DTMFdec_inst,
                                          const WebRtc_Word16 *encoded, WebRtc_Word16 len,
                                          WebRtc_UWord32 timeStamp);




















WebRtc_Word16 WebRtcNetEQ_DtmfDecode(dtmf_inst_t *DTMFdec_inst, WebRtc_Word16 *event,
                                     WebRtc_Word16 *volume, WebRtc_UWord32 currTimeStamp);

#endif    

#endif    

