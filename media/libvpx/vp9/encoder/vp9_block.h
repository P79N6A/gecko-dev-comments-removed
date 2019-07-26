









#ifndef VP9_ENCODER_VP9_BLOCK_H_
#define VP9_ENCODER_VP9_BLOCK_H_

#include "vp9/common/vp9_onyx.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_entropy.h"
#include "vpx_ports/mem.h"
#include "vp9/common/vp9_onyxc_int.h"


typedef struct {
  MV mv;
  int offset;
} search_site;


typedef struct {
  MODE_INFO mic;
  uint8_t *zcoeff_blk;
  int16_t *coeff[MAX_MB_PLANE][3];
  int16_t *qcoeff[MAX_MB_PLANE][3];
  int16_t *dqcoeff[MAX_MB_PLANE][3];
  uint16_t *eobs[MAX_MB_PLANE][3];

  
  int16_t *coeff_pbuf[MAX_MB_PLANE][3];
  int16_t *qcoeff_pbuf[MAX_MB_PLANE][3];
  int16_t *dqcoeff_pbuf[MAX_MB_PLANE][3];
  uint16_t *eobs_pbuf[MAX_MB_PLANE][3];

  int is_coded;
  int num_4x4_blk;
  int skip;
  int_mv best_ref_mv;
  int_mv second_best_ref_mv;
  int_mv ref_mvs[MAX_REF_FRAMES][MAX_MV_REF_CANDIDATES];
  int rate;
  int distortion;
  int64_t intra_error;
  int best_mode_index;
  int rddiv;
  int rdmult;
  int hybrid_pred_diff;
  int comp_pred_diff;
  int single_pred_diff;
  int64_t tx_rd_diff[TX_MODES];
  int64_t best_filter_diff[SWITCHABLE_FILTER_CONTEXTS];

  
  
  int_mv pred_mv[MAX_REF_FRAMES];

  
  unsigned int modes_with_high_error;

  
  unsigned int frames_with_high_error;
} PICK_MODE_CONTEXT;

struct macroblock_plane {
  DECLARE_ALIGNED(16, int16_t, src_diff[64 * 64]);
  int16_t *coeff;
  struct buf_2d src;

  
  int16_t *quant;
  int16_t *quant_shift;
  int16_t *zbin;
  int16_t *round;

  
  int16_t zbin_extra;
};



typedef unsigned int vp9_coeff_cost[BLOCK_TYPES][REF_TYPES][COEF_BANDS][2]
                                   [PREV_COEF_CONTEXTS][MAX_ENTROPY_TOKENS];

typedef struct macroblock MACROBLOCK;
struct macroblock {
  struct macroblock_plane plane[MAX_MB_PLANE];

  MACROBLOCKD e_mbd;
  int skip_block;
  int select_txfm_size;
  int skip_recode;
  int skip_optimize;
  int q_index;

  search_site *ss;
  int ss_count;
  int searches_per_step;

  int errorperbit;
  int sadperbit16;
  int sadperbit4;
  int rddiv;
  int rdmult;
  unsigned int mb_energy;
  unsigned int *mb_activity_ptr;
  int *mb_norm_activity_ptr;
  signed int act_zbin_adj;

  int mv_best_ref_index[MAX_REF_FRAMES];
  unsigned int max_mv_context[MAX_REF_FRAMES];
  unsigned int source_variance;

  int nmvjointcost[MV_JOINTS];
  int nmvcosts[2][MV_VALS];
  int *nmvcost[2];
  int nmvcosts_hp[2][MV_VALS];
  int *nmvcost_hp[2];
  int **mvcost;

  int nmvjointsadcost[MV_JOINTS];
  int nmvsadcosts[2][MV_VALS];
  int *nmvsadcost[2];
  int nmvsadcosts_hp[2][MV_VALS];
  int *nmvsadcost_hp[2];
  int **mvsadcost;

  int mbmode_cost[MB_MODE_COUNT];
  unsigned inter_mode_cost[INTER_MODE_CONTEXTS][INTER_MODES];
  int intra_uv_mode_cost[2][MB_MODE_COUNT];
  int y_mode_costs[INTRA_MODES][INTRA_MODES][INTRA_MODES];
  int switchable_interp_costs[SWITCHABLE_FILTER_CONTEXTS][SWITCHABLE_FILTERS];

