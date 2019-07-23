
















#if !defined(_quant_H)
# define _quant_H (1)
# include "theora/codec.h"
# include "ocintrin.h"

typedef ogg_uint16_t   oc_quant_table[64];



#define OC_QUANT_MAX          (1024<<2)


void oc_dequant_tables_init(ogg_uint16_t *_dequant[64][3][2],
 int _pp_dc_scale[64],const th_quant_info *_qinfo);

#endif
