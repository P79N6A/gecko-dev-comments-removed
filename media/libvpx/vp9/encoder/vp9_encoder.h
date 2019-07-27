









#ifndef VP9_ENCODER_VP9_ENCODER_H_
#define VP9_ENCODER_VP9_ENCODER_H_

#include <stdio.h>

#include "./vpx_config.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "vpx/vp8cx.h"

#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_ppflags.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_thread_common.h"
#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_thread.h"

#include "vp9/encoder/vp9_aq_cyclicrefresh.h"
#include "vp9/encoder/vp9_context_tree.h"
#include "vp9/encoder/vp9_encodemb.h"
#include "vp9/encoder/vp9_firstpass.h"
#include "vp9/encoder/vp9_lookahead.h"
#include "vp9/encoder/vp9_mbgraph.h"
#include "vp9/encoder/vp9_mcomp.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/encoder/vp9_rd.h"
#include "vp9/encoder/vp9_speed_features.h"
#include "vp9/encoder/vp9_svc_layercontext.h"
#include "vp9/encoder/vp9_tokenize.h"
#include "vp9/encoder/vp9_variance.h"

#if CONFIG_VP9_TEMPORAL_DENOISING
#include "vp9/encoder/vp9_denoiser.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_GF_INTERVAL         10

typedef struct {
  int nmvjointcost[MV_JOINTS];
  int nmvcosts[2][MV_VALS];
  int nmvcosts_hp[2][MV_VALS];

  vp9_prob segment_pred_probs[PREDICTION_PROBS];

  unsigned char *last_frame_seg_map_copy;

  
  signed char last_ref_lf_deltas[MAX_REF_LF_DELTAS];
  
  signed char last_mode_lf_deltas[MAX_MODE_LF_DELTAS];

  FRAME_CONTEXT fc;
} CODING_CONTEXT;


typedef enum {
  
  ENCODE_BREAKOUT_DISABLED = 0,
  
  ENCODE_BREAKOUT_ENABLED = 1,
  
  ENCODE_BREAKOUT_LIMITED = 2
} ENCODE_BREAKOUT_TYPE;

typedef enum {
  NORMAL      = 0,
  FOURFIVE    = 1,
  THREEFIVE   = 2,
  ONETWO      = 3
} VPX_SCALING;

typedef enum {
  
  
  GOOD,

  
  
  
  BEST,

  
  
  
  REALTIME
} MODE;

typedef enum {
  FRAMEFLAGS_KEY    = 1 << 0,
  FRAMEFLAGS_GOLDEN = 1 << 1,
  FRAMEFLAGS_ALTREF = 1 << 2,
} FRAMETYPE_FLAGS;

typedef enum {
  NO_AQ = 0,
  VARIANCE_AQ = 1,
  COMPLEXITY_AQ = 2,
  CYCLIC_REFRESH_AQ = 3,
  AQ_MODE_COUNT  
} AQ_MODE;

typedef enum {
  RESIZE_NONE = 0,    
  RESIZE_FIXED = 1,   
  RESIZE_DYNAMIC = 2  
} RESIZE_TYPE;

typedef struct VP9EncoderConfig {
  BITSTREAM_PROFILE profile;
  vpx_bit_depth_t bit_depth;     
  int width;  
  int height;  
  unsigned int input_bit_depth;  
  double init_framerate;  
  int64_t target_bandwidth;  

  int noise_sensitivity;  
  int sharpness;  
  int speed;
  
  unsigned int rc_max_intra_bitrate_pct;
  
  unsigned int rc_max_inter_bitrate_pct;
  
  unsigned int gf_cbr_boost_pct;

  MODE mode;
  int pass;

  
  int auto_key;  
  int key_freq;  

  int lag_in_frames;  

  
  

  
  enum vpx_rc_mode rc_mode;

  
  int under_shoot_pct;
  int over_shoot_pct;

  
  int64_t starting_buffer_level_ms;
  int64_t optimal_buffer_level_ms;
  int64_t maximum_buffer_size_ms;

  
  int drop_frames_water_mark;

  
  int fixed_q;
  int worst_allowed_q;
  int best_allowed_q;
  int cq_level;
  AQ_MODE aq_mode;  

  
  RESIZE_TYPE resize_mode;
  int scaled_frame_width;
  int scaled_frame_height;

  
  int frame_periodic_boost;

  
  int two_pass_vbrbias;        
  int two_pass_vbrmin_section;
  int two_pass_vbrmax_section;
  
  

  
  int ss_number_layers;  
  int ts_number_layers;  
  
  int ss_target_bitrate[VPX_SS_MAX_LAYERS];
  int ss_enable_auto_arf[VPX_SS_MAX_LAYERS];
  
  int ts_target_bitrate[VPX_TS_MAX_LAYERS];
  int ts_rate_decimator[VPX_TS_MAX_LAYERS];

  int enable_auto_arf;

  int encode_breakout;  

  



  unsigned int error_resilient_mode;

  



  unsigned int frame_parallel_decoding_mode;

  int arnr_max_frames;
  int arnr_strength;

  int tile_columns;
  int tile_rows;

  int max_threads;

  vpx_fixed_buf_t two_pass_stats_in;
  struct vpx_codec_pkt_list *output_pkt_list;

#if CONFIG_FP_MB_STATS
  vpx_fixed_buf_t firstpass_mb_stats_in;
#endif

  vp8e_tuning tuning;
  vp9e_tune_content content;
#if CONFIG_VP9_HIGHBITDEPTH
  int use_highbitdepth;
#endif
  vpx_color_space_t color_space;
} VP9EncoderConfig;

