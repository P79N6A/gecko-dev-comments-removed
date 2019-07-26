













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

#define  SYNC_PAYLOAD_LEN_BYTES  7
static const uint8_t kSyncPayload[SYNC_PAYLOAD_LEN_BYTES] = {
    'a', 'v', 's', 'y', 'n', 'c', '\0' };


typedef struct
{
    DSPInst_t DSPinst; 
    MCUInst_t MCUinst; 
    int16_t ErrorCode; 
#ifdef NETEQ_STEREO
    int16_t masterSlave; 
#endif 
} MainInst_t;


typedef struct
{
    uint32_t playedOutTS; 
    uint16_t samplesLeft; 
    int16_t MD; 
    int16_t lastMode; 
    int16_t frameLen; 
} DSP2MCU_info_t;


int WebRtcNetEQ_DSPinit(MainInst_t *inst);


int WebRtcNetEQ_DSP2MCUinterrupt(MainInst_t *inst, int16_t *pw16_shared_mem);



int WebRtcNetEQ_IsSyncPayload(const void* payload, int payload_len_bytes);

#endif
