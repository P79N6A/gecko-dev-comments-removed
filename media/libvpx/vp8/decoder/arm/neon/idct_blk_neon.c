









#include "vpx_ports/config.h"
#include "vp8/common/idct.h"
#include "vp8/decoder/dequantize.h"




void idct_dequant_dc_full_2x_neon
            (short *input, short *dq, unsigned char *pre, unsigned char *dst,
             int stride, short *dc);
void idct_dequant_dc_0_2x_neon
            (short *dc, unsigned char *pre, unsigned char *dst, int stride);
void idct_dequant_full_2x_neon
            (short *q, short *dq, unsigned char *pre, unsigned char *dst,
             int pitch, int stride);
void idct_dequant_0_2x_neon
            (short *q, short dq, unsigned char *pre, int pitch,
             unsigned char *dst, int stride);

void vp8_dequant_dc_idct_add_y_block_neon
            (short *q, short *dq, unsigned char *pre,
             unsigned char *dst, int stride, char *eobs, short *dc)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (((short *)eobs)[0] & 0xfefe)
            idct_dequant_dc_full_2x_neon (q, dq, pre, dst, stride, dc);
        else
            idct_dequant_dc_0_2x_neon(dc, pre, dst, stride);

        if (((short *)eobs)[1] & 0xfefe)
            idct_dequant_dc_full_2x_neon (q+32, dq, pre+8, dst+8, stride, dc+2);
        else
            idct_dequant_dc_0_2x_neon(dc+2, pre+8, dst+8, stride);

        q    += 64;
        dc   += 4;
        pre  += 64;
        dst  += 4*stride;
        eobs += 4;
    }
}

void vp8_dequant_idct_add_y_block_neon
            (short *q, short *dq, unsigned char *pre,
             unsigned char *dst, int stride, char *eobs)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (((short *)eobs)[0] & 0xfefe)
            idct_dequant_full_2x_neon (q, dq, pre, dst, 16, stride);
        else
            idct_dequant_0_2x_neon (q, dq[0], pre, 16, dst, stride);

        if (((short *)eobs)[1] & 0xfefe)
            idct_dequant_full_2x_neon (q+32, dq, pre+8, dst+8, 16, stride);
        else
            idct_dequant_0_2x_neon (q+32, dq[0], pre+8, 16, dst+8, stride);

        q    += 64;
        pre  += 64;
        dst  += 4*stride;
        eobs += 4;
    }
}

void vp8_dequant_idct_add_uv_block_neon
            (short *q, short *dq, unsigned char *pre,
             unsigned char *dstu, unsigned char *dstv, int stride, char *eobs)
{
    if (((short *)eobs)[0] & 0xfefe)
        idct_dequant_full_2x_neon (q, dq, pre, dstu, 8, stride);
    else
        idct_dequant_0_2x_neon (q, dq[0], pre, 8, dstu, stride);

    q    += 32;
    pre  += 32;
    dstu += 4*stride;

    if (((short *)eobs)[1] & 0xfefe)
        idct_dequant_full_2x_neon (q, dq, pre, dstu, 8, stride);
    else
        idct_dequant_0_2x_neon (q, dq[0], pre, 8, dstu, stride);

    q += 32;
    pre += 32;

    if (((short *)eobs)[2] & 0xfefe)
        idct_dequant_full_2x_neon (q, dq, pre, dstv, 8, stride);
    else
        idct_dequant_0_2x_neon (q, dq[0], pre, 8, dstv, stride);

    q    += 32;
    pre  += 32;
    dstv += 4*stride;

    if (((short *)eobs)[3] & 0xfefe)
        idct_dequant_full_2x_neon (q, dq, pre, dstv, 8, stride);
    else
        idct_dequant_0_2x_neon (q, dq[0], pre, 8, dstv, stride);
}
