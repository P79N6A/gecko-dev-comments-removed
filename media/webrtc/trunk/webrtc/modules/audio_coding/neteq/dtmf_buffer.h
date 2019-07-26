













#ifndef DTMF_BUFFER_H
#define DTMF_BUFFER_H

#include "typedefs.h"

#include "neteq_defines.h"


#ifdef NETEQ_ATEVENT_DECODE

#define MAX_DTMF_QUEUE_SIZE 4 

typedef struct dtmf_inst_t_
{
    int16_t MaxPLCtime;
    int16_t CurrentPLCtime;
    int16_t EventQueue[MAX_DTMF_QUEUE_SIZE];
    int16_t EventQueueVolume[MAX_DTMF_QUEUE_SIZE];
    int16_t EventQueueEnded[MAX_DTMF_QUEUE_SIZE];
    uint32_t EventQueueStartTime[MAX_DTMF_QUEUE_SIZE];
    uint32_t EventQueueEndTime[MAX_DTMF_QUEUE_SIZE];
    int16_t EventBufferSize;
    int16_t framelen;
} dtmf_inst_t;















int16_t WebRtcNetEQ_DtmfDecoderInit(dtmf_inst_t *DTMFdec_inst, uint16_t fs,
                                    int16_t MaxPLCtime);
















int16_t WebRtcNetEQ_DtmfInsertEvent(dtmf_inst_t *DTMFdec_inst,
                                    const int16_t *encoded, int16_t len,
                                    uint32_t timeStamp);




















int16_t WebRtcNetEQ_DtmfDecode(dtmf_inst_t *DTMFdec_inst, int16_t *event,
                               int16_t *volume, uint32_t currTimeStamp);

#endif    

#endif    

