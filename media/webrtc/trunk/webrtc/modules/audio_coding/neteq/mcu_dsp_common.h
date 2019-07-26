













#ifndef MCU_DSP_COMMON_H
#define MCU_DSP_COMMON_H

#include "typedefs.h"

#include "dsp.h"
#include "mcu.h"


#if defined(NETEQ_48KHZ_WIDEBAND)
    #define SHARED_MEM_SIZE (6*640)
#elif defined(NETEQ_32KHZ_WIDEBAND)
    #define SHARED_MEM_SIZE (4*640)
#elif defined(NETEQ_WIDEBAND)
    #define SHARED_MEM_SIZE (2*640)
#else
    #define SHARED_MEM_SIZE 640
#endif


typedef struct
{
    DSPInst_t DSPinst; 
    MCUInst_t MCUinst; 
    WebRtc_Word16 ErrorCode; 
#ifdef NETEQ_STEREO
    WebRtc_Word16 masterSlave; 
#endif 
} MainInst_t;


typedef struct
{
    WebRtc_UWord32 playedOutTS; 
    WebRtc_UWord16 samplesLeft; 
    WebRtc_Word16 MD; 
    WebRtc_Word16 lastMode; 
    WebRtc_Word16 frameLen; 
} DSP2MCU_info_t;


int WebRtcNetEQ_DSPinit(MainInst_t *inst);


int WebRtcNetEQ_DSP2MCUinterrupt(MainInst_t *inst, WebRtc_Word16 *pw16_shared_mem);

#endif
