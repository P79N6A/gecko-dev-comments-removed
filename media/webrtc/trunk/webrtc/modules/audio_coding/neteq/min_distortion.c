













#include "dsp_helpfunctions.h"

#include "signal_processing_library.h"

int16_t WebRtcNetEQ_MinDistortion(const int16_t *pw16_data,
                                  int16_t w16_minLag, int16_t w16_maxLag,
                                  int16_t len, int32_t *pw16_dist)
{
    int i, j;
    const int16_t *pw16_data1;
    const int16_t *pw16_data2;
    int32_t w32_diff;
    int32_t w32_sumdiff;
    int16_t bestIndex = -1;
    int32_t minDist = WEBRTC_SPL_WORD32_MAX;

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

