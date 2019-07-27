









#ifndef VP9_ENCODER_VP9_ONYX_INT_H_
#define VP9_ENCODER_VP9_ONYX_INT_H_

#include <stdio.h>

#include "./vpx_config.h"
#include "vpx_ports/mem.h"
#include "vpx/internal/vpx_codec_internal.h"

#include "vp9/common/vp9_entropy.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_onyx.h"
#include "vp9/common/vp9_onyxc_int.h"

#include "vp9/encoder/vp9_encodemb.h"
#include "vp9/encoder/vp9_firstpass.h"
#include "vp9/encoder/vp9_lookahead.h"
#include "vp9/encoder/vp9_mbgraph.h"
#include "vp9/encoder/vp9_mcomp.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/encoder/vp9_tokenize.h"
#include "vp9/encoder/vp9_treewriter.h"
#include "vp9/encoder/vp9_variance.h"

#ifdef __cplusplus
extern "C" {
#endif



#if CONFIG_MULTIPLE_ARF

#define MIN_GF_INTERVAL             2
#else
#define MIN_GF_INTERVAL             4
#endif
#define DEFAULT_GF_INTERVAL         10
#define DEFAULT_KF_BOOST            2000
#define DEFAULT_GF_BOOST            2000

#define KEY_FRAME_CONTEXT 5

#define MAX_MODES 30
#define MAX_REFS  6

#define MIN_THRESHMULT  32
#define MAX_THRESHMULT  512

#define GF_ZEROMV_ZBIN_BOOST 0
#define LF_ZEROMV_ZBIN_BOOST 0
#define MV_ZBIN_BOOST        0
#define SPLIT_MV_ZBIN_BOOST  0
#define INTRA_ZBIN_BOOST     0

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
  THR_NEARESTMV,
  THR_NEARESTA,
  THR_NEARESTG,

  THR_DC,

  THR_NEWMV,
  THR_NEWA,
  THR_NEWG,

  THR_NEARMV,
  THR_NEARA,
  THR_COMP_NEARESTLA,
  THR_COMP_NEARESTGA,

  THR_TM,

  THR_COMP_NEARLA,
  THR_COMP_NEWLA,
  THR_NEARG,
  THR_COMP_NEARGA,
  THR_COMP_NEWGA,

  THR_ZEROMV,
  THR_ZEROG,
  THR_ZEROA,
  THR_COMP_ZEROLA,
  THR_COMP_ZEROGA,

  THR_H_PRED,
  THR_V_PRED,
  THR_D135_PRED,
  THR_D207_PRED,
  THR_D153_PRED,
  THR_D63_PRED,
  THR_D117_PRED,
  THR_D45_PRED,
} THR_MODES;

typedef enum {
  THR_LAST,
  THR_GOLD,
  THR_ALTR,
  THR_COMP_LA,
  THR_COMP_GA,
  THR_INTRA,
} THR_MODES_SUB8X8;

typedef enum {
  DIAMOND = 0,
  NSTEP = 1,
  HEX = 2,
  BIGDIA = 3,
  SQUARE = 4
} SEARCH_METHODS;

typedef enum {
  USE_FULL_RD = 0,
  USE_LARGESTINTRA,
  USE_LARGESTINTRA_MODELINTER,
  USE_LARGESTALL
} TX_SIZE_SEARCH_METHOD;

typedef enum {
  NOT_IN_USE = 0,
  RELAXED_NEIGHBORING_MIN_MAX = 1,
  STRICT_NEIGHBORING_MIN_MAX = 2
} AUTO_MIN_MAX_MODE;

typedef enum {
  
  

  
  
  FLAG_EARLY_TERMINATE = 1,

  
  FLAG_SKIP_COMP_BESTINTRA = 2,

  
  
  
  FLAG_SKIP_COMP_REFMISMATCH = 4,

  
  FLAG_SKIP_INTRA_BESTINTER = 8,

  
  
  FLAG_SKIP_INTRA_DIRMISMATCH = 16,

  
  
  FLAG_SKIP_INTRA_LOWVAR = 32,
} MODE_SEARCH_SKIP_LOGIC;

typedef enum {
  SUBPEL_TREE = 0,
  
} SUBPEL_SEARCH_METHODS;

