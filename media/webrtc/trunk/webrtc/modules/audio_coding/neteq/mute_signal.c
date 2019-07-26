













#include "dsp_helpfunctions.h"

#include "signal_processing_library.h"

void WebRtcNetEQ_MuteSignal(int16_t *pw16_inout, int16_t muteSlope,
                            int16_t N)
{
    int i;
    int32_t w32_tmp = 1048608; 

    for (i = 0; i < N; i++)
    {
        pw16_inout[i]
            = (int16_t) ((WEBRTC_SPL_MUL_16_16((int16_t)(w32_tmp>>6), pw16_inout[i])
                + 8192) >> 14);
        w32_tmp -= muteSlope;
    }
}

