















#if !defined(_encint_H)
# define _encint_H (1)
# if defined(HAVE_CONFIG_H)
#  include "config.h"
# endif
# include "theora/theoraenc.h"
# include "internal.h"
# include "ocintrin.h"
# include "mathops.h"
# include "enquant.h"
# include "huffenc.h"




typedef oc_mv                         oc_mv2[2];

typedef struct oc_enc_opt_vtable      oc_enc_opt_vtable;
typedef struct oc_mb_enc_info         oc_mb_enc_info;
typedef struct oc_mode_scheme_chooser oc_mode_scheme_chooser;
typedef struct oc_iir_filter          oc_iir_filter;
typedef struct oc_frame_metrics       oc_frame_metrics;
typedef struct oc_rc_state            oc_rc_state;
typedef struct th_enc_ctx             oc_enc_ctx;
typedef struct oc_token_checkpoint    oc_token_checkpoint;






#define OC_PACKET_EMPTY (0)

#define OC_PACKET_READY (1)


#define OC_SP_LEVEL_SLOW       (0)

#define OC_SP_LEVEL_EARLY_SKIP (1)

#define OC_SP_LEVEL_NOMC       (2)

#define OC_SP_LEVEL_MAX        (2)



extern const unsigned char OC_MODE_BITS[2][OC_NMODES];


extern const unsigned char OC_MV_BITS[2][64];



extern const ogg_uint16_t  OC_SB_RUN_VAL_MIN[8];

extern const unsigned char OC_SB_RUN_CODE_NBITS[7];


extern const unsigned char OC_BLOCK_RUN_CODE_NBITS[30];




struct oc_enc_opt_vtable{
  unsigned (*frag_sad)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride);
  unsigned (*frag_sad_thresh)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride,unsigned _thresh);
  unsigned (*frag_sad2_thresh)(const unsigned char *_src,
   const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
   unsigned _thresh);
  unsigned (*frag_satd_thresh)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride,unsigned _thresh);
  unsigned (*frag_satd2_thresh)(const unsigned char *_src,
   const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
   unsigned _thresh);
  unsigned (*frag_intra_satd)(const unsigned char *_src,int _ystride);
  void     (*frag_sub)(ogg_int16_t _diff[64],const unsigned char *_src,
   const unsigned char *_ref,int _ystride);
  void     (*frag_sub_128)(ogg_int16_t _diff[64],
   const unsigned char *_src,int _ystride);
  void     (*frag_copy2)(unsigned char *_dst,
   const unsigned char *_src1,const unsigned char *_src2,int _ystride);
  void     (*frag_recon_intra)(unsigned char *_dst,int _ystride,
   const ogg_int16_t _residue[64]);
  void     (*frag_recon_inter)(unsigned char *_dst,
   const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
  void     (*fdct8x8)(ogg_int16_t _y[64],const ogg_int16_t _x[64]);
};


void oc_enc_vtable_init(oc_enc_ctx *_enc);




struct oc_mb_enc_info{
  
  unsigned      cneighbors[4];
  
  unsigned      pneighbors[4];
  
  unsigned char ncneighbors;
  
  unsigned char npneighbors;
  
  unsigned char refined;
  





  oc_mv2        analysis_mv[3];
  
  oc_mv         unref_mv[2];
  
  oc_mv         block_mv[4];
  
  oc_mv         ref_mv[4];
  
  ogg_uint16_t  error[2];
  
  unsigned      satd[2];
  
  unsigned      block_satd[4];
};




struct oc_mode_scheme_chooser{
  



  const unsigned char *mode_ranks[8];
  

  unsigned char        scheme0_ranks[OC_NMODES];
  

  unsigned char        scheme0_list[OC_NMODES];
  
  int                  mode_counts[OC_NMODES];
  
  unsigned char        scheme_list[8];
  
  ptrdiff_t            scheme_bits[8];
};


void oc_mode_scheme_chooser_init(oc_mode_scheme_chooser *_chooser);






struct oc_iir_filter{
  ogg_int32_t c[2];
  ogg_int64_t g;
  ogg_int32_t x[2];
  ogg_int32_t y[2];
};




struct oc_frame_metrics{
  
  ogg_int32_t   log_scale;
  
