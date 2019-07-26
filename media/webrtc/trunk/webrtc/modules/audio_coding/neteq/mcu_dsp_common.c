













#include "mcu_dsp_common.h"

#include <string.h>


int WebRtcNetEQ_DSPinit(MainInst_t *inst)
{
    int res = 0;

    res |= WebRtcNetEQ_AddressInit(&inst->DSPinst, NULL, NULL, inst);
    res |= WebRtcNetEQ_McuAddressInit(&inst->MCUinst, NULL, NULL, inst);

    return res;

}


int WebRtcNetEQ_DSP2MCUinterrupt(MainInst_t *inst, int16_t *pw16_shared_mem)
{
    inst->MCUinst.pw16_readAddress = pw16_shared_mem;
    inst->MCUinst.pw16_writeAddress = pw16_shared_mem;
    return WebRtcNetEQ_SignalMcu(&inst->MCUinst);
}

int WebRtcNetEQ_IsSyncPayload(const void* payload, int payload_len_bytes) {
  if (payload_len_bytes != SYNC_PAYLOAD_LEN_BYTES ||
      memcmp(payload, kSyncPayload, SYNC_PAYLOAD_LEN_BYTES) != 0) {
    return 0;
  }
  return 1;
}
