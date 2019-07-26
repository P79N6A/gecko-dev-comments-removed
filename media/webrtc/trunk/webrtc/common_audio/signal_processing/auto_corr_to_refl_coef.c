
















#include "signal_processing_library.h"

void WebRtcSpl_AutoCorrToReflCoef(const int32_t *R, int use_order, int16_t *K)
{
    int i, n;
    int16_t tmp;
    const int32_t *rptr;
    int32_t L_num, L_den;
    int16_t *acfptr, *pptr, *wptr, *p1ptr, *w1ptr, ACF[WEBRTC_SPL_MAX_LPC_ORDER],
            P[WEBRTC_SPL_MAX_LPC_ORDER], W[WEBRTC_SPL_MAX_LPC_ORDER];

    
    acfptr = ACF;
    rptr = R;
    pptr = P;
    p1ptr = &P[1];
    w1ptr = &W[1];
    wptr = w1ptr;

    
    tmp = WebRtcSpl_NormW32(*R);
    *acfptr = (int16_t)((*rptr++ << tmp) >> 16);
    *pptr++ = *acfptr++;

    
    for (i = 1; i <= use_order; i++)
    {
        *acfptr = (int16_t)((*rptr++ << tmp) >> 16);
        *wptr++ = *acfptr;
        *pptr++ = *acfptr++;
    }

    
    for (n = 1; n <= use_order; n++, K++)
    {
        tmp = WEBRTC_SPL_ABS_W16(*p1ptr);
        if (*P < tmp)
        {
            for (i = n; i <= use_order; i++)
                *K++ = 0;

            return;
        }

        
        *K = 0;
        if (tmp != 0)
        {
            L_num = tmp;
            L_den = *P;
            i = 15;
            while (i--)
            {
                (*K) <<= 1;
                L_num <<= 1;
                if (L_num >= L_den)
                {
                    L_num -= L_den;
                    (*K)++;
                }
            }
            if (*p1ptr > 0)
                *K = -*K;
        }

        
        if (n == use_order)
            return;

        
        pptr = P;
        wptr = w1ptr;
        tmp = (int16_t)(((int32_t)*p1ptr * (int32_t)*K + 16384) >> 15);
        *pptr = WEBRTC_SPL_ADD_SAT_W16( *pptr, tmp );
        pptr++;
        for (i = 1; i <= use_order - n; i++)
        {
            tmp = (int16_t)(((int32_t)*wptr * (int32_t)*K + 16384) >> 15);
            *pptr = WEBRTC_SPL_ADD_SAT_W16( *(pptr+1), tmp );
            pptr++;
            tmp = (int16_t)(((int32_t)*pptr * (int32_t)*K + 16384) >> 15);
            *wptr = WEBRTC_SPL_ADD_SAT_W16( *wptr, tmp );
            wptr++;
        }
    }
}
