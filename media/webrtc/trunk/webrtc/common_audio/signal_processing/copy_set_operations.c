

























#include <string.h>
#include "signal_processing_library.h"


void WebRtcSpl_MemSetW16(int16_t *ptr, int16_t set_value, int length)
{
    int j;
    int16_t *arrptr = ptr;

    for (j = length; j > 0; j--)
    {
        *arrptr++ = set_value;
    }
}

void WebRtcSpl_MemSetW32(int32_t *ptr, int32_t set_value, int length)
{
    int j;
    int32_t *arrptr = ptr;

    for (j = length; j > 0; j--)
    {
        *arrptr++ = set_value;
    }
}

void WebRtcSpl_MemCpyReversedOrder(int16_t* dest, int16_t* source, int length)
{
    int j;
    int16_t* destPtr = dest;
    int16_t* sourcePtr = source;

    for (j = 0; j < length; j++)
    {
        *destPtr-- = *sourcePtr++;
    }
}

int16_t WebRtcSpl_CopyFromEndW16(const int16_t *vector_in,
                                 int16_t length,
                                 int16_t samples,
                                 int16_t *vector_out)
{
    
    WEBRTC_SPL_MEMCPY_W16(vector_out, &vector_in[length - samples], samples);

    return samples;
}

int16_t WebRtcSpl_ZerosArrayW16(int16_t *vector, int16_t length)
{
    WebRtcSpl_MemSetW16(vector, 0, length);
    return length;
}

int16_t WebRtcSpl_ZerosArrayW32(int32_t *vector, int16_t length)
{
    WebRtcSpl_MemSetW32(vector, 0, length);
    return length;
}

int16_t WebRtcSpl_OnesArrayW16(int16_t *vector, int16_t length)
{
    int16_t i;
    int16_t *tmpvec = vector;
    for (i = 0; i < length; i++)
    {
        *tmpvec++ = 1;
    }
    return length;
}

int16_t WebRtcSpl_OnesArrayW32(int32_t *vector, int16_t length)
{
    int16_t i;
    int32_t *tmpvec = vector;
    for (i = 0; i < length; i++)
    {
        *tmpvec++ = 1;
    }
    return length;
}