  unsigned      dup_count:31;
  
  unsigned      frame_type:1;
};




struct oc_rc_state{
  
  ogg_int64_t        bits_per_frame;
  
  ogg_int64_t        fullness;
  


  ogg_int64_t        target;
  
  ogg_int64_t        max;
  
  ogg_int64_t        log_npixels;
  
  unsigned           exp[2];
  
  int                buf_delay;
  


  ogg_uint32_t       prev_drop_count;
  

  ogg_int64_t        log_drop_scale;
  
  ogg_int64_t        log_scale[2];
  
  ogg_int64_t        log_qtarget;
  
  unsigned char      drop_frames;
  
  unsigned char      cap_overflow;
  
  unsigned char      cap_underflow;
  
  oc_iir_filter      scalefilter[2];
  int                inter_count;
  int                inter_delay;
  int                inter_delay_target;
  oc_iir_filter      vfrfilter;
  



  int                twopass;
  
  unsigned char      twopass_buffer[48];
  


  int                twopass_buffer_bytes;
  int                twopass_buffer_fill;
  
  unsigned char      twopass_force_kf;
  
  oc_frame_metrics   prev_metrics;
  
  oc_frame_metrics   cur_metrics;
  
  oc_frame_metrics  *frame_metrics;
  int                nframe_metrics;
  int                cframe_metrics;
  
  int                frame_metrics_head;
  

  ogg_uint32_t       frames_total[3];
  
  ogg_uint32_t       frames_left[3];
  
  ogg_int64_t        scale_sum[2];
  
  int                scale_window0;
  
  int                scale_window_end;
  

  int                nframes[3];
  
  ogg_int64_t        rate_bias;
};


void oc_rc_state_init(oc_rc_state *_rc,oc_enc_ctx *_enc);
void oc_rc_state_clear(oc_rc_state *_rc);

void oc_enc_rc_resize(oc_enc_ctx *_enc);
int oc_enc_select_qi(oc_enc_ctx *_enc,int _qti,int _clamp);
void oc_enc_calc_lambda(oc_enc_ctx *_enc,int _frame_type);
int oc_enc_update_rc_state(oc_enc_ctx *_enc,
 long _bits,int _qti,int _qi,int _trial,int _droppable);
int oc_enc_rc_2pass_out(oc_enc_ctx *_enc,unsigned char **_buf);
int oc_enc_rc_2pass_in(oc_enc_ctx *_enc,unsigned char *_buf,size_t _bytes);




struct th_enc_ctx{
  
  oc_theora_state          state;
  
  oggpack_buffer           opb;
  
  oc_mb_enc_info          *mb_info;
  
  ogg_int16_t             *frag_dc;
  
  unsigned                *coded_mbis;
  
  size_t                   ncoded_mbis;
  



  int                      packet_state;
  
  ogg_uint32_t             keyframe_frequency_force;
  
  ogg_uint32_t             dup_count;
  
  ogg_uint32_t             nqueued_dups;
  
  ogg_uint32_t             prev_dup_count;
  
  int                      sp_level;
  
  unsigned char            vp3_compatible;
  
  unsigned char            coded_inter_frame;
  
  unsigned char            prevframe_dropped;
  



  unsigned char            huff_idxs[2][2][2];
  
  size_t                   mv_bits[2];
  
  oc_mode_scheme_chooser   chooser;
  
  int                      mcu_nvsbs;
  
  unsigned                *mcu_skip_ssd;
  
  unsigned char          **dct_tokens[3];
  
  ogg_uint16_t           **extra_bits[3];
  
  ptrdiff_t                ndct_tokens[3][64];
  
  ogg_uint16_t             eob_run[3][64];
  
  unsigned char            dct_token_offs[3][64];
  
  int                      dc_pred_last[3][3];
#if defined(OC_COLLECT_METRICS)
  
  unsigned                *frag_satd;
  
  unsigned                *frag_ssd;
#endif
  
  int                      lambda;
  
  th_huff_code             huff_codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS];
  
  th_quant_info            qinfo;
  oc_iquant               *enquant_tables[64][3][2];
  oc_iquant_table          enquant_table_data[64][3][2];
  





  ogg_int64_t              log_qavg[2][64];
  
  oc_rc_state              rc;
  
  oc_enc_opt_vtable        opt_vtable;
};


