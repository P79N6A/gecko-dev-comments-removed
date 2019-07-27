









#ifndef VP9_COMMON_VP9_ONYXC_INT_H_
#define VP9_COMMON_VP9_ONYXC_INT_H_

#include "./vpx_config.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "./vp9_rtcd.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_frame_buffers.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_tile_common.h"

#if CONFIG_VP9_POSTPROC
#include "vp9/common/vp9_postproc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define REFS_PER_FRAME 3

#define REF_FRAMES_LOG2 3
#define REF_FRAMES (1 << REF_FRAMES_LOG2)




#define FRAME_BUFFERS (REF_FRAMES + 4)

#define FRAME_CONTEXTS_LOG2 2
#define FRAME_CONTEXTS (1 << FRAME_CONTEXTS_LOG2)

extern const struct {
  PARTITION_CONTEXT above;
  PARTITION_CONTEXT left;
} partition_context_lookup[BLOCK_SIZES];


typedef enum {
  SINGLE_REFERENCE      = 0,
  COMPOUND_REFERENCE    = 1,
  REFERENCE_MODE_SELECT = 2,
  REFERENCE_MODES       = 3,
} REFERENCE_MODE;


typedef struct {
  int ref_count;
  vpx_codec_frame_buffer_t raw_frame_buffer;
  YV12_BUFFER_CONFIG buf;
} RefCntBuffer;

typedef struct VP9Common {
  struct vpx_internal_error_info  error;

  DECLARE_ALIGNED(16, int16_t, y_dequant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_dequant[QINDEX_RANGE][8]);

  COLOR_SPACE color_space;

  int width;
  int height;
  int display_width;
  int display_height;
  int last_width;
  int last_height;

  
  
  
  int subsampling_x;
  int subsampling_y;

#if CONFIG_VP9_HIGHBITDEPTH
  int use_highbitdepth;  
#endif

  YV12_BUFFER_CONFIG *frame_to_show;

  RefCntBuffer frame_bufs[FRAME_BUFFERS];

  int ref_frame_map[REF_FRAMES]; 

  
  

  
  RefBuffer frame_refs[REFS_PER_FRAME];

  int new_fb_idx;

  YV12_BUFFER_CONFIG post_proc_buffer;

  FRAME_TYPE last_frame_type;  
  FRAME_TYPE frame_type;

  int show_frame;
  int last_show_frame;
  int show_existing_frame;

  
  int intra_only;

  int allow_high_precision_mv;

  
  
  
  int reset_frame_context;

  
  
  int MBs;
  int mb_rows, mi_rows;
  int mb_cols, mi_cols;
  int mi_stride;

  
  TX_MODE tx_mode;

  int base_qindex;
  int y_dc_delta_q;
  int uv_dc_delta_q;
  int uv_ac_delta_q;

  


  int mi_idx;
  int prev_mi_idx;
  int mi_alloc_size;
  MODE_INFO *mip_array[2];
  MODE_INFO **mi_grid_base_array[2];

  MODE_INFO *mip; 
  MODE_INFO *mi;  
  MODE_INFO *prev_mip; 
  MODE_INFO *prev_mi;  

  
  unsigned char *last_frame_seg_map;

  INTERP_FILTER interp_filter;

  loop_filter_info_n lf_info;

  int refresh_frame_context;    

  int ref_frame_sign_bias[MAX_REF_FRAMES];    

  struct loopfilter lf;
  struct segmentation seg;

  
  int allow_comp_inter_inter;
  MV_REFERENCE_FRAME comp_fixed_ref;
  MV_REFERENCE_FRAME comp_var_ref[2];
  REFERENCE_MODE reference_mode;

  FRAME_CONTEXT fc;  
  FRAME_CONTEXT frame_contexts[FRAME_CONTEXTS];
  unsigned int  frame_context_idx; 
  FRAME_COUNTS counts;

  unsigned int current_video_frame;
  BITSTREAM_PROFILE profile;

  
  vpx_bit_depth_t bit_depth;

#if CONFIG_VP9_POSTPROC
  struct postproc_state  postproc_state;
#endif

  int error_resilient_mode;
  int frame_parallel_decoding_mode;

  int log2_tile_cols, log2_tile_rows;

  
  void *cb_priv;
  vpx_get_frame_buffer_cb_fn_t get_fb_cb;
  vpx_release_frame_buffer_cb_fn_t release_fb_cb;

  
  InternalFrameBufferList int_frame_buffers;

  PARTITION_CONTEXT *above_seg_context;
  ENTROPY_CONTEXT *above_context;
} VP9_COMMON;

static INLINE YV12_BUFFER_CONFIG *get_ref_frame(VP9_COMMON *cm, int index) {
  if (index < 0 || index >= REF_FRAMES)
    return NULL;
  if (cm->ref_frame_map[index] < 0)
    return NULL;
  assert(cm->ref_frame_map[index] < FRAME_BUFFERS);
  return &cm->frame_bufs[cm->ref_frame_map[index]].buf;
}

static INLINE YV12_BUFFER_CONFIG *get_frame_new_buffer(VP9_COMMON *cm) {
  return &cm->frame_bufs[cm->new_fb_idx].buf;
}

