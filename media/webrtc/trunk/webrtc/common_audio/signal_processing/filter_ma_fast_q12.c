
















#include "signal_processing_library.h"

void WebRtcSpl_FilterMAFastQ12(WebRtc_Word16* in_ptr,
                               WebRtc_Word16* out_ptr,
                               WebRtc_Word16* B,
                               WebRtc_Word16 B_length,
                               WebRtc_Word16 length)
{
    WebRtc_Word32 o;
    int i, j;
    for (i = 0; i < length; i++)
    {
        G_CONST WebRtc_Word16* b_ptr = &B[0];
        G_CONST WebRtc_Word16* x_ptr = &in_ptr[i];

        o = (WebRtc_Word32)0;

        for (j = 0; j < B_length; j++)
        {
            o += WEBRTC_SPL_MUL_16_16(*b_ptr++, *x_ptr--);
        }

        
        

        
        o = WEBRTC_SPL_SAT((WebRtc_Word32)134215679, o, (WebRtc_Word32)-134217728);

        *out_ptr++ = (WebRtc_Word16)((o + (WebRtc_Word32)2048) >> 12);
    }
    return;
}
