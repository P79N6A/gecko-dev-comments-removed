









#include "vpx_config.h"
#include "vp8_rtcd.h"




void idct_dequant_full_2x_neon(short *q, short *dq,
                               unsigned char *dst, int stride);
void idct_dequant_0_2x_neon(short *q, short dq,
                            unsigned char *dst, int stride);


void vp8_dequant_idct_add_y_block_neon(short *q, short *dq,
                                       unsigned char *dst,
                                       int stride, char *eobs)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (((short *)(eobs))[0])
        {
            if (((short *)eobs)[0] & 0xfefe)
                idct_dequant_full_2x_neon (q, dq, dst, stride);
            else
                idct_dequant_0_2x_neon (q, dq[0], dst, stride);
        }

        if (((short *)(eobs))[1])
        {
            if (((short *)eobs)[1] & 0xfefe)
                idct_dequant_full_2x_neon (q+32, dq, dst+8, stride);
            else
                idct_dequant_0_2x_neon (q+32, dq[0], dst+8, stride);
        }
        q    += 64;
        dst  += 4*stride;
        eobs += 4;
    }
}

void vp8_dequant_idct_add_uv_block_neon(short *q, short *dq,
                                        unsigned char *dstu,
                                        unsigned char *dstv,
                                        int stride, char *eobs)
{
    if (((short *)(eobs))[0])
    {
        if (((short *)eobs)[0] & 0xfefe)
            idct_dequant_full_2x_neon (q, dq, dstu, stride);
        else
            idct_dequant_0_2x_neon (q, dq[0], dstu, stride);
    }

    q    += 32;
    dstu += 4*stride;

    if (((short *)(eobs))[1])
    {
        if (((short *)eobs)[1] & 0xfefe)
            idct_dequant_full_2x_neon (q, dq, dstu, stride);
        else
            idct_dequant_0_2x_neon (q, dq[0], dstu, stride);
    }

    q += 32;

    if (((short *)(eobs))[2])
    {
        if (((short *)eobs)[2] & 0xfefe)
            idct_dequant_full_2x_neon (q, dq, dstv, stride);
        else
            idct_dequant_0_2x_neon (q, dq[0], dstv, stride);
    }

    q    += 32;
    dstv += 4*stride;

    if (((short *)(eobs))[3])
    {
        if (((short *)eobs)[3] & 0xfefe)
            idct_dequant_full_2x_neon (q, dq, dstv, stride);
        else
            idct_dequant_0_2x_neon (q, dq[0], dstv, stride);
    }
}
