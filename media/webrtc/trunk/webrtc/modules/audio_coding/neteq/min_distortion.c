













#include "dsp_helpfunctions.h"

#include "signal_processing_library.h"

WebRtc_Word16 WebRtcNetEQ_MinDistortion(const WebRtc_Word16 *pw16_data,
                                        WebRtc_Word16 w16_minLag, WebRtc_Word16 w16_maxLag,
                                        WebRtc_Word16 len, WebRtc_Word32 *pw16_dist)
{
    int i, j;
    const WebRtc_Word16 *pw16_data1;
    const WebRtc_Word16 *pw16_data2;
    WebRtc_Word32 w32_diff;
    WebRtc_Word32 w32_sumdiff;
    WebRtc_Word16 bestIndex = -1;
    WebRtc_Word32 minDist = WEBRTC_SPL_WORD32_MAX;

    for (i = w16_minLag; i <= w16_maxLag; i++)
    {
        w32_sumdiff = 0;
        pw16_data1 = pw16_data;
        pw16_data2 = pw16_data - i;

        for (j = 0; j < len; j++)
        {
            w32_diff = pw16_data1[j] - pw16_data2[j];
            w32_sumdiff += WEBRTC_SPL_ABS_W32(w32_diff);
        }

        
        if (w32_sumdiff < minDist)
        {
            minDist = w32_sumdiff;
            bestIndex = i;
        }
    }

    *pw16_dist = minDist;

    return bestIndex;
}