void oc_enc_analyze_intra(oc_enc_ctx *_enc,int _recode);
int oc_enc_analyze_inter(oc_enc_ctx *_enc,int _allow_keyframe,int _recode);
#if defined(OC_COLLECT_METRICS)
void oc_enc_mode_metrics_collect(oc_enc_ctx *_enc);
void oc_enc_mode_metrics_dump(oc_enc_ctx *_enc);
#endif




void oc_mcenc_search(oc_enc_ctx *_enc,int _mbi);

void oc_mcenc_refine1mv(oc_enc_ctx *_enc,int _mbi,int _frame);

void oc_mcenc_refine4mv(oc_enc_ctx *_enc,int _mbi);






struct oc_token_checkpoint{
  
  unsigned char pli;
  
  unsigned char zzi;
  
  ogg_uint16_t  eob_run;
  
  ptrdiff_t     ndct_tokens;
};



void oc_enc_tokenize_start(oc_enc_ctx *_enc);
int oc_enc_tokenize_ac(oc_enc_ctx *_enc,int _pli,ptrdiff_t _fragi,
 ogg_int16_t *_qdct,const ogg_uint16_t *_dequant,const ogg_int16_t *_dct,
 int _zzi,oc_token_checkpoint **_stack,int _acmin);
void oc_enc_tokenlog_rollback(oc_enc_ctx *_enc,
 const oc_token_checkpoint *_stack,int _n);
void oc_enc_pred_dc_frag_rows(oc_enc_ctx *_enc,
 int _pli,int _fragy0,int _frag_yend);
void oc_enc_tokenize_dc_frag_list(oc_enc_ctx *_enc,int _pli,
 const ptrdiff_t *_coded_fragis,ptrdiff_t _ncoded_fragis,
 int _prev_ndct_tokens1,int _prev_eob_run1);
void oc_enc_tokenize_finish(oc_enc_ctx *_enc);




int oc_state_flushheader(oc_theora_state *_state,int *_packet_state,
 oggpack_buffer *_opb,const th_quant_info *_qinfo,
 const th_huff_code _codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS],
 const char *_vendor,th_comment *_tc,ogg_packet *_op);




void oc_enc_frag_sub(const oc_enc_ctx *_enc,ogg_int16_t _diff[64],
 const unsigned char *_src,const unsigned char *_ref,int _ystride);
void oc_enc_frag_sub_128(const oc_enc_ctx *_enc,ogg_int16_t _diff[64],
 const unsigned char *_src,int _ystride);
unsigned oc_enc_frag_sad(const oc_enc_ctx *_enc,const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_sad_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_sad2_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref1,
 const unsigned char *_ref2,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_satd_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_satd2_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref1,
 const unsigned char *_ref2,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_intra_satd(const oc_enc_ctx *_enc,
 const unsigned char *_src,int _ystride);
void oc_enc_frag_copy2(const oc_enc_ctx *_enc,unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride);
void oc_enc_frag_recon_intra(const oc_enc_ctx *_enc,
 unsigned char *_dst,int _ystride,const ogg_int16_t _residue[64]);
void oc_enc_frag_recon_inter(const oc_enc_ctx *_enc,unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
void oc_enc_fdct8x8(const oc_enc_ctx *_enc,ogg_int16_t _y[64],
 const ogg_int16_t _x[64]);


void oc_enc_vtable_init_c(oc_enc_ctx *_enc);

void oc_enc_frag_sub_c(ogg_int16_t _diff[64],
 const unsigned char *_src,const unsigned char *_ref,int _ystride);
void oc_enc_frag_sub_128_c(ogg_int16_t _diff[64],
 const unsigned char *_src,int _ystride);
void oc_enc_frag_copy2_c(unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride);
unsigned oc_enc_frag_sad_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_sad_thresh_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_sad2_thresh_c(const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_satd_thresh_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_satd2_thresh_c(const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_intra_satd_c(const unsigned char *_src,int _ystride);
void oc_enc_fdct8x8_c(ogg_int16_t _y[64],const ogg_int16_t _x[64]);

#endif
