









#include "mcu.h"

#include <string.h> 




int WebRtcNetEQ_McuAddressInit(MCUInst_t *inst, void * Data2McuAddress,
                               void * Data2DspAddress, void *main_inst)
{

    inst->pw16_readAddress = (WebRtc_Word16*) Data2McuAddress;
    inst->pw16_writeAddress = (WebRtc_Word16*) Data2DspAddress;
    inst->main_inst = main_inst;

    inst->millisecondsPerCall = 10;

    
    if (inst->pw16_writeAddress != NULL) inst->pw16_writeAddress[0] = DSP_INSTR_EXPAND;

    return (0);
}

