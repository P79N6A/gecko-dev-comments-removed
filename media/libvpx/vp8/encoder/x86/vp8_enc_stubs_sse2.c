










#include "vpx_config.h"
#include "vp8_rtcd.h"
#include "vpx_ports/x86.h"
#include "vp8/encoder/block.h"

int vp8_mbblock_error_xmm_impl(short *coeff_ptr, short *dcoef_ptr, int dc);
int vp8_mbblock_error_xmm(MACROBLOCK *mb, int dc)
{
    short *coeff_ptr =  mb->block[0].coeff;
    short *dcoef_ptr =  mb->e_mbd.block[0].dqcoeff;
    return vp8_mbblock_error_xmm_impl(coeff_ptr, dcoef_ptr, dc);
}

int vp8_mbuverror_xmm_impl(short *s_ptr, short *d_ptr);
int vp8_mbuverror_xmm(MACROBLOCK *mb)
{
    short *s_ptr = &mb->coeff[256];
    short *d_ptr = &mb->e_mbd.dqcoeff[256];
    return vp8_mbuverror_xmm_impl(s_ptr, d_ptr);
}

void vp8_subtract_b_sse2_impl(unsigned char *z,  int src_stride,
                             short *diff, unsigned char *predictor,
                             int pitch);
void vp8_subtract_b_sse2(BLOCK *be, BLOCKD *bd, int pitch)
{
    unsigned char *z = *(be->base_src) + be->src;
    unsigned int  src_stride = be->src_stride;
    short *diff = &be->src_diff[0];
    unsigned char *predictor = &bd->predictor[0];
    vp8_subtract_b_sse2_impl(z, src_stride, diff, predictor, pitch);
}
