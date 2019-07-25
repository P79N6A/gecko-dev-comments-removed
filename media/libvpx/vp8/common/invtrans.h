










#ifndef __INC_INVTRANS_H
#define __INC_INVTRANS_H

#include "vpx_config.h"
#include "idct.h"
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

static void vp8_inverse_transform_mby(MACROBLOCKD *xd,
                                      const VP8_COMMON_RTCD *rtcd)
{
    short *DQC = xd->dequant_y1;

    if (xd->mode_info_context->mbmi.mode != SPLITMV)
    {
        
        if (xd->eobs[24] > 1)
        {
            IDCT_INVOKE(&rtcd->idct, iwalsh16)
                (&xd->block[24].dqcoeff[0], xd->qcoeff);
        }
        else
        {
            IDCT_INVOKE(&rtcd->idct, iwalsh1)
                (&xd->block[24].dqcoeff[0], xd->qcoeff);
        }
        eob_adjust(xd->eobs, xd->qcoeff);

        DQC = xd->dequant_y1_dc;
    }
    DEQUANT_INVOKE (&rtcd->dequant, idct_add_y_block)
                    (xd->qcoeff, DQC,
                     xd->dst.y_buffer,
                     xd->dst.y_stride, xd->eobs);
}
#endif
