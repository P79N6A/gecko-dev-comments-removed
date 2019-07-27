










#ifndef VP9_COMMON_VP9_BLOCKD_H_
#define VP9_COMMON_VP9_BLOCKD_H_

#include "./vpx_config.h"

#include "vpx_ports/mem.h"
#include "vpx_scale/yv12config.h"

#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_common_data.h"
#include "vp9/common/vp9_enums.h"
#include "vp9/common/vp9_filter.h"
#include "vp9/common/vp9_idct.h"
#include "vp9/common/vp9_mv.h"
#include "vp9/common/vp9_scale.h"
#include "vp9/common/vp9_seg_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE_GROUPS 4
#define SKIP_CONTEXTS 3
#define INTER_MODE_CONTEXTS 7


#define MAX_MV_REF_CANDIDATES 2

#define INTRA_INTER_CONTEXTS 4
#define COMP_INTER_CONTEXTS 5
#define REF_CONTEXTS 5

typedef enum {
  PLANE_TYPE_Y  = 0,
  PLANE_TYPE_UV = 1,
  PLANE_TYPES
} PLANE_TYPE;

typedef char ENTROPY_CONTEXT;

typedef char PARTITION_CONTEXT;

static INLINE int combine_entropy_contexts(ENTROPY_CONTEXT a,
                                           ENTROPY_CONTEXT b) {
  return (a != 0) + (b != 0);
}

typedef enum {
  KEY_FRAME = 0,
  INTER_FRAME = 1,
  FRAME_TYPES,
} FRAME_TYPE;

typedef enum {
  DC_PRED,         
  V_PRED,          
  H_PRED,          
  D45_PRED,        
  D135_PRED,       
  D117_PRED,       
  D153_PRED,       
  D207_PRED,       
  D63_PRED,        
  TM_PRED,         
  NEARESTMV,
  NEARMV,
  ZEROMV,
  NEWMV,
  MB_MODE_COUNT
} PREDICTION_MODE;

static INLINE int is_inter_mode(PREDICTION_MODE mode) {
  return mode >= NEARESTMV && mode <= NEWMV;
}

#define INTRA_MODES (TM_PRED + 1)

#define INTER_MODES (1 + NEWMV - NEARESTMV)

#define INTER_OFFSET(mode) ((mode) - NEARESTMV)





typedef struct {
  PREDICTION_MODE as_mode;
  int_mv as_mv[2];  
} b_mode_info;




typedef enum {
  NONE = -1,
  INTRA_FRAME = 0,
  LAST_FRAME = 1,
  GOLDEN_FRAME = 2,
  ALTREF_FRAME = 3,
  MAX_REF_FRAMES = 4
} MV_REFERENCE_FRAME;

static INLINE int b_width_log2(BLOCK_SIZE sb_type) {
  return b_width_log2_lookup[sb_type];
}
static INLINE int b_height_log2(BLOCK_SIZE sb_type) {
  return b_height_log2_lookup[sb_type];
}

static INLINE int mi_width_log2(BLOCK_SIZE sb_type) {
  return mi_width_log2_lookup[sb_type];
}


typedef struct {
  
  BLOCK_SIZE sb_type;
  PREDICTION_MODE mode;
  TX_SIZE tx_size;
  int8_t skip;
  int8_t segment_id;
  int8_t seg_id_predicted;  

  
  PREDICTION_MODE uv_mode;

  
  MV_REFERENCE_FRAME ref_frame[2];
  int_mv mv[2];
  int_mv ref_mvs[MAX_REF_FRAMES][MAX_MV_REF_CANDIDATES];
  uint8_t mode_context[MAX_REF_FRAMES];
  INTERP_FILTER interp_filter;
} MB_MODE_INFO;

typedef struct MODE_INFO {
  struct MODE_INFO *src_mi;
  MB_MODE_INFO mbmi;
  b_mode_info bmi[4];
} MODE_INFO;

static INLINE PREDICTION_MODE get_y_mode(const MODE_INFO *mi, int block) {
  return mi->mbmi.sb_type < BLOCK_8X8 ? mi->bmi[block].as_mode
                                      : mi->mbmi.mode;
}

static INLINE int is_inter_block(const MB_MODE_INFO *mbmi) {
  return mbmi->ref_frame[0] > INTRA_FRAME;
}

static INLINE int has_second_ref(const MB_MODE_INFO *mbmi) {
  return mbmi->ref_frame[1] > INTRA_FRAME;
}

PREDICTION_MODE vp9_left_block_mode(const MODE_INFO *cur_mi,
                                    const MODE_INFO *left_mi, int b);

PREDICTION_MODE vp9_above_block_mode(const MODE_INFO *cur_mi,
                                     const MODE_INFO *above_mi, int b);

enum mv_precision {
  MV_PRECISION_Q3,
  MV_PRECISION_Q4
};

enum { MAX_MB_PLANE = 3 };

struct buf_2d {
  uint8_t *buf;
  int stride;
};

struct macroblockd_plane {
  tran_low_t *dqcoeff;
  PLANE_TYPE plane_type;
  int subsampling_x;
  int subsampling_y;
  struct buf_2d dst;
  struct buf_2d pre[2];
  const int16_t *dequant;
  ENTROPY_CONTEXT *above_context;
  ENTROPY_CONTEXT *left_context;
};

