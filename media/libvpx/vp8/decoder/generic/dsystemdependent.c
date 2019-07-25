










#include "vpx_ports/config.h"
#include "dequantize.h"
#include "onyxd_int.h"

extern void vp8_arch_x86_decode_init(VP8D_COMP *pbi);

void vp8_dmachine_specific_config(VP8D_COMP *pbi)
{
    
#if CONFIG_RUNTIME_CPU_DETECT
    pbi->mb.rtcd                     = &pbi->common.rtcd;
    pbi->dequant.block               = vp8_dequantize_b_c;
    pbi->dequant.idct_add            = vp8_dequant_idct_add_c;
    pbi->dequant.dc_idct_add         = vp8_dequant_dc_idct_add_c;
    pbi->dequant.dc_idct_add_y_block = vp8_dequant_dc_idct_add_y_block_c;
    pbi->dequant.idct_add_y_block    = vp8_dequant_idct_add_y_block_c;
    pbi->dequant.idct_add_uv_block   = vp8_dequant_idct_add_uv_block_c;
    pbi->dboolhuff.start             = vp8dx_start_decode_c;
    pbi->dboolhuff.fill              = vp8dx_bool_decoder_fill_c;
#if 0 
    pbi->dboolhuff.debool = vp8dx_decode_bool_c;
    pbi->dboolhuff.devalue = vp8dx_decode_value_c;
#endif
#endif

#if ARCH_X86 || ARCH_X86_64
    vp8_arch_x86_decode_init(pbi);
#endif
}
