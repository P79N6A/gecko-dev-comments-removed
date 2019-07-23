
















#include <limits.h>
#if !defined(_decint_H)
# define _decint_H (1)
# include "theora/theoradec.h"
# include "../internal.h"
# include "bitpack.h"

typedef struct th_setup_info oc_setup_info;
typedef struct th_dec_ctx    oc_dec_ctx;

# include "idct.h"
# include "huffdec.h"
# include "dequant.h"




#define OC_PACKET_DATA (0)



struct th_setup_info{
  
  oc_huff_node      *huff_tables[TH_NHUFFMAN_TABLES];
  
  th_quant_info  qinfo;
};



struct th_dec_ctx{
  
  oc_theora_state      state;
  



  int                  packet_state;
  
  oggpack_buffer       opb;
  
  oc_huff_node        *huff_tables[TH_NHUFFMAN_TABLES];
  

  int                  ti0[3][64];
  



  int                  ebi0[3][64];
  

  int                  eob_runs[3][64];
  
  unsigned char      **dct_tokens;
  
  ogg_uint16_t       **extra_bits;
  
  int                  pp_level;
  
  int                  pp_dc_scale[64];
  
  int                  pp_sharp_mod[64];
  
  unsigned char       *dc_qis;
  
  int                 *variances;
  
  unsigned char       *pp_frame_data;
  
  int                  pp_frame_has_chroma;
  
  th_ycbcr_buffer      pp_frame_buf;
  
  th_stripe_callback   stripe_cb;
};

#endif
