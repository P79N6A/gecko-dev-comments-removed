










#ifndef __INC_INVTRANS_H
#define __INC_INVTRANS_H

#include "vpx_config.h"
#include "vp8_rtcd.h"
#include "blockd.h"
#include "onyxc_int.h"

#if CONFIG_MULTITHREAD
#include "vpx_mem/vpx_mem.h"
#endif

static void eob_adjust(char *eobs, short *diff)
{
    
    int js;
    for(js = 0; js < 16; js++)
    {
        if((eobs[js] == 0) && (diff[0] != 0))
            eobs[js]++;
        diff+=16;
    }
}

static void vp8_inverse_transform_mby(MACROBLOCKD *xd)
{
    short *DQC = xd->dequant_y1;

    if (xd->mode_info_context->mbmi.mode != SPLITMV)
    {
        
        if (xd->eobs[24] > 1)
        {
            vp8_short_inv_walsh4x4
                (&xd->block[24].dqcoeff[0], xd->qcoeff);
        }
        else
        {
            vp8_short_inv_walsh4x4_1
                (&xd->block[24].dqcoeff[0], xd->qcoeff);
        }
        eob_adjust(xd->eobs, xd->qcoeff);

        DQC = xd->dequant_y1_dc;
    }
    vp8_dequant_idct_add_y_block
                    (xd->qcoeff, DQC,
                     xd->dst.y_buffer,
                     xd->dst.y_stride, xd->eobs);
}
#endif
