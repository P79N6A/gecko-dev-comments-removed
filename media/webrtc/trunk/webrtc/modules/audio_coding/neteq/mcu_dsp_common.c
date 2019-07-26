













#include "mcu_dsp_common.h"

#include <string.h>


int WebRtcNetEQ_DSPinit(MainInst_t *inst)
{
    int res = 0;

    res |= WebRtcNetEQ_AddressInit(&inst->DSPinst, NULL, NULL, inst);
    res |= WebRtcNetEQ_McuAddressInit(&inst->MCUinst, NULL, NULL, inst);

    return res;

}


int WebRtcNetEQ_DSP2MCUinterrupt(MainInst_t *inst, WebRtc_Word16 *pw16_shared_mem)
{
    inst->MCUinst.pw16_readAddress = pw16_shared_mem;
    inst->MCUinst.pw16_writeAddress = pw16_shared_mem;
    return WebRtcNetEQ_SignalMcu(&inst->MCUinst);
}
