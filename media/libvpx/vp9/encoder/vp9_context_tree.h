









#ifndef VP9_ENCODER_VP9_CONTEXT_TREE_H_
#define VP9_ENCODER_VP9_CONTEXT_TREE_H_

#include "vp9/common/vp9_blockd.h"

struct VP9_COMP;
struct VP9Common;
struct ThreadData;


typedef struct {
  MODE_INFO mic;
  uint8_t *zcoeff_blk;
  tran_low_t *coeff[MAX_MB_PLANE][3];
  tran_low_t *qcoeff[MAX_MB_PLANE][3];
  tran_low_t *dqcoeff[MAX_MB_PLANE][3];
  uint16_t *eobs[MAX_MB_PLANE][3];

  
  tran_low_t *coeff_pbuf[MAX_MB_PLANE][3];
  tran_low_t *qcoeff_pbuf[MAX_MB_PLANE][3];
  tran_low_t *dqcoeff_pbuf[MAX_MB_PLANE][3];
  uint16_t *eobs_pbuf[MAX_MB_PLANE][3];

  int is_coded;
  int num_4x4_blk;
  int skip;
  int pred_pixel_ready;
  
  
  int skippable;
  uint8_t skip_txfm[MAX_MB_PLANE << 2];
  int best_mode_index;
  int hybrid_pred_diff;
  int comp_pred_diff;
  int single_pred_diff;
  int64_t tx_rd_diff[TX_MODES];
  int64_t best_filter_diff[SWITCHABLE_FILTER_CONTEXTS];

  
  
  int rate;
  int64_t dist;

#if CONFIG_VP9_TEMPORAL_DENOISING
  unsigned int newmv_sse;
  unsigned int zeromv_sse;
  PREDICTION_MODE best_sse_inter_mode;
  int_mv best_sse_mv;
  MV_REFERENCE_FRAME best_reference_frame;
  MV_REFERENCE_FRAME best_zeromv_reference_frame;
#endif

  
  
  MV pred_mv[MAX_REF_FRAMES];
  INTERP_FILTER pred_interp_filter;
} PICK_MODE_CONTEXT;

typedef struct PC_TREE {
  int index;
  PARTITION_TYPE partitioning;
  BLOCK_SIZE block_size;
  PICK_MODE_CONTEXT none;
  PICK_MODE_CONTEXT horizontal[2];
  PICK_MODE_CONTEXT vertical[2];
  union {
    struct PC_TREE *split[4];
    PICK_MODE_CONTEXT *leaf_split[4];
  };
} PC_TREE;

void vp9_setup_pc_tree(struct VP9Common *cm, struct ThreadData *td);
void vp9_free_pc_tree(struct ThreadData *td);

#endif 