#define ALL_INTRA_MODES 0x3FF
#define INTRA_DC_ONLY 0x01
#define INTRA_DC_TM ((1 << TM_PRED) | (1 << DC_PRED))
#define INTRA_DC_H_V ((1 << DC_PRED) | (1 << V_PRED) | (1 << H_PRED))
#define INTRA_DC_TM_H_V (INTRA_DC_TM | (1 << V_PRED) | (1 << H_PRED))

typedef enum {
  LAST_FRAME_PARTITION_OFF = 0,
  LAST_FRAME_PARTITION_LOW_MOTION = 1,
  LAST_FRAME_PARTITION_ALL = 2
} LAST_FRAME_PARTITION_METHOD;

typedef enum {
  
  DISALLOW_RECODE = 0,
  
  ALLOW_RECODE_KFMAXBW = 1,
  
  ALLOW_RECODE_KFARFGF = 2,
  
  ALLOW_RECODE = 3,
} RECODE_LOOP_TYPE;

typedef enum {
  
  ENCODE_BREAKOUT_DISABLED = 0,
  
  ENCODE_BREAKOUT_ENABLED = 1,
  
  ENCODE_BREAKOUT_LIMITED = 2
} ENCODE_BREAKOUT_TYPE;

typedef struct {
  
  int frame_parameter_update;

  
  SEARCH_METHODS search_method;

  RECODE_LOOP_TYPE recode_loop;

  
  
  
  
  SUBPEL_SEARCH_METHODS subpel_search_method;

  
  int subpel_iters_per_step;

  
  int subpel_force_stop;

  
  
  
  
  int thresh_mult[MAX_MODES];
  int thresh_mult_sub8x8[MAX_REFS];

  
  
  int max_step_search_steps;

  
  
  int reduce_first_step_size;

  
  
  int auto_mv_step_size;

  
  int optimize_coefficients;

  
  
  
  
  
  int static_segmentation;

  
  
  
  
  int comp_inter_joint_search_thresh;

  
  
  int adaptive_rd_thresh;

  
  
  
  int skip_encode_sb;
  int skip_encode_frame;

  
  
  
  
  
  
  
  LAST_FRAME_PARTITION_METHOD use_lastframe_partitioning;

  
  
  
  TX_SIZE_SEARCH_METHOD tx_size_search_method;

  
  
  int use_lp32x32fdct;

  

  
  int use_one_partition_size_always;

  
  
  int less_rectangular_check;

  
  int use_square_partition_only;

  
  
  
  int mode_skip_start;

  
  int reference_masking;

  
  BLOCK_SIZE always_this_block_size;

  
  
  AUTO_MIN_MAX_MODE auto_min_max_partition_size;

  
  
  BLOCK_SIZE min_partition_size;
  BLOCK_SIZE max_partition_size;

  
  
  int adjust_partitioning_from_last_frame;

  
  
  int last_partitioning_redo_frequency;

  
  
  
  int disable_split_mask;

  
  
  
  int adaptive_motion_search;

  
  
  
  
  int adaptive_pred_interp_filter;

  
  
  
  unsigned int mode_search_skip_flags;

  
  unsigned int disable_split_var_thresh;

  
  
  unsigned int disable_filter_search_var_thresh;

  
  
  int intra_y_mode_mask[TX_SIZES];
  int intra_uv_mode_mask[TX_SIZES];

  
  
  
  int use_rd_breakout;

  
  
  
  int use_uv_intra_rd_estimate;

  
  
  
  
  int use_fast_lpf_pick;

  
  
  int use_fast_coef_updates;  

  
  int use_pick_mode;

  
  
  int encode_breakout_thresh;
} SPEED_FEATURES;

typedef struct {
  RATE_CONTROL rc;
  int target_bandwidth;
  int64_t starting_buffer_level;
  int64_t optimal_buffer_level;
  int64_t maximum_buffer_size;
  double framerate;
  int avg_frame_size;
} LAYER_CONTEXT;

