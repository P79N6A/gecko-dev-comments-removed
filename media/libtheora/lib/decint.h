
















#include <limits.h>
#if !defined(_decint_H)
# define _decint_H (1)
# include "theora/theoradec.h"
# include "internal.h"
# include "bitpack.h"

typedef struct th_setup_info oc_setup_info;
typedef struct th_dec_ctx    oc_dec_ctx;

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
  
  oc_pack_buf          opb;
  
  oc_huff_node        *huff_tables[TH_NHUFFMAN_TABLES];
  
  ptrdiff_t            ti0[3][64];
  

  ptrdiff_t            eob_runs[3][64];
  
  unsigned char       *dct_tokens;
  
  unsigned char       *extra_bits;
  
  int                  dct_tokens_count;
  
  int                  pp_level;
  
  int                  pp_dc_scale[64];
  
  int                  pp_sharp_mod[64];
  
  unsigned char       *dc_qis;
  
  int                 *variances;
  
  unsigned char       *pp_frame_data;
  
  int                  pp_frame_state;
  


  th_ycbcr_buffer      pp_frame_buf;
  
  th_stripe_callback   stripe_cb;
# if defined(HAVE_CAIRO)
  
  int                  telemetry;
  int                  telemetry_mbmode;
  int                  telemetry_mv;
  int                  telemetry_qi;
  int                  telemetry_bits;
  int                  telemetry_frame_bytes;
  int                  telemetry_coding_bytes;
  int                  telemetry_mode_bytes;
  int                  telemetry_mv_bytes;
  int                  telemetry_qi_bytes;
  int                  telemetry_dc_bytes;
  unsigned char       *telemetry_frame_data;
# endif
};

#endif
