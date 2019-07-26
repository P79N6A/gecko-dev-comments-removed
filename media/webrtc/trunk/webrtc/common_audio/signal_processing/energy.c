
















#include "signal_processing_library.h"

int32_t WebRtcSpl_Energy(int16_t* vector, int vector_length, int* scale_factor)
{
    int32_t en = 0;
    int i;
    int scaling = WebRtcSpl_GetScalingSquare(vector, vector_length, vector_length);
    int looptimes = vector_length;
    int16_t *vectorptr = vector;

    for (i = 0; i < looptimes; i++)
    {
        en += WEBRTC_SPL_MUL_16_16_RSFT(*vectorptr, *vectorptr, scaling);
        vectorptr++;
    }
    *scale_factor = scaling;

    return en;
}