static INLINE int is_lossless_requested(const VP9EncoderConfig *cfg) {
  return cfg->best_allowed_q == 0 && cfg->worst_allowed_q == 0;
}


typedef struct TileDataEnc {
  TileInfo tile_info;
  int thresh_freq_fact[BLOCK_SIZES][MAX_MODES];
  int mode_map[BLOCK_SIZES][MAX_MODES];
} TileDataEnc;

typedef struct RD_COUNTS {
  vp9_coeff_count coef_counts[TX_SIZES][PLANE_TYPES];
  int64_t comp_pred_diff[REFERENCE_MODES];
  int64_t tx_select_diff[TX_MODES];
  int64_t filter_diff[SWITCHABLE_FILTER_CONTEXTS];
} RD_COUNTS;

typedef struct ThreadData {
  MACROBLOCK mb;
  RD_COUNTS rd_counts;
  FRAME_COUNTS *counts;

  PICK_MODE_CONTEXT *leaf_tree;
  PC_TREE *pc_tree;
  PC_TREE *pc_root;
} ThreadData;

struct EncWorkerData;

typedef struct ActiveMap {
  int enabled;
  int update;
  unsigned char *map;
} ActiveMap;

typedef struct VP9_COMP {
  QUANTS quants;
  ThreadData td;
  DECLARE_ALIGNED(16, int16_t, y_dequant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_dequant[QINDEX_RANGE][8]);
  VP9_COMMON common;
  VP9EncoderConfig oxcf;
  struct lookahead_ctx    *lookahead;
  struct lookahead_entry  *alt_ref_source;

  YV12_BUFFER_CONFIG *Source;
  YV12_BUFFER_CONFIG *Last_Source;  
  YV12_BUFFER_CONFIG *un_scaled_source;
  YV12_BUFFER_CONFIG scaled_source;
  YV12_BUFFER_CONFIG *unscaled_last_source;
  YV12_BUFFER_CONFIG scaled_last_source;

  TileDataEnc *tile_data;

  
  int partition_search_skippable_frame;

  int scaled_ref_idx[MAX_REF_FRAMES];
  int lst_fb_idx;
  int gld_fb_idx;
  int alt_fb_idx;

  int refresh_last_frame;
  int refresh_golden_frame;
  int refresh_alt_ref_frame;

  int ext_refresh_frame_flags_pending;
  int ext_refresh_last_frame;
  int ext_refresh_golden_frame;
  int ext_refresh_alt_ref_frame;

  int ext_refresh_frame_context_pending;
  int ext_refresh_frame_context;

  YV12_BUFFER_CONFIG last_frame_uf;

  TOKENEXTRA *tile_tok[4][1 << 6];
  unsigned int tok_count[4][1 << 6];

  
  int64_t ambient_err;

  RD_OPT rd;

  CODING_CONTEXT coding_context;

  int *nmvcosts[2];
  int *nmvcosts_hp[2];
  int *nmvsadcosts[2];
  int *nmvsadcosts_hp[2];

  int64_t last_time_stamp_seen;
  int64_t last_end_time_stamp_seen;
  int64_t first_time_stamp_ever;

  RATE_CONTROL rc;
  double framerate;

  int interp_filter_selected[MAX_REF_FRAMES][SWITCHABLE];

  struct vpx_codec_pkt_list  *output_pkt_list;

  MBGRAPH_FRAME_STATS mbgraph_stats[MAX_LAG_BUFFERS];
  int mbgraph_n_frames;             
  int static_mb_pct;                
  int ref_frame_flags;

  SPEED_FEATURES sf;

  unsigned int max_mv_magnitude;
  int mv_step_param;

  int allow_comp_inter_inter;

  
  ENCODE_BREAKOUT_TYPE allow_encode_breakout;

  
  
  int encode_breakout;

  unsigned char *segmentation_map;

  
  int  segment_encode_breakout[MAX_SEGMENTS];

  CYCLIC_REFRESH *cyclic_refresh;
  ActiveMap active_map;

  fractional_mv_step_fp *find_fractional_mv_step;
  vp9_full_search_fn_t full_search_sad;
  vp9_diamond_search_fn_t diamond_search_sad;
  vp9_variance_fn_ptr_t fn_ptr[BLOCK_SIZES];
  uint64_t time_receive_data;
  uint64_t time_compress_data;
  uint64_t time_pick_lpf;
  uint64_t time_encode_sb_row;

#if CONFIG_FP_MB_STATS
  int use_fp_mb_stats;
#endif

  TWO_PASS twopass;

  YV12_BUFFER_CONFIG alt_ref_buffer;


#if CONFIG_INTERNAL_STATS
  unsigned int mode_chosen_counts[MAX_MODES];

  int    count;
  double total_y;
  double total_u;
  double total_v;
  double total;
  uint64_t total_sq_error;
  uint64_t total_samples;

  double totalp_y;
  double totalp_u;
  double totalp_v;
  double totalp;
  uint64_t totalp_sq_error;
  uint64_t totalp_samples;

  int    bytes;
  double summed_quality;
  double summed_weights;
  double summedp_quality;
  double summedp_weights;
  unsigned int tot_recode_hits;


  double total_ssimg_y;
  double total_ssimg_u;
  double total_ssimg_v;
  double total_ssimg_all;

  int b_calculate_ssimg;
#endif
  int b_calculate_psnr;

  int droppable;

  int initial_width;
  int initial_height;
  int initial_mbs;  
                    
                    
                    

  int use_svc;

  SVC svc;

  
  diff *source_diff_var;
  
  unsigned int source_var_thresh;
  int frames_till_next_var_check;

  int frame_flags;

  search_site_config ss_cfg;

  int mbmode_cost[INTRA_MODES];
  unsigned int inter_mode_cost[INTER_MODE_CONTEXTS][INTER_MODES];
  int intra_uv_mode_cost[FRAME_TYPES][INTRA_MODES];
  int y_mode_costs[INTRA_MODES][INTRA_MODES][INTRA_MODES];
  int switchable_interp_costs[SWITCHABLE_FILTER_CONTEXTS][SWITCHABLE_FILTERS];
  int partition_cost[PARTITION_CONTEXTS][PARTITION_TYPES];

  int multi_arf_allowed;
  int multi_arf_enabled;
  int multi_arf_last_grp_enabled;

#if CONFIG_VP9_TEMPORAL_DENOISING
  VP9_DENOISER denoiser;
#endif

  int resize_pending;

  
  int64_t vbp_threshold;
  int64_t vbp_threshold_bsize_min;
  int64_t vbp_threshold_bsize_max;
  int64_t vbp_threshold_16x16;
  BLOCK_SIZE vbp_bsize_min;

  
  int num_workers;
  VP9Worker *workers;
  struct EncWorkerData *tile_thr_data;
  VP9LfSync lf_row_sync;
} VP9_COMP;

