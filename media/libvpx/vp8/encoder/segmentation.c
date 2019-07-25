










#include "segmentation.h"
#include "vpx_mem/vpx_mem.h"

void vp8_update_gf_useage_maps(VP8_COMP *cpi, VP8_COMMON *cm, MACROBLOCK *x)
{
    int mb_row, mb_col;

    MODE_INFO *this_mb_mode_info = cm->mi;

    x->gf_active_ptr = (signed char *)cpi->gf_active_flags;

    if ((cm->frame_type == KEY_FRAME) || (cm->refresh_golden_frame))
    {
        
        vpx_memset(cpi->gf_active_flags, 1, (cm->mb_rows * cm->mb_cols));
        cpi->gf_active_count = cm->mb_rows * cm->mb_cols;
    }
    else
    {
        
        for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
        {
            
            for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
            {

                
                
                
                if ((this_mb_mode_info->mbmi.ref_frame == GOLDEN_FRAME) || (this_mb_mode_info->mbmi.ref_frame == ALTREF_FRAME))
                {
                    if (*(x->gf_active_ptr) == 0)
                    {
                        *(x->gf_active_ptr) = 1;
                        cpi->gf_active_count ++;
                    }
                }
                else if ((this_mb_mode_info->mbmi.mode != ZEROMV) && *(x->gf_active_ptr))
                {
                    *(x->gf_active_ptr) = 0;
                    cpi->gf_active_count--;
                }

                x->gf_active_ptr++;          
                this_mb_mode_info++;           

            }

            
            this_mb_mode_info++;
        }
    }
}
