
















#include "signal_processing_library.h"

#define SPL_LEVINSON_MAXORDER 20

WebRtc_Word16 WebRtcSpl_LevinsonDurbin(WebRtc_Word32 *R, WebRtc_Word16 *A, WebRtc_Word16 *K,
                                       WebRtc_Word16 order)
{
    WebRtc_Word16 i, j;
    
    WebRtc_Word16 R_hi[SPL_LEVINSON_MAXORDER + 1], R_low[SPL_LEVINSON_MAXORDER + 1];
    
    WebRtc_Word16 A_hi[SPL_LEVINSON_MAXORDER + 1], A_low[SPL_LEVINSON_MAXORDER + 1];
    
    WebRtc_Word16 A_upd_hi[SPL_LEVINSON_MAXORDER + 1], A_upd_low[SPL_LEVINSON_MAXORDER + 1];
    
    WebRtc_Word16 K_hi, K_low;
    
    WebRtc_Word16 Alpha_hi, Alpha_low, Alpha_exp;
    WebRtc_Word16 tmp_hi, tmp_low;
    WebRtc_Word32 temp1W32, temp2W32, temp3W32;
    WebRtc_Word16 norm;

    

    norm = WebRtcSpl_NormW32(R[0]);

    for (i = order; i >= 0; i--)
    {
        temp1W32 = WEBRTC_SPL_LSHIFT_W32(R[i], norm);
        
        R_hi[i] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
        R_low[i] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
                - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)R_hi[i], 16)), 1);
    }

    

    temp2W32 = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)R_hi[1],16)
            + WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)R_low[1],1); 
    temp3W32 = WEBRTC_SPL_ABS_W32(temp2W32); 
    temp1W32 = WebRtcSpl_DivW32HiLow(temp3W32, R_hi[0], R_low[0]); 
    
    if (temp2W32 > 0)
    {
        temp1W32 = -temp1W32;
    }

    
    K_hi = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
    K_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
            - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)K_hi, 16)), 1);

    
    K[0] = K_hi;

    temp1W32 = WEBRTC_SPL_RSHIFT_W32(temp1W32, 4); 

    
    A_hi[1] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
    A_low[1] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
            - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)A_hi[1], 16)), 1);

    

    temp1W32 = (((WEBRTC_SPL_MUL_16_16(K_hi, K_low) >> 14) + WEBRTC_SPL_MUL_16_16(K_hi, K_hi))
            << 1); 

    temp1W32 = WEBRTC_SPL_ABS_W32(temp1W32); 
    temp1W32 = (WebRtc_Word32)0x7fffffffL - temp1W32; 

    
    tmp_hi = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
    tmp_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
            - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)tmp_hi, 16)), 1);

    
    temp1W32 = ((WEBRTC_SPL_MUL_16_16(R_hi[0], tmp_hi)
            + (WEBRTC_SPL_MUL_16_16(R_hi[0], tmp_low) >> 15)
            + (WEBRTC_SPL_MUL_16_16(R_low[0], tmp_hi) >> 15)) << 1);

    

    Alpha_exp = WebRtcSpl_NormW32(temp1W32);
    temp1W32 = WEBRTC_SPL_LSHIFT_W32(temp1W32, Alpha_exp);
    Alpha_hi = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
    Alpha_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
            - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)Alpha_hi, 16)), 1);

    

    for (i = 2; i <= order; i++)
    {
        






        temp1W32 = 0;

        for (j = 1; j < i; j++)
        {
            
            temp1W32 += ((WEBRTC_SPL_MUL_16_16(R_hi[j], A_hi[i-j]) << 1)
                    + (((WEBRTC_SPL_MUL_16_16(R_hi[j], A_low[i-j]) >> 15)
                            + (WEBRTC_SPL_MUL_16_16(R_low[j], A_hi[i-j]) >> 15)) << 1));
        }

        temp1W32 = WEBRTC_SPL_LSHIFT_W32(temp1W32, 4);
        temp1W32 += (WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)R_hi[i], 16)
                + WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)R_low[i], 1));

        
        temp2W32 = WEBRTC_SPL_ABS_W32(temp1W32); 
        temp3W32 = WebRtcSpl_DivW32HiLow(temp2W32, Alpha_hi, Alpha_low); 

        
        if (temp1W32 > 0)
        {
            temp3W32 = -temp3W32;
        }

        
        norm = WebRtcSpl_NormW32(temp3W32);
        if ((Alpha_exp <= norm) || (temp3W32 == 0))
        {
            temp3W32 = WEBRTC_SPL_LSHIFT_W32(temp3W32, Alpha_exp);
        } else
        {
            if (temp3W32 > 0)
            {
                temp3W32 = (WebRtc_Word32)0x7fffffffL;
            } else
            {
                temp3W32 = (WebRtc_Word32)0x80000000L;
            }
        }

        
        K_hi = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp3W32, 16);
        K_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp3W32
                - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)K_hi, 16)), 1);

        
        K[i - 1] = K_hi;

        
        

        if ((WebRtc_Word32)WEBRTC_SPL_ABS_W16(K_hi) > (WebRtc_Word32)32750)
        {
            return 0; 
        }

        





        for (j = 1; j < i; j++)
        {
            
            temp1W32 = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)A_hi[j],16)
                    + WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)A_low[j],1);

            
            temp1W32 += ((WEBRTC_SPL_MUL_16_16(K_hi, A_hi[i-j])
                    + (WEBRTC_SPL_MUL_16_16(K_hi, A_low[i-j]) >> 15)
                    + (WEBRTC_SPL_MUL_16_16(K_low, A_hi[i-j]) >> 15)) << 1);

            
            A_upd_hi[j] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
            A_upd_low[j] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
                    - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)A_upd_hi[j], 16)), 1);
        }

        
        temp3W32 = WEBRTC_SPL_RSHIFT_W32(temp3W32, 4);

        
        A_upd_hi[i] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp3W32, 16);
        A_upd_low[i] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp3W32
                - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)A_upd_hi[i], 16)), 1);

        

        temp1W32 = (((WEBRTC_SPL_MUL_16_16(K_hi, K_low) >> 14)
                + WEBRTC_SPL_MUL_16_16(K_hi, K_hi)) << 1); 

        temp1W32 = WEBRTC_SPL_ABS_W32(temp1W32); 
        temp1W32 = (WebRtc_Word32)0x7fffffffL - temp1W32; 

        
        tmp_hi = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
        tmp_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
                - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)tmp_hi, 16)), 1);

        
        temp1W32 = ((WEBRTC_SPL_MUL_16_16(Alpha_hi, tmp_hi)
                + (WEBRTC_SPL_MUL_16_16(Alpha_hi, tmp_low) >> 15)
                + (WEBRTC_SPL_MUL_16_16(Alpha_low, tmp_hi) >> 15)) << 1);

        

        norm = WebRtcSpl_NormW32(temp1W32);
        temp1W32 = WEBRTC_SPL_LSHIFT_W32(temp1W32, norm);

        Alpha_hi = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(temp1W32, 16);
        Alpha_low = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32
                - WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)Alpha_hi, 16)), 1);

        
        Alpha_exp = Alpha_exp + norm;

        

        for (j = 1; j <= i; j++)
        {
            A_hi[j] = A_upd_hi[j];
            A_low[j] = A_upd_low[j];
        }
    }

    




    A[0] = 4096;

    for (i = 1; i <= order; i++)
    {
        
        temp1W32 = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)A_hi[i], 16)
                + WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)A_low[i], 1);
        
        A[i] = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32((temp1W32<<1)+(WebRtc_Word32)32768, 16);
    }
    return 1; 
}
