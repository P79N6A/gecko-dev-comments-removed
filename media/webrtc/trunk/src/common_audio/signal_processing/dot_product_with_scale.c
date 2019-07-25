
















#include "signal_processing_library.h"

WebRtc_Word32 WebRtcSpl_DotProductWithScale(WebRtc_Word16 *vector1, WebRtc_Word16 *vector2,
                                            int length, int scaling)
{
    WebRtc_Word32 sum;
    int i;
#ifdef _ARM_OPT_
#pragma message("NOTE: _ARM_OPT_ optimizations are used")
    WebRtc_Word16 len4 = (length >> 2) << 2;
#endif

    sum = 0;

#ifndef _ARM_OPT_
    for (i = 0; i < length; i++)
    {
        sum += WEBRTC_SPL_MUL_16_16_RSFT(*vector1++, *vector2++, scaling);
    }
#else
    if (scaling == 0)
    {
        for (i = 0; i < len4; i = i + 4)
        {
            sum += WEBRTC_SPL_MUL_16_16(*vector1, *vector2);
            vector1++;
            vector2++;
            sum += WEBRTC_SPL_MUL_16_16(*vector1, *vector2);
            vector1++;
            vector2++;
            sum += WEBRTC_SPL_MUL_16_16(*vector1, *vector2);
            vector1++;
            vector2++;
            sum += WEBRTC_SPL_MUL_16_16(*vector1, *vector2);
            vector1++;
            vector2++;
        }

        for (i = len4; i < length; i++)
        {
            sum += WEBRTC_SPL_MUL_16_16(*vector1, *vector2);
            vector1++;
            vector2++;
        }
    }
    else
    {
        for (i = 0; i < len4; i = i + 4)
        {
            sum += WEBRTC_SPL_MUL_16_16_RSFT(*vector1, *vector2, scaling);
            vector1++;
            vector2++;
            sum += WEBRTC_SPL_MUL_16_16_RSFT(*vector1, *vector2, scaling);
            vector1++;
            vector2++;
            sum += WEBRTC_SPL_MUL_16_16_RSFT(*vector1, *vector2, scaling);
            vector1++;
            vector2++;
            sum += WEBRTC_SPL_MUL_16_16_RSFT(*vector1, *vector2, scaling);
            vector1++;
            vector2++;
        }

        for (i = len4; i < length; i++)
        {
            sum += WEBRTC_SPL_MUL_16_16_RSFT(*vector1, *vector2, scaling);
            vector1++;
            vector2++;
        }
    }
#endif

    return sum;
}
