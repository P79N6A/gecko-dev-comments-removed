
















#include "signal_processing_library.h"

int WebRtcSpl_GetScalingSquare(WebRtc_Word16 *in_vector, int in_vector_length, int times)
{
    int nbits = WebRtcSpl_GetSizeInBits(times);
    int i;
    WebRtc_Word16 smax = -1;
    WebRtc_Word16 sabs;
    WebRtc_Word16 *sptr = in_vector;
    int t;
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