  unsigned char sb_index;   
  unsigned char mb_index;   
  unsigned char b_index;    
  unsigned char ab_index;   

  
  
  int mv_col_min;
  int mv_col_max;
  int mv_row_min;
  int mv_row_max;

  uint8_t zcoeff_blk[TX_SIZES][256];
  int skip;

  int encode_breakout;

  unsigned char *active_ptr;

  
  vp9_coeff_cost token_costs[TX_SIZES];
  DECLARE_ALIGNED(16, uint8_t, token_cache[1024]);

  int optimize;

  
  int use_lp32x32fdct;
  int skip_encode;

  
  int fast_ms;
  int_mv pred_mv[MAX_REF_FRAMES];
  int subblock_ref;

  
  
  PICK_MODE_CONTEXT ab4x4_context[4][4][4];
  PICK_MODE_CONTEXT sb8x4_context[4][4][4];
  PICK_MODE_CONTEXT sb4x8_context[4][4][4];
  PICK_MODE_CONTEXT sb8x8_context[4][4][4];
  PICK_MODE_CONTEXT sb8x16_context[4][4][2];
  PICK_MODE_CONTEXT sb16x8_context[4][4][2];
  PICK_MODE_CONTEXT mb_context[4][4];
  PICK_MODE_CONTEXT sb32x16_context[4][2];
  PICK_MODE_CONTEXT sb16x32_context[4][2];
  
  PICK_MODE_CONTEXT sb32_context[4];
  PICK_MODE_CONTEXT sb32x64_context[2];
  PICK_MODE_CONTEXT sb64x32_context[2];
  PICK_MODE_CONTEXT sb64_context;
  int partition_cost[PARTITION_CONTEXTS][PARTITION_TYPES];

  BLOCK_SIZE b_partitioning[4][4][4];
  BLOCK_SIZE mb_partitioning[4][4];
  BLOCK_SIZE sb_partitioning[4];
  BLOCK_SIZE sb64_partitioning;

  void (*fwd_txm4x4)(const int16_t *input, int16_t *output, int stride);
};




static PICK_MODE_CONTEXT *get_block_context(MACROBLOCK *x, BLOCK_SIZE bsize) {
  switch (bsize) {
    case BLOCK_64X64:
      return &x->sb64_context;
    case BLOCK_64X32:
      return &x->sb64x32_context[x->sb_index];
    case BLOCK_32X64:
      return &x->sb32x64_context[x->sb_index];
    case BLOCK_32X32:
      return &x->sb32_context[x->sb_index];
    case BLOCK_32X16:
      return &x->sb32x16_context[x->sb_index][x->mb_index];
    case BLOCK_16X32:
      return &x->sb16x32_context[x->sb_index][x->mb_index];
    case BLOCK_16X16:
      return &x->mb_context[x->sb_index][x->mb_index];
    case BLOCK_16X8:
      return &x->sb16x8_context[x->sb_index][x->mb_index][x->b_index];
    case BLOCK_8X16:
      return &x->sb8x16_context[x->sb_index][x->mb_index][x->b_index];
    case BLOCK_8X8:
      return &x->sb8x8_context[x->sb_index][x->mb_index][x->b_index];
    case BLOCK_8X4:
      return &x->sb8x4_context[x->sb_index][x->mb_index][x->b_index];
    case BLOCK_4X8:
      return &x->sb4x8_context[x->sb_index][x->mb_index][x->b_index];
    case BLOCK_4X4:
      return &x->ab4x4_context[x->sb_index][x->mb_index][x->b_index];
    default:
      assert(0);
      return NULL;
  }
}

struct rdcost_block_args {
  MACROBLOCK *x;
  ENTROPY_CONTEXT t_above[16];
  ENTROPY_CONTEXT t_left[16];
  TX_SIZE tx_size;
  int bw;
  int bh;
  int rate;
  int64_t dist;
  int64_t sse;
  int this_rate;
  int64_t this_dist;
  int64_t this_sse;
  int64_t this_rd;
  int64_t best_rd;
  int skip;
  const int16_t *scan, *nb;
};

#endif  