typedef struct VP9_COMP {
  DECLARE_ALIGNED(16, int16_t, y_quant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, y_quant_shift[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, y_zbin[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, y_round[QINDEX_RANGE][8]);

  DECLARE_ALIGNED(16, int16_t, uv_quant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_quant_shift[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_zbin[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_round[QINDEX_RANGE][8]);

#if CONFIG_ALPHA
  DECLARE_ALIGNED(16, int16_t, a_quant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, a_quant_shift[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, a_zbin[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, a_round[QINDEX_RANGE][8]);
#endif

  MACROBLOCK mb;
  VP9_COMMON common;
  VP9_CONFIG oxcf;
  struct lookahead_ctx    *lookahead;
  struct lookahead_entry  *source;
#if CONFIG_MULTIPLE_ARF
  struct lookahead_entry  *alt_ref_source[REF_FRAMES];
#else
  struct lookahead_entry  *alt_ref_source;
#endif

  YV12_BUFFER_CONFIG *Source;
  YV12_BUFFER_CONFIG *un_scaled_source;
  YV12_BUFFER_CONFIG scaled_source;

  unsigned int key_frame_frequency;

  int gold_is_last;  
  int alt_is_last;  
  int gold_is_alt;  

  int scaled_ref_idx[3];
  int lst_fb_idx;
  int gld_fb_idx;
  int alt_fb_idx;

#if CONFIG_MULTIPLE_ARF
  int alt_ref_fb_idx[REF_FRAMES - 3];
#endif
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

  TOKENEXTRA *tok;
  unsigned int tok_count[4][1 << 6];

#if CONFIG_MULTIPLE_ARF
  
  unsigned int sequence_number;
  
  int next_frame_in_order;
#endif

  
  int ambient_err;

  unsigned int mode_chosen_counts[MAX_MODES];
  unsigned int sub8x8_mode_chosen_counts[MAX_REFS];
  int64_t mode_skip_mask;
  int ref_frame_mask;
  int set_ref_frame_mask;

  int rd_threshes[MAX_SEGMENTS][BLOCK_SIZES][MAX_MODES];
  int rd_thresh_freq_fact[BLOCK_SIZES][MAX_MODES];
  int rd_thresh_sub8x8[MAX_SEGMENTS][BLOCK_SIZES][MAX_REFS];
  int rd_thresh_freq_sub8x8[BLOCK_SIZES][MAX_REFS];

  int64_t rd_comp_pred_diff[REFERENCE_MODES];
  int64_t rd_prediction_type_threshes[4][REFERENCE_MODES];
  int64_t rd_tx_select_diff[TX_MODES];
  
  int rd_tx_select_threshes[4][TX_MODES];

  int64_t rd_filter_diff[SWITCHABLE_FILTER_CONTEXTS];
  int64_t rd_filter_threshes[4][SWITCHABLE_FILTER_CONTEXTS];
  int64_t rd_filter_cache[SWITCHABLE_FILTER_CONTEXTS];
  int64_t mask_filter_rd;

  int RDMULT;
  int RDDIV;

  CODING_CONTEXT coding_context;

  int zbin_mode_boost;
  int zbin_mode_boost_enabled;
  int active_arnr_frames;           
  int active_arnr_strength;         

  double output_framerate;
  int64_t last_time_stamp_seen;
  int64_t last_end_time_stamp_seen;
  int64_t first_time_stamp_ever;

  RATE_CONTROL rc;

  int cq_target_quality;

  vp9_coeff_count coef_counts[TX_SIZES][PLANE_TYPES];
  vp9_coeff_probs_model frame_coef_probs[TX_SIZES][PLANE_TYPES];
  vp9_coeff_stats frame_branch_ct[TX_SIZES][PLANE_TYPES];

  struct vpx_codec_pkt_list  *output_pkt_list;

  MBGRAPH_FRAME_STATS mbgraph_stats[MAX_LAG_BUFFERS];
  int mbgraph_n_frames;             
  int static_mb_pct;                
  int seg0_progress, seg0_idx, seg0_cnt;

  
  int speed;

  int cpu_used;
  int pass;

  vp9_prob last_skip_false_probs[3][SKIP_CONTEXTS];
  int last_skip_probs_q[3];

  int ref_frame_flags;

  SPEED_FEATURES sf;

  unsigned int max_mv_magnitude;
  int mv_step_param;

  
  ENCODE_BREAKOUT_TYPE allow_encode_breakout;

  
  
  int encode_breakout;

  unsigned char *segmentation_map;

  
  int  segment_encode_breakout[MAX_SEGMENTS];

  unsigned char *complexity_map;

  unsigned char *active_map;
  unsigned int active_map_enabled;

  fractional_mv_step_fp *find_fractional_mv_step;
  fractional_mv_step_comp_fp *find_fractional_mv_step_comp;
  vp9_full_search_fn_t full_search_sad;
  vp9_refining_search_fn_t refining_search_sad;
  vp9_diamond_search_fn_t diamond_search_sad;
  vp9_variance_fn_ptr_t fn_ptr[BLOCK_SIZES];
  uint64_t time_receive_data;
  uint64_t time_compress_data;
  uint64_t time_pick_lpf;
  uint64_t time_encode_sb_row;

  struct twopass_rc twopass;

  YV12_BUFFER_CONFIG alt_ref_buffer;
  YV12_BUFFER_CONFIG *frames[MAX_LAG_BUFFERS];
  int fixed_divide[512];

#if CONFIG_INTERNAL_STATS
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

  
  unsigned int activity_avg;
  unsigned int *mb_activity_map;
  int *mb_norm_activity_map;
  int output_partition;

  
  int force_next_frame_intra;

  int droppable;

  int dummy_packing;    

  unsigned int tx_stepdown_count[TX_SIZES];

  int initial_width;
  int initial_height;

  int use_svc;

  struct svc {
    int spatial_layer_id;
    int temporal_layer_id;
    int number_spatial_layers;
    int number_temporal_layers;
    
    
    LAYER_CONTEXT layer_context[VPX_TS_MAX_LAYERS];
  } svc;

#if CONFIG_MULTIPLE_ARF
  
  int multi_arf_enabled;
  unsigned int frame_coding_order_period;
  unsigned int new_frame_coding_order_period;
  int frame_coding_order[MAX_LAG_BUFFERS * 2];
  int arf_buffer_idx[MAX_LAG_BUFFERS * 3 / 2];
  int arf_weight[MAX_LAG_BUFFERS];
  int arf_buffered;
  int this_frame_weight;
  int max_arf_level;
#endif

#ifdef ENTROPY_STATS
  int64_t mv_ref_stats[INTER_MODE_CONTEXTS][INTER_MODES - 1][2];
#endif


#ifdef MODE_TEST_HIT_STATS
  
  int64_t mode_test_hits[BLOCK_SIZES];
#endif

  
  ENTROPY_CONTEXT *above_context[MAX_MB_PLANE];
  ENTROPY_CONTEXT left_context[MAX_MB_PLANE][16];

  PARTITION_CONTEXT *above_seg_context;
  PARTITION_CONTEXT left_seg_context[8];
} VP9_COMP;

static int get_ref_frame_idx(const VP9_COMP *cpi,
                             MV_REFERENCE_FRAME ref_frame) {
  if (ref_frame == LAST_FRAME) {
    return cpi->lst_fb_idx;
  } else if (ref_frame == GOLDEN_FRAME) {
    return cpi->gld_fb_idx;
  } else {
    return cpi->alt_fb_idx;
  }
}

static YV12_BUFFER_CONFIG *get_ref_frame_buffer(VP9_COMP *cpi,
                                                MV_REFERENCE_FRAME ref_frame) {
  VP9_COMMON *const cm = &cpi->common;
  return &cm->frame_bufs[cm->ref_frame_map[get_ref_frame_idx(cpi,
                                                             ref_frame)]].buf;
}

void vp9_encode_frame(VP9_COMP *cpi);

void vp9_pack_bitstream(VP9_COMP *cpi, uint8_t *dest, size_t *size);

void vp9_set_speed_features(VP9_COMP *cpi);

int vp9_calc_ss_err(const YV12_BUFFER_CONFIG *source,
                    const YV12_BUFFER_CONFIG *reference);

void vp9_alloc_compressor_data(VP9_COMP *cpi);

int vp9_compute_qdelta(const VP9_COMP *cpi, double qstart, double qtarget);

static int get_token_alloc(int mb_rows, int mb_cols) {
  return mb_rows * mb_cols * (48 * 16 + 4);
}

static void set_ref_ptrs(VP9_COMMON *cm, MACROBLOCKD *xd,
                         MV_REFERENCE_FRAME ref0, MV_REFERENCE_FRAME ref1) {
  xd->block_refs[0] = &cm->frame_refs[ref0 >= LAST_FRAME ? ref0 - LAST_FRAME
                                                         : 0];
  xd->block_refs[1] = &cm->frame_refs[ref1 >= LAST_FRAME ? ref1 - LAST_FRAME
                                                         : 0];
}

#ifdef __cplusplus
}  
#endif

#endif