#define BLOCK_OFFSET(x, i) ((x) + (i) * 16)

typedef struct RefBuffer {
  
  
  int idx;
  YV12_BUFFER_CONFIG *buf;
  struct scale_factors sf;
} RefBuffer;

typedef struct macroblockd {
  struct macroblockd_plane plane[MAX_MB_PLANE];

  int mi_stride;

  MODE_INFO *mi;

  int up_available;
  int left_available;

  
  int mb_to_left_edge;
  int mb_to_right_edge;
  int mb_to_top_edge;
  int mb_to_bottom_edge;

  
  RefBuffer *block_refs[2];

  
  const YV12_BUFFER_CONFIG *cur_buf;

  
  DECLARE_ALIGNED(16, uint8_t, mc_buf[80 * 2 * 80 * 2]);

#if CONFIG_VP9_HIGHBITDEPTH
  
  int bd;
  DECLARE_ALIGNED(16, uint16_t, mc_buf_high[80 * 2 * 80 * 2]);
#endif

  int lossless;

  int corrupted;

  DECLARE_ALIGNED(16, tran_low_t, dqcoeff[MAX_MB_PLANE][64 * 64]);

  ENTROPY_CONTEXT *above_context[MAX_MB_PLANE];
  ENTROPY_CONTEXT left_context[MAX_MB_PLANE][16];

  PARTITION_CONTEXT *above_seg_context;
  PARTITION_CONTEXT left_seg_context[8];
} MACROBLOCKD;

static INLINE BLOCK_SIZE get_subsize(BLOCK_SIZE bsize,
                                     PARTITION_TYPE partition) {
  return subsize_lookup[partition][bsize];
}

extern const TX_TYPE intra_mode_to_tx_type_lookup[INTRA_MODES];

static INLINE TX_TYPE get_tx_type(PLANE_TYPE plane_type,
                                  const MACROBLOCKD *xd) {
  const MB_MODE_INFO *const mbmi = &xd->mi[0].src_mi->mbmi;

  if (plane_type != PLANE_TYPE_Y || is_inter_block(mbmi))
    return DCT_DCT;
  return intra_mode_to_tx_type_lookup[mbmi->mode];
}

static INLINE TX_TYPE get_tx_type_4x4(PLANE_TYPE plane_type,
                                      const MACROBLOCKD *xd, int ib) {
  const MODE_INFO *const mi = xd->mi[0].src_mi;

  if (plane_type != PLANE_TYPE_Y || xd->lossless || is_inter_block(&mi->mbmi))
    return DCT_DCT;

  return intra_mode_to_tx_type_lookup[get_y_mode(mi, ib)];
}

void vp9_setup_block_planes(MACROBLOCKD *xd, int ss_x, int ss_y);

static INLINE TX_SIZE get_uv_tx_size_impl(TX_SIZE y_tx_size, BLOCK_SIZE bsize,
                                          int xss, int yss) {
  if (bsize < BLOCK_8X8) {
    return TX_4X4;
  } else {
    const BLOCK_SIZE plane_bsize = ss_size_lookup[bsize][xss][yss];
    return MIN(y_tx_size, max_txsize_lookup[plane_bsize]);
  }
}

static INLINE TX_SIZE get_uv_tx_size(const MB_MODE_INFO *mbmi,
                                     const struct macroblockd_plane *pd) {
  return get_uv_tx_size_impl(mbmi->tx_size, mbmi->sb_type, pd->subsampling_x,
                             pd->subsampling_y);
}

static INLINE BLOCK_SIZE get_plane_block_size(BLOCK_SIZE bsize,
    const struct macroblockd_plane *pd) {
  return ss_size_lookup[bsize][pd->subsampling_x][pd->subsampling_y];
}

typedef void (*foreach_transformed_block_visitor)(int plane, int block,
                                                  BLOCK_SIZE plane_bsize,
                                                  TX_SIZE tx_size,
                                                  void *arg);

void vp9_foreach_transformed_block_in_plane(
    const MACROBLOCKD *const xd, BLOCK_SIZE bsize, int plane,
    foreach_transformed_block_visitor visit, void *arg);


void vp9_foreach_transformed_block(
    const MACROBLOCKD* const xd, BLOCK_SIZE bsize,
    foreach_transformed_block_visitor visit, void *arg);

static INLINE void txfrm_block_to_raster_xy(BLOCK_SIZE plane_bsize,
                                            TX_SIZE tx_size, int block,
                                            int *x, int *y) {
  const int bwl = b_width_log2(plane_bsize);
  const int tx_cols_log2 = bwl - tx_size;
  const int tx_cols = 1 << tx_cols_log2;
  const int raster_mb = block >> (tx_size << 1);
  *x = (raster_mb & (tx_cols - 1)) << tx_size;
  *y = (raster_mb >> tx_cols_log2) << tx_size;
}

void vp9_set_contexts(const MACROBLOCKD *xd, struct macroblockd_plane *pd,
                      BLOCK_SIZE plane_bsize, TX_SIZE tx_size, int has_eob,
                      int aoff, int loff);

#ifdef __cplusplus
}  
#endif

#endif
