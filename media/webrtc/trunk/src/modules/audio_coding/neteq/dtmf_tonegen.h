













#ifndef DTMF_TONEGEN_H
#define DTMF_TONEGEN_H

#include "typedefs.h"

#include "neteq_defines.h"

#ifdef NETEQ_ATEVENT_DECODE


#define DTMF_AMP_LOW	23171	/* 3 dB lower than the high frequency */


typedef struct dtmf_tone_inst_t_
{

    WebRtc_Word16 reinit; 

    WebRtc_Word16 oldOutputLow[2]; 
    WebRtc_Word16 oldOutputHigh[2]; 

    int lastDtmfSample; 

}dtmf_tone_inst_t;























WebRtc_Word16 WebRtcNetEQ_DTMFGenerate(dtmf_tone_inst_t *DTMFdecInst,
                WebRtc_Word16 value,
                WebRtc_Word16 volume,
                WebRtc_Word16 *signal,
                WebRtc_UWord16 sampFreq,
                WebRtc_Word16 frameLen
);

#endif 

#endif 

