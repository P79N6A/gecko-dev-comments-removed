















#include "resample_by_2_internal.h"


static const WebRtc_Word16 kResampleAllpass[2][3] = {
        {821, 6110, 12382},
        {3050, 9368, 15063}
};







void WebRtcSpl_DownBy2IntToShort(WebRtc_Word32 *in, WebRtc_Word32 len, WebRtc_Word16 *out,
                                 WebRtc_Word32 *state)
{
    WebRtc_Word32 tmp0, tmp1, diff;
    WebRtc_Word32 i;

    len >>= 1;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i << 1];
        diff = tmp0 - state[1];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[0] + diff * kResampleAllpass[1][0];
        state[0] = tmp0;
        diff = tmp1 - state[2];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[1] + diff * kResampleAllpass[1][1];
        state[1] = tmp1;
        diff = tmp0 - state[3];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[3] = state[2] + diff * kResampleAllpass[1][2];
        state[2] = tmp0;

        
        in[i << 1] = (state[3] >> 1);
    }

    in++;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i << 1];
        diff = tmp0 - state[5];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[4] + diff * kResampleAllpass[0][0];
        state[4] = tmp0;
        diff = tmp1 - state[6];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[5] + diff * kResampleAllpass[0][1];
        state[5] = tmp1;
        diff = tmp0 - state[7];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[7] = state[6] + diff * kResampleAllpass[0][2];
        state[6] = tmp0;

        
        in[i << 1] = (state[7] >> 1);
    }

    in--;

    
    for (i = 0; i < len; i += 2)
    {
        
        tmp0 = (in[i << 1] + in[(i << 1) + 1]) >> 15;
        tmp1 = (in[(i << 1) + 2] + in[(i << 1) + 3]) >> 15;
        if (tmp0 > (WebRtc_Word32)0x00007FFF)
            tmp0 = 0x00007FFF;
        if (tmp0 < (WebRtc_Word32)0xFFFF8000)
            tmp0 = 0xFFFF8000;
        out[i] = (WebRtc_Word16)tmp0;
        if (tmp1 > (WebRtc_Word32)0x00007FFF)
            tmp1 = 0x00007FFF;
        if (tmp1 < (WebRtc_Word32)0xFFFF8000)
            tmp1 = 0xFFFF8000;
        out[i + 1] = (WebRtc_Word16)tmp1;
    }
}







void WebRtcSpl_DownBy2ShortToInt(const WebRtc_Word16 *in,
                                  WebRtc_Word32 len,
                                  WebRtc_Word32 *out,
                                  WebRtc_Word32 *state)
{
    WebRtc_Word32 tmp0, tmp1, diff;
    WebRtc_Word32 i;

    len >>= 1;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = ((WebRtc_Word32)in[i << 1] << 15) + (1 << 14);
        diff = tmp0 - state[1];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[0] + diff * kResampleAllpass[1][0];
        state[0] = tmp0;
        diff = tmp1 - state[2];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[1] + diff * kResampleAllpass[1][1];
        state[1] = tmp1;
        diff = tmp0 - state[3];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[3] = state[2] + diff * kResampleAllpass[1][2];
        state[2] = tmp0;

        
        out[i] = (state[3] >> 1);
    }

    in++;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = ((WebRtc_Word32)in[i << 1] << 15) + (1 << 14);
        diff = tmp0 - state[5];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[4] + diff * kResampleAllpass[0][0];
        state[4] = tmp0;
        diff = tmp1 - state[6];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[5] + diff * kResampleAllpass[0][1];
        state[5] = tmp1;
        diff = tmp0 - state[7];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[7] = state[6] + diff * kResampleAllpass[0][2];
        state[6] = tmp0;

        
        out[i] += (state[7] >> 1);
    }

    in--;
}






void WebRtcSpl_UpBy2ShortToInt(const WebRtc_Word16 *in, WebRtc_Word32 len, WebRtc_Word32 *out,
                               WebRtc_Word32 *state)
{
    WebRtc_Word32 tmp0, tmp1, diff;
    WebRtc_Word32 i;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = ((WebRtc_Word32)in[i] << 15) + (1 << 14);
        diff = tmp0 - state[5];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[4] + diff * kResampleAllpass[0][0];
        state[4] = tmp0;
        diff = tmp1 - state[6];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[5] + diff * kResampleAllpass[0][1];
        state[5] = tmp1;
        diff = tmp0 - state[7];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[7] = state[6] + diff * kResampleAllpass[0][2];
        state[6] = tmp0;

        
        out[i << 1] = state[7] >> 15;
    }

    out++;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = ((WebRtc_Word32)in[i] << 15) + (1 << 14);
        diff = tmp0 - state[1];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[0] + diff * kResampleAllpass[1][0];
        state[0] = tmp0;
        diff = tmp1 - state[2];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[1] + diff * kResampleAllpass[1][1];
        state[1] = tmp1;
        diff = tmp0 - state[3];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[3] = state[2] + diff * kResampleAllpass[1][2];
        state[2] = tmp0;

        
        out[i << 1] = state[3] >> 15;
    }
}