void vp9_initialize_enc(void);

struct VP9_COMP *vp9_create_compressor(VP9EncoderConfig *oxcf,
                                       BufferPool *const pool);
void vp9_remove_compressor(VP9_COMP *cpi);

void vp9_change_config(VP9_COMP *cpi, const VP9EncoderConfig *oxcf);

  
  
int vp9_receive_raw_frame(VP9_COMP *cpi, unsigned int frame_flags,
                          YV12_BUFFER_CONFIG *sd, int64_t time_stamp,
                          int64_t end_time_stamp);

int vp9_get_compressed_data(VP9_COMP *cpi, unsigned int *frame_flags,
                            size_t *size, uint8_t *dest,
                            int64_t *time_stamp, int64_t *time_end, int flush);

int vp9_get_preview_raw_frame(VP9_COMP *cpi, YV12_BUFFER_CONFIG *dest,
                              vp9_ppflags_t *flags);

int vp9_use_as_reference(VP9_COMP *cpi, int ref_frame_flags);

void vp9_update_reference(VP9_COMP *cpi, int ref_frame_flags);

int vp9_copy_reference_enc(VP9_COMP *cpi, VP9_REFFRAME ref_frame_flag,
                           YV12_BUFFER_CONFIG *sd);

int vp9_set_reference_enc(VP9_COMP *cpi, VP9_REFFRAME ref_frame_flag,
                          YV12_BUFFER_CONFIG *sd);

int vp9_update_entropy(VP9_COMP *cpi, int update);

int vp9_set_active_map(VP9_COMP *cpi, unsigned char *map, int rows, int cols);

int vp9_set_internal_size(VP9_COMP *cpi,
                          VPX_SCALING horiz_mode, VPX_SCALING vert_mode);

int vp9_set_size_literal(VP9_COMP *cpi, unsigned int width,
                         unsigned int height);

