













#ifndef DTMF_TONEGEN_H
#define DTMF_TONEGEN_H

#include "typedefs.h"

#include "neteq_defines.h"

#ifdef NETEQ_ATEVENT_DECODE


#define DTMF_AMP_LOW	23171	/* 3 dB lower than the high frequency */


typedef struct dtmf_tone_inst_t_
{

    int16_t reinit; 

    int16_t oldOutputLow[2]; 
    int16_t oldOutputHigh[2]; 

    int lastDtmfSample; 

}dtmf_tone_inst_t;























int16_t WebRtcNetEQ_DTMFGenerate(dtmf_tone_inst_t *DTMFdecInst,
                int16_t value,
                int16_t volume,
                int16_t *signal,
                uint16_t sampFreq,
                int16_t frameLen
);

#endif 

#endif 