void WebRtcSpl_UpBy2IntToInt(const WebRtc_Word32 *in, WebRtc_Word32 len, WebRtc_Word32 *out,
                             WebRtc_Word32 *state)
{
    WebRtc_Word32 tmp0, tmp1, diff;
    WebRtc_Word32 i;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i];
        diff = tmp0 - state[5];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[4] + diff * kResampleAllpass[0][0];
        state[4] = tmp0;
        diff = tmp1 - state[6];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[5] + diff * kResampleAllpass[0][1];
        state[5] = tmp1;
        diff = tmp0 - state[7];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[7] = state[6] + diff * kResampleAllpass[0][2];
        state[6] = tmp0;

        
        out[i << 1] = state[7];
    }

    out++;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i];
        diff = tmp0 - state[1];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[0] + diff * kResampleAllpass[1][0];
        state[0] = tmp0;
        diff = tmp1 - state[2];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[1] + diff * kResampleAllpass[1][1];
        state[1] = tmp1;
        diff = tmp0 - state[3];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[3] = state[2] + diff * kResampleAllpass[1][2];
        state[2] = tmp0;

        
        out[i << 1] = state[3];
    }
}






void WebRtcSpl_UpBy2IntToShort(const WebRtc_Word32 *in, WebRtc_Word32 len, WebRtc_Word16 *out,
                               WebRtc_Word32 *state)
{
    WebRtc_Word32 tmp0, tmp1, diff;
    WebRtc_Word32 i;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i];
        diff = tmp0 - state[5];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[4] + diff * kResampleAllpass[0][0];
        state[4] = tmp0;
        diff = tmp1 - state[6];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[5] + diff * kResampleAllpass[0][1];
        state[5] = tmp1;
        diff = tmp0 - state[7];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[7] = state[6] + diff * kResampleAllpass[0][2];
        state[6] = tmp0;

        
        tmp1 = state[7] >> 15;
        if (tmp1 > (WebRtc_Word32)0x00007FFF)
            tmp1 = 0x00007FFF;
        if (tmp1 < (WebRtc_Word32)0xFFFF8000)
            tmp1 = 0xFFFF8000;
        out[i << 1] = (WebRtc_Word16)tmp1;
    }

    out++;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i];
        diff = tmp0 - state[1];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[0] + diff * kResampleAllpass[1][0];
        state[0] = tmp0;
        diff = tmp1 - state[2];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[1] + diff * kResampleAllpass[1][1];
        state[1] = tmp1;
        diff = tmp0 - state[3];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[3] = state[2] + diff * kResampleAllpass[1][2];
        state[2] = tmp0;

        
        tmp1 = state[3] >> 15;
        if (tmp1 > (WebRtc_Word32)0x00007FFF)
            tmp1 = 0x00007FFF;
        if (tmp1 < (WebRtc_Word32)0xFFFF8000)
            tmp1 = 0xFFFF8000;
        out[i << 1] = (WebRtc_Word16)tmp1;
    }
}