static INLINE int get_free_fb(VP9_COMMON *cm) {
  int i;
  for (i = 0; i < FRAME_BUFFERS; i++)
    if (cm->frame_bufs[i].ref_count == 0)
      break;

  assert(i < FRAME_BUFFERS);
  cm->frame_bufs[i].ref_count = 1;
  return i;
}

static INLINE void ref_cnt_fb(RefCntBuffer *bufs, int *idx, int new_idx) {
  const int ref_index = *idx;

  if (ref_index >= 0 && bufs[ref_index].ref_count > 0)
    bufs[ref_index].ref_count--;

  *idx = new_idx;

  bufs[new_idx].ref_count++;
}

static INLINE int mi_cols_aligned_to_sb(int n_mis) {
  return ALIGN_POWER_OF_TWO(n_mis, MI_BLOCK_SIZE_LOG2);
}

static INLINE void init_macroblockd(VP9_COMMON *cm, MACROBLOCKD *xd) {
  int i;

  for (i = 0; i < MAX_MB_PLANE; ++i) {
    xd->plane[i].dqcoeff = xd->dqcoeff[i];
    xd->above_context[i] = cm->above_context +
        i * sizeof(*cm->above_context) * 2 * mi_cols_aligned_to_sb(cm->mi_cols);
  }

  xd->above_seg_context = cm->above_seg_context;
  xd->mi_stride = cm->mi_stride;
}

static INLINE int frame_is_intra_only(const VP9_COMMON *const cm) {
  return cm->frame_type == KEY_FRAME || cm->intra_only;
}

static INLINE const vp9_prob* get_partition_probs(const VP9_COMMON *cm,
                                                  int ctx) {
  return frame_is_intra_only(cm) ? vp9_kf_partition_probs[ctx]
                                 : cm->fc.partition_prob[ctx];
}

static INLINE void set_skip_context(MACROBLOCKD *xd, int mi_row, int mi_col) {
  const int above_idx = mi_col * 2;
  const int left_idx = (mi_row * 2) & 15;
  int i;
  for (i = 0; i < MAX_MB_PLANE; ++i) {
    struct macroblockd_plane *const pd = &xd->plane[i];
    pd->above_context = &xd->above_context[i][above_idx >> pd->subsampling_x];
    pd->left_context = &xd->left_context[i][left_idx >> pd->subsampling_y];
  }
}

static INLINE int calc_mi_size(int len) {
  
  return len + MI_BLOCK_SIZE;
}

static INLINE void set_mi_row_col(MACROBLOCKD *xd, const TileInfo *const tile,
                                  int mi_row, int bh,
                                  int mi_col, int bw,
                                  int mi_rows, int mi_cols) {
  xd->mb_to_top_edge    = -((mi_row * MI_SIZE) * 8);
  xd->mb_to_bottom_edge = ((mi_rows - bh - mi_row) * MI_SIZE) * 8;
  xd->mb_to_left_edge   = -((mi_col * MI_SIZE) * 8);
  xd->mb_to_right_edge  = ((mi_cols - bw - mi_col) * MI_SIZE) * 8;

  
  xd->up_available    = (mi_row != 0);
  xd->left_available  = (mi_col > tile->mi_col_start);
}

static INLINE void set_prev_mi(VP9_COMMON *cm) {
  const int use_prev_in_find_mv_refs = cm->width == cm->last_width &&
                                       cm->height == cm->last_height &&
                                       !cm->intra_only &&
                                       cm->last_show_frame;
  
  
  cm->prev_mi = use_prev_in_find_mv_refs ?
                  cm->prev_mip + cm->mi_stride + 1 : NULL;
}

static INLINE void update_partition_context(MACROBLOCKD *xd,
                                            int mi_row, int mi_col,
                                            BLOCK_SIZE subsize,
                                            BLOCK_SIZE bsize) {
  PARTITION_CONTEXT *const above_ctx = xd->above_seg_context + mi_col;
  PARTITION_CONTEXT *const left_ctx = xd->left_seg_context + (mi_row & MI_MASK);

  
  const int bs = num_8x8_blocks_wide_lookup[bsize];

  
  
  
  vpx_memset(above_ctx, partition_context_lookup[subsize].above, bs);
  vpx_memset(left_ctx, partition_context_lookup[subsize].left, bs);
}

static INLINE int partition_plane_context(const MACROBLOCKD *xd,
                                          int mi_row, int mi_col,
                                          BLOCK_SIZE bsize) {
  const PARTITION_CONTEXT *above_ctx = xd->above_seg_context + mi_col;
  const PARTITION_CONTEXT *left_ctx = xd->left_seg_context + (mi_row & MI_MASK);

  const int bsl = mi_width_log2(bsize);
  const int bs = 1 << bsl;
  int above = 0, left = 0, i;

  assert(b_width_log2(bsize) == b_height_log2(bsize));
  assert(bsl >= 0);

  for (i = 0; i < bs; i++) {
    above |= above_ctx[i];
    left |= left_ctx[i];
  }
  above = (above & bs) > 0;
  left  = (left & bs) > 0;

  return (left * 2 + above) + bsl * PARTITION_PLOFFSET;
}

#ifdef __cplusplus
}  
#endif

#endif
