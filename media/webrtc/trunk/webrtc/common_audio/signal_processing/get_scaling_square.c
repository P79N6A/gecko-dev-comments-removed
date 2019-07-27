
















#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

int16_t WebRtcSpl_GetScalingSquare(int16_t* in_vector,
                                   int in_vector_length,
                                   int times)
{
    int16_t nbits = WebRtcSpl_GetSizeInBits(times);
    int i;
    int16_t smax = -1;
    int16_t sabs;
    int16_t *sptr = in_vector;
    int16_t t;
    int looptimes = in_vector_length;

    for (i = looptimes; i > 0; i--)
    {
        sabs = (*sptr > 0 ? *sptr++ : -*sptr++);
        smax = (sabs > smax ? sabs : smax);
    }
    t = WebRtcSpl_NormW32(WEBRTC_SPL_MUL(smax, smax));

    if (smax == 0)
    {
        return 0; 
    } else
    {
        return (t > nbits) ? 0 : nbits - t;
    }
}