void WebRtcSpl_LPBy2ShortToInt(const WebRtc_Word16* in, WebRtc_Word32 len, WebRtc_Word32* out,
                               WebRtc_Word32* state)
{
    WebRtc_Word32 tmp0, tmp1, diff;
    WebRtc_Word32 i;

    len >>= 1;

    
    in++;
    
    tmp0 = state[12];
    for (i = 0; i < len; i++)
    {
        diff = tmp0 - state[1];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[0] + diff * kResampleAllpass[1][0];
        state[0] = tmp0;
        diff = tmp1 - state[2];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[1] + diff * kResampleAllpass[1][1];
        state[1] = tmp1;
        diff = tmp0 - state[3];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[3] = state[2] + diff * kResampleAllpass[1][2];
        state[2] = tmp0;

        
        out[i << 1] = state[3] >> 1;
        tmp0 = ((WebRtc_Word32)in[i << 1] << 15) + (1 << 14);
    }
    in--;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = ((WebRtc_Word32)in[i << 1] << 15) + (1 << 14);
        diff = tmp0 - state[5];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[4] + diff * kResampleAllpass[0][0];
        state[4] = tmp0;
        diff = tmp1 - state[6];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[5] + diff * kResampleAllpass[0][1];
        state[5] = tmp1;
        diff = tmp0 - state[7];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[7] = state[6] + diff * kResampleAllpass[0][2];
        state[6] = tmp0;

        
        out[i << 1] = (out[i << 1] + (state[7] >> 1)) >> 15;
    }

    
    out++;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = ((WebRtc_Word32)in[i << 1] << 15) + (1 << 14);
        diff = tmp0 - state[9];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[8] + diff * kResampleAllpass[1][0];
        state[8] = tmp0;
        diff = tmp1 - state[10];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[9] + diff * kResampleAllpass[1][1];
        state[9] = tmp1;
        diff = tmp0 - state[11];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[11] = state[10] + diff * kResampleAllpass[1][2];
        state[10] = tmp0;

        
        out[i << 1] = state[11] >> 1;
    }

    
    in++;
    for (i = 0; i < len; i++)
    {
        tmp0 = ((WebRtc_Word32)in[i << 1] << 15) + (1 << 14);
        diff = tmp0 - state[13];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[12] + diff * kResampleAllpass[0][0];
        state[12] = tmp0;
        diff = tmp1 - state[14];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[13] + diff * kResampleAllpass[0][1];
        state[13] = tmp1;
        diff = tmp0 - state[15];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[15] = state[14] + diff * kResampleAllpass[0][2];
        state[14] = tmp0;

        
        out[i << 1] = (out[i << 1] + (state[15] >> 1)) >> 15;
    }
}





void WebRtcSpl_LPBy2IntToInt(const WebRtc_Word32* in, WebRtc_Word32 len, WebRtc_Word32* out,
                             WebRtc_Word32* state)
{
    WebRtc_Word32 tmp0, tmp1, diff;
    WebRtc_Word32 i;

    len >>= 1;

    
    in++;
    
    tmp0 = state[12];
    for (i = 0; i < len; i++)
    {
        diff = tmp0 - state[1];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[0] + diff * kResampleAllpass[1][0];
        state[0] = tmp0;
        diff = tmp1 - state[2];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[1] + diff * kResampleAllpass[1][1];
        state[1] = tmp1;
        diff = tmp0 - state[3];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[3] = state[2] + diff * kResampleAllpass[1][2];
        state[2] = tmp0;

        
        out[i << 1] = state[3] >> 1;
        tmp0 = in[i << 1];
    }
    in--;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i << 1];
        diff = tmp0 - state[5];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[4] + diff * kResampleAllpass[0][0];
        state[4] = tmp0;
        diff = tmp1 - state[6];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[5] + diff * kResampleAllpass[0][1];
        state[5] = tmp1;
        diff = tmp0 - state[7];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[7] = state[6] + diff * kResampleAllpass[0][2];
        state[6] = tmp0;

        
        out[i << 1] = (out[i << 1] + (state[7] >> 1)) >> 15;
    }

    
    out++;

    
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i << 1];
        diff = tmp0 - state[9];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[8] + diff * kResampleAllpass[1][0];
        state[8] = tmp0;
        diff = tmp1 - state[10];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[9] + diff * kResampleAllpass[1][1];
        state[9] = tmp1;
        diff = tmp0 - state[11];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[11] = state[10] + diff * kResampleAllpass[1][2];
        state[10] = tmp0;

        
        out[i << 1] = state[11] >> 1;
    }

    
    in++;
    for (i = 0; i < len; i++)
    {
        tmp0 = in[i << 1];
        diff = tmp0 - state[13];
        
        diff = (diff + (1 << 13)) >> 14;
        tmp1 = state[12] + diff * kResampleAllpass[0][0];
        state[12] = tmp0;
        diff = tmp1 - state[14];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        tmp0 = state[13] + diff * kResampleAllpass[0][1];
        state[13] = tmp1;
        diff = tmp0 - state[15];
        
        diff = diff >> 14;
        if (diff < 0)
            diff += 1;
        state[15] = state[14] + diff * kResampleAllpass[0][2];
        state[14] = tmp0;

        
        out[i << 1] = (out[i << 1] + (state[15] >> 1)) >> 15;
    }
}
