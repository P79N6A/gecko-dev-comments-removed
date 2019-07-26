
















#include "signal_processing_library.h"

void WebRtcSpl_SqrtOfOneMinusXSquared(WebRtc_Word16 *xQ15, int vector_length,
                                      WebRtc_Word16 *yQ15)
{
    WebRtc_Word32 sq;
    int m;
    WebRtc_Word16 tmp;

    for (m = 0; m < vector_length; m++)
    {
        tmp = xQ15[m];
        sq = WEBRTC_SPL_MUL_16_16(tmp, tmp); 
        sq = 1073741823 - sq; 
        sq = WebRtcSpl_Sqrt(sq); 
        yQ15[m] = (WebRtc_Word16)sq;
    }
}
