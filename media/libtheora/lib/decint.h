
















#include <limits.h>
#if !defined(_decint_H)
# define _decint_H (1)
# include "theora/theoradec.h"
# include "state.h"
# include "bitpack.h"
# include "huffdec.h"
# include "dequant.h"

typedef struct th_setup_info         oc_setup_info;
typedef struct oc_dec_opt_vtable     oc_dec_opt_vtable;
typedef struct oc_dec_pipeline_state oc_dec_pipeline_state;
typedef struct th_dec_ctx            oc_dec_ctx;




# if defined(OC_C64X_ASM)
#  include "c64x/c64xdec.h"
# endif

# if !defined(oc_dec_accel_init)
#  define oc_dec_accel_init oc_dec_accel_init_c
# endif
# if defined(OC_DEC_USE_VTABLE)
#  if !defined(oc_dec_dc_unpredict_mcu_plane)
#   define oc_dec_dc_unpredict_mcu_plane(_dec,_pipe,_pli) \
 ((*(_dec)->opt_vtable.dc_unpredict_mcu_plane)(_dec,_pipe,_pli))
#  endif
# else
#  if !defined(oc_dec_dc_unpredict_mcu_plane)
#   define oc_dec_dc_unpredict_mcu_plane oc_dec_dc_unpredict_mcu_plane_c
#  endif
# endif






#define OC_PACKET_DATA (0)



struct th_setup_info{
  
  ogg_int16_t   *huff_tables[TH_NHUFFMAN_TABLES];
  
  th_quant_info  qinfo;
};




struct oc_dec_opt_vtable{
  void (*dc_unpredict_mcu_plane)(oc_dec_ctx *_dec,
   oc_dec_pipeline_state *_pipe,int _pli);
};



struct oc_dec_pipeline_state{
  















  OC_ALIGN16(ogg_int16_t dct_coeffs[128]);
  OC_ALIGN16(signed char bounding_values[256]);
  ptrdiff_t           ti[3][64];
  ptrdiff_t           ebi[3][64];
  ptrdiff_t           eob_runs[3][64];
  const ptrdiff_t    *coded_fragis[3];
  const ptrdiff_t    *uncoded_fragis[3];
  ptrdiff_t           ncoded_fragis[3];
  ptrdiff_t           nuncoded_fragis[3];
  const ogg_uint16_t *dequant[3][3][2];
  int                 fragy0[3];
  int                 fragy_end[3];
  int                 pred_last[3][4];
  int                 mcu_nvfrags;
  int                 loop_filter;
  int                 pp_level;
};


struct th_dec_ctx{
  
  oc_theora_state        state;
  



  int                    packet_state;
  
  oc_pack_buf            opb;
  
  ogg_int16_t           *huff_tables[TH_NHUFFMAN_TABLES];
  
  ptrdiff_t              ti0[3][64];
  

  ptrdiff_t              eob_runs[3][64];
  
  unsigned char         *dct_tokens;
  
  unsigned char         *extra_bits;
  
  int                    dct_tokens_count;
  
  int                    pp_level;
  
  int                    pp_dc_scale[64];
  
  int                    pp_sharp_mod[64];
  
  unsigned char         *dc_qis;
  
  int                   *variances;
  
  unsigned char         *pp_frame_data;
  
  int                    pp_frame_state;
  


  th_ycbcr_buffer        pp_frame_buf;
  
  th_stripe_callback     stripe_cb;
  oc_dec_pipeline_state  pipe;
# if defined(OC_DEC_USE_VTABLE)
  
  oc_dec_opt_vtable      opt_vtable;
# endif
# if defined(HAVE_CAIRO)
  
  int                    telemetry;
  int                    telemetry_mbmode;
  int                    telemetry_mv;
  int                    telemetry_qi;
  int                    telemetry_bits;
  int                    telemetry_frame_bytes;
  int                    telemetry_coding_bytes;
  int                    telemetry_mode_bytes;
  int                    telemetry_mv_bytes;
  int                    telemetry_qi_bytes;
  int                    telemetry_dc_bytes;
  unsigned char         *telemetry_frame_data;
# endif
};


void oc_dec_accel_init_c(oc_dec_ctx *_dec);

void oc_dec_dc_unpredict_mcu_plane_c(oc_dec_ctx *_dec,
 oc_dec_pipeline_state *_pipe,int _pli);

#endif
