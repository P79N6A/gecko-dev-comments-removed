
















#include "signal_processing_library.h"

int WebRtcSpl_AutoCorrelation(G_CONST WebRtc_Word16* in_vector,
                              int in_vector_length,
                              int order,
                              WebRtc_Word32* result,
                              int* scale)
{
    WebRtc_Word32 sum;
    int i, j;
    WebRtc_Word16 smax; 
    G_CONST WebRtc_Word16* xptr1;
    G_CONST WebRtc_Word16* xptr2;
    WebRtc_Word32* resultptr;
    int scaling = 0;

#ifdef _ARM_OPT_
#pragma message("NOTE: _ARM_OPT_ optimizations are used")
    WebRtc_Word16 loops4;
#endif

    if (order < 0)
        order = in_vector_length;

    
    smax = WebRtcSpl_MaxAbsValueW16(in_vector, in_vector_length);

    
    

    if (smax == 0)
    {
        scaling = 0;
    } else
    {
        int nbits = WebRtcSpl_GetSizeInBits(in_vector_length); 
        int t = WebRtcSpl_NormW32(WEBRTC_SPL_MUL(smax, smax)); 

        if (t > nbits)
        {
            scaling = 0;
        } else
        {
            scaling = nbits - t;
        }

    }

    resultptr = result;

    
    for (i = 0; i < order + 1; i++)
    {
        int loops = (in_vector_length - i);
        sum = 0;
        xptr1 = in_vector;
        xptr2 = &in_vector[i];
#ifndef _ARM_OPT_
        for (j = loops; j > 0; j--)
        {
            sum += WEBRTC_SPL_MUL_16_16_RSFT(*xptr1++, *xptr2++, scaling);
        }
#else
        loops4 = (loops >> 2) << 2;

        if (scaling == 0)
        {
            for (j = 0; j < loops4; j = j + 4)
            {
                sum += WEBRTC_SPL_MUL_16_16(*xptr1, *xptr2);
                xptr1++;
                xptr2++;
                sum += WEBRTC_SPL_MUL_16_16(*xptr1, *xptr2);
                xptr1++;
                xptr2++;
                sum += WEBRTC_SPL_MUL_16_16(*xptr1, *xptr2);
                xptr1++;
                xptr2++;
                sum += WEBRTC_SPL_MUL_16_16(*xptr1, *xptr2);
                xptr1++;
                xptr2++;
            }

            for (j = loops4; j < loops; j++)
            {
                sum += WEBRTC_SPL_MUL_16_16(*xptr1, *xptr2);
                xptr1++;
                xptr2++;
            }
        }
        else
        {
            for (j = 0; j < loops4; j = j + 4)
            {
                sum += WEBRTC_SPL_MUL_16_16_RSFT(*xptr1, *xptr2, scaling);
                xptr1++;
                xptr2++;
                sum += WEBRTC_SPL_MUL_16_16_RSFT(*xptr1, *xptr2, scaling);
                xptr1++;
                xptr2++;
                sum += WEBRTC_SPL_MUL_16_16_RSFT(*xptr1, *xptr2, scaling);
                xptr1++;
                xptr2++;
                sum += WEBRTC_SPL_MUL_16_16_RSFT(*xptr1, *xptr2, scaling);
                xptr1++;
                xptr2++;
            }

            for (j = loops4; j < loops; j++)
            {
                sum += WEBRTC_SPL_MUL_16_16_RSFT(*xptr1, *xptr2, scaling);
                xptr1++;
                xptr2++;
            }
        }

#endif
        *resultptr++ = sum;
    }

    *scale = scaling;

    return order + 1;
}
