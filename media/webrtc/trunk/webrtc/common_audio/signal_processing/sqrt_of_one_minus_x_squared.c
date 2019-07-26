
















#include "signal_processing_library.h"

void WebRtcSpl_SqrtOfOneMinusXSquared(int16_t *xQ15, int vector_length,
                                      int16_t *yQ15)
{
    int32_t sq;
    int m;
    int16_t tmp;

    for (m = 0; m < vector_length; m++)
    {
        tmp = xQ15[m];
        sq = WEBRTC_SPL_MUL_16_16(tmp, tmp); 
        sq = 1073741823 - sq; 
        sq = WebRtcSpl_Sqrt(sq); 
        yQ15[m] = (int16_t)sq;
    }
}
