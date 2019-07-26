






















#include "signal_processing_library.h"

uint32_t WebRtcSpl_DivU32U16(uint32_t num, uint16_t den)
{
    
    if (den != 0)
    {
        return (uint32_t)(num / den);
    } else
    {
        return (uint32_t)0xFFFFFFFF;
    }
}

int32_t WebRtcSpl_DivW32W16(int32_t num, int16_t den)
{
    
    if (den != 0)
    {
        return (int32_t)(num / den);
    } else
    {
        return (int32_t)0x7FFFFFFF;
    }
}

int16_t WebRtcSpl_DivW32W16ResW16(int32_t num, int16_t den)
{
    
    if (den != 0)
    {
        return (int16_t)(num / den);
    } else
    {
        return (int16_t)0x7FFF;
    }
}

int32_t WebRtcSpl_DivResultInQ31(int32_t num, int32_t den)
{
    int32_t L_num = num;
    int32_t L_den = den;
    int32_t div = 0;
    int k = 31;
    int change_sign = 0;

    if (num == 0)
        return 0;

    if (num < 0)
    {
        change_sign++;
        L_num = -num;
    }
    if (den < 0)
    {
        change_sign++;
        L_den = -den;
    }
    while (k--)
    {
        div <<= 1;
        L_num <<= 1;
        if (L_num >= L_den)
        {
            L_num -= L_den;
            div++;
        }
    }
    if (change_sign == 1)
    {
        div = -div;
    }
    return div;
}

int32_t WebRtcSpl_DivW32HiLow(int32_t num, int16_t den_hi, int16_t den_low)
{
    int16_t approx, tmp_hi, tmp_low, num_hi, num_low;
    int32_t tmpW32;

    approx = (int16_t)WebRtcSpl_DivW32W16((int32_t)0x1FFFFFFF, den_hi);
    

    
    tmpW32 = (WEBRTC_SPL_MUL_16_16(den_hi, approx) << 1)
            + ((WEBRTC_SPL_MUL_16_16(den_low, approx) >> 15) << 1);
    

    tmpW32 = (int32_t)0x7fffffffL - tmpW32; 

    
    tmp_hi = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmpW32, 16);
    tmp_low = (int16_t)WEBRTC_SPL_RSHIFT_W32((tmpW32
            - WEBRTC_SPL_LSHIFT_W32((int32_t)tmp_hi, 16)), 1);

    
    tmpW32 = ((WEBRTC_SPL_MUL_16_16(tmp_hi, approx) + (WEBRTC_SPL_MUL_16_16(tmp_low, approx)
            >> 15)) << 1);

    
    tmp_hi = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmpW32, 16);
    tmp_low = (int16_t)WEBRTC_SPL_RSHIFT_W32((tmpW32
            - WEBRTC_SPL_LSHIFT_W32((int32_t)tmp_hi, 16)), 1);

    
    num_hi = (int16_t)WEBRTC_SPL_RSHIFT_W32(num, 16);
    num_low = (int16_t)WEBRTC_SPL_RSHIFT_W32((num
            - WEBRTC_SPL_LSHIFT_W32((int32_t)num_hi, 16)), 1);

    

    tmpW32 = (WEBRTC_SPL_MUL_16_16(num_hi, tmp_hi) + (WEBRTC_SPL_MUL_16_16(num_hi, tmp_low)
            >> 15) + (WEBRTC_SPL_MUL_16_16(num_low, tmp_hi) >> 15));

    
    tmpW32 = WEBRTC_SPL_LSHIFT_W32(tmpW32, 3);

    return tmpW32;
}
