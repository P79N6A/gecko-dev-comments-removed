













#include "dsp_helpfunctions.h"

#include "signal_processing_library.h"


void WebRtcNetEQ_UnmuteSignal(WebRtc_Word16 *pw16_inVec, WebRtc_Word16 *startMuteFact,
                              WebRtc_Word16 *pw16_outVec, WebRtc_Word16 unmuteFact,
                              WebRtc_Word16 N)
{
    int i;
    WebRtc_UWord16 w16_tmp;
    WebRtc_Word32 w32_tmp;

    w16_tmp = (WebRtc_UWord16) *startMuteFact;
    w32_tmp = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)w16_tmp,6) + 32;
    for (i = 0; i < N; i++)
    {
        pw16_outVec[i]
            = (WebRtc_Word16) ((WEBRTC_SPL_MUL_16_16(w16_tmp, pw16_inVec[i]) + 8192) >> 14);
        w32_tmp += unmuteFact;
        w32_tmp = WEBRTC_SPL_MAX(0, w32_tmp);
        w16_tmp = (WebRtc_UWord16) WEBRTC_SPL_RSHIFT_W32(w32_tmp, 6); 
        w16_tmp = WEBRTC_SPL_MIN(16384, w16_tmp);
    }
    *startMuteFact = (WebRtc_Word16) w16_tmp;
}

