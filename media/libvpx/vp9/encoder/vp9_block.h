









#ifndef VP9_ENCODER_VP9_BLOCK_H_
#define VP9_ENCODER_VP9_BLOCK_H_

#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_entropy.h"
#include "vpx_ports/mem.h"
#include "vp9/common/vp9_onyxc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned int sse;
  int sum;
  unsigned int var;
} diff;

struct macroblock_plane {
  DECLARE_ALIGNED(16, int16_t, src_diff[64 * 64]);
  int16_t *qcoeff;
  int16_t *coeff;
  uint16_t *eobs;
  struct buf_2d src;

  
  int16_t *quant_fp;
  int16_t *round_fp;
  int16_t *quant;
  int16_t *quant_shift;
  int16_t *zbin;
  int16_t *round;

  int64_t quant_thred[2];
  
  int16_t zbin_extra;
};



typedef unsigned int vp9_coeff_cost[PLANE_TYPES][REF_TYPES][COEF_BANDS][2]
                                   [COEFF_CONTEXTS][ENTROPY_TOKENS];

typedef struct macroblock MACROBLOCK;
struct macroblock {
  struct macroblock_plane plane[MAX_MB_PLANE];

  MACROBLOCKD e_mbd;
  int skip_block;
  int select_tx_size;
  int skip_recode;
  int skip_optimize;
  int q_index;

  int errorperbit;
  int sadperbit16;
  int sadperbit4;
  int rddiv;
  int rdmult;
  int mb_energy;

  int mv_best_ref_index[MAX_REF_FRAMES];
  unsigned int max_mv_context[MAX_REF_FRAMES];
  unsigned int source_variance;
  unsigned int pred_sse[MAX_REF_FRAMES];
  int pred_mv_sad[MAX_REF_FRAMES];

  int nmvjointcost[MV_JOINTS];
  int *nmvcost[2];
  int *nmvcost_hp[2];
  int **mvcost;

  int nmvjointsadcost[MV_JOINTS];
  int *nmvsadcost[2];
  int *nmvsadcost_hp[2];
  int **mvsadcost;

  
  
  int mv_col_min;
  int mv_col_max;
  int mv_row_min;
  int mv_row_max;

  uint8_t zcoeff_blk[TX_SIZES][256];
  int skip;

  int encode_breakout;

  
  vp9_coeff_cost token_costs[TX_SIZES];

  int in_static_area;

  int optimize;

  
  int use_lp32x32fdct;
  int skip_encode;

  
  int quant_fp;

  
  uint8_t skip_txfm[MAX_MB_PLANE << 2];

  int64_t bsse[MAX_MB_PLANE << 2];

  
  MV pred_mv[MAX_REF_FRAMES];

  void (*fwd_txm4x4)(const int16_t *input, int16_t *output, int stride);
  void (*itxm_add)(const int16_t *input, uint8_t *dest, int stride, int eob);
};

#ifdef __cplusplus
}  
#endif

#endif