void vp9_set_svc(VP9_COMP *cpi, int use_svc);

int vp9_get_quantizer(struct VP9_COMP *cpi);

static INLINE int frame_is_kf_gf_arf(const VP9_COMP *cpi) {
  return frame_is_intra_only(&cpi->common) ||
         cpi->refresh_alt_ref_frame ||
         (cpi->refresh_golden_frame && !cpi->rc.is_src_frame_alt_ref);
}

static INLINE int get_ref_frame_map_idx(const VP9_COMP *cpi,
                                        MV_REFERENCE_FRAME ref_frame) {
  if (ref_frame == LAST_FRAME) {
    return cpi->lst_fb_idx;
  } else if (ref_frame == GOLDEN_FRAME) {
    return cpi->gld_fb_idx;
  } else {
    return cpi->alt_fb_idx;
  }
}

static INLINE int get_ref_frame_buf_idx(const VP9_COMP *const cpi,
                                        int ref_frame) {
  const VP9_COMMON *const cm = &cpi->common;
  const int map_idx = get_ref_frame_map_idx(cpi, ref_frame);
  return (map_idx != INVALID_IDX) ? cm->ref_frame_map[map_idx] : INVALID_IDX;
}

static INLINE YV12_BUFFER_CONFIG *get_ref_frame_buffer(
    VP9_COMP *cpi, MV_REFERENCE_FRAME ref_frame) {
  VP9_COMMON *const cm = &cpi->common;
  const int buf_idx = get_ref_frame_buf_idx(cpi, ref_frame);
  return
      buf_idx != INVALID_IDX ? &cm->buffer_pool->frame_bufs[buf_idx].buf : NULL;
}

static INLINE int get_token_alloc(int mb_rows, int mb_cols) {
  
  
  
  
  
  return mb_rows * mb_cols * (16 * 16 * 3 + 4);
}



static INLINE int allocated_tokens(TileInfo tile) {
  int tile_mb_rows = (tile.mi_row_end - tile.mi_row_start + 1) >> 1;
  int tile_mb_cols = (tile.mi_col_end - tile.mi_col_start + 1) >> 1;

  return get_token_alloc(tile_mb_rows, tile_mb_cols);
}

int64_t vp9_get_y_sse(const YV12_BUFFER_CONFIG *a, const YV12_BUFFER_CONFIG *b);
#if CONFIG_VP9_HIGHBITDEPTH
int64_t vp9_highbd_get_y_sse(const YV12_BUFFER_CONFIG *a,
                             const YV12_BUFFER_CONFIG *b);
#endif  

void vp9_alloc_compressor_data(VP9_COMP *cpi);

void vp9_scale_references(VP9_COMP *cpi);

void vp9_update_reference_frames(VP9_COMP *cpi);

void vp9_set_high_precision_mv(VP9_COMP *cpi, int allow_high_precision_mv);

YV12_BUFFER_CONFIG *vp9_scale_if_required(VP9_COMMON *cm,
                                          YV12_BUFFER_CONFIG *unscaled,
                                          YV12_BUFFER_CONFIG *scaled);

void vp9_apply_encoding_flags(VP9_COMP *cpi, vpx_enc_frame_flags_t flags);

static INLINE int is_two_pass_svc(const struct VP9_COMP *const cpi) {
  return cpi->use_svc &&
         ((cpi->svc.number_spatial_layers > 1) ||
         (cpi->svc.number_temporal_layers > 1 && cpi->oxcf.pass != 0));
}

static INLINE int is_altref_enabled(const VP9_COMP *const cpi) {
  return cpi->oxcf.mode != REALTIME && cpi->oxcf.lag_in_frames > 0 &&
         (cpi->oxcf.enable_auto_arf &&
          (!is_two_pass_svc(cpi) ||
           cpi->oxcf.ss_enable_auto_arf[cpi->svc.spatial_layer_id]));
}

static INLINE void set_ref_ptrs(VP9_COMMON *cm, MACROBLOCKD *xd,
                                MV_REFERENCE_FRAME ref0,
                                MV_REFERENCE_FRAME ref1) {
  xd->block_refs[0] = &cm->frame_refs[ref0 >= LAST_FRAME ? ref0 - LAST_FRAME
                                                         : 0];
  xd->block_refs[1] = &cm->frame_refs[ref1 >= LAST_FRAME ? ref1 - LAST_FRAME
                                                         : 0];
}

static INLINE int get_chessboard_index(const int frame_index) {
  return frame_index & 0x1;
}

static INLINE int *cond_cost_list(const struct VP9_COMP *cpi, int *cost_list) {
  return cpi->sf.mv.subpel_search_method != SUBPEL_TREE ? cost_list : NULL;
}

void vp9_new_framerate(VP9_COMP *cpi, double framerate);

#ifdef __cplusplus
}  
#endif

#endif
