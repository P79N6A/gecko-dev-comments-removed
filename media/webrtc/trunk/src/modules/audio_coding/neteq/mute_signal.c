













#include "dsp_helpfunctions.h"

#include "signal_processing_library.h"

void WebRtcNetEQ_MuteSignal(WebRtc_Word16 *pw16_inout, WebRtc_Word16 muteSlope,
                            WebRtc_Word16 N)
{
    int i;
    WebRtc_Word32 w32_tmp = 1048608; 

    for (i = 0; i < N; i++)
    {
        pw16_inout[i]
            = (WebRtc_Word16) ((WEBRTC_SPL_MUL_16_16((WebRtc_Word16)(w32_tmp>>6), pw16_inout[i])
                + 8192) >> 14);
        w32_tmp -= muteSlope;
    }
}

