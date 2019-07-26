
















#include "signal_processing_library.h"

WebRtc_Word32 WebRtcSpl_Energy(WebRtc_Word16* vector, int vector_length, int* scale_factor)
{
    WebRtc_Word32 en = 0;
    int i;
    int scaling = WebRtcSpl_GetScalingSquare(vector, vector_length, vector_length);
    int looptimes = vector_length;
    WebRtc_Word16 *vectorptr = vector;

    for (i = 0; i < looptimes; i++)
    {
        en += WEBRTC_SPL_MUL_16_16_RSFT(*vectorptr, *vectorptr, scaling);
        vectorptr++;
    }
    *scale_factor = scaling;

    return en;
}
