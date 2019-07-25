
















#include "signal_processing_library.h"

#define SPL_LPC_TO_REFL_COEF_MAX_AR_MODEL_ORDER 50

void WebRtcSpl_LpcToReflCoef(WebRtc_Word16* a16, int use_order, WebRtc_Word16* k16)
{
    int m, k;
    WebRtc_Word32 tmp32[SPL_LPC_TO_REFL_COEF_MAX_AR_MODEL_ORDER];
    WebRtc_Word32 tmp_inv_denom32;
    WebRtc_Word16 tmp_inv_denom16;

    k16[use_order - 1] = WEBRTC_SPL_LSHIFT_W16(a16[use_order], 3); 
    for (m = use_order - 1; m > 0; m--)
    {
        
        tmp_inv_denom32 = ((WebRtc_Word32)1073741823) - WEBRTC_SPL_MUL_16_16(k16[m], k16[m]);
        
        tmp_inv_denom16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp_inv_denom32, 15);

        for (k = 1; k <= m; k++)
        {
            

            
            tmp32[k] = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)a16[k], 16)
                    - WEBRTC_SPL_LSHIFT_W32(WEBRTC_SPL_MUL_16_16(k16[m], a16[m-k+1]), 1);

            tmp32[k] = WebRtcSpl_DivW32W16(tmp32[k], tmp_inv_denom16); 
        }

        for (k = 1; k < m; k++)
        {
            a16[k] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp32[k], 1); 
        }

        tmp32[m] = WEBRTC_SPL_SAT(8191, tmp32[m], -8191);
        k16[m - 1] = (WebRtc_Word16)WEBRTC_SPL_LSHIFT_W32(tmp32[m], 2); 
    }
    return;
}
