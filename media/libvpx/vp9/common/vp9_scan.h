









#ifndef VP9_COMMON_VP9_SCAN_H_
#define VP9_COMMON_VP9_SCAN_H_

#include "vpx/vpx_integer.h"
#include "vpx_ports/mem.h"

#include "vp9/common/vp9_enums.h"
#include "vp9/common/vp9_blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NEIGHBORS 2

typedef struct {
  const int16_t *scan;
  const int16_t *iscan;
  const int16_t *neighbors;
} scan_order;

extern const scan_order vp9_default_scan_orders[TX_SIZES];
extern const scan_order vp9_scan_orders[TX_SIZES][TX_TYPES];

static INLINE int get_coef_context(const int16_t *neighbors,
                                   const uint8_t *token_cache, int c) {
  return (1 + token_cache[neighbors[MAX_NEIGHBORS * c + 0]] +
          token_cache[neighbors[MAX_NEIGHBORS * c + 1]]) >> 1;
}

static INLINE const scan_order *get_scan(const MACROBLOCKD *xd, TX_SIZE tx_size,
                                         PLANE_TYPE type, int block_idx) {
  const MODE_INFO *const mi = xd->mi[0];

  if (is_inter_block(&mi->mbmi) || type != PLANE_TYPE_Y || xd->lossless) {
    return &vp9_default_scan_orders[tx_size];
  } else {
    const PREDICTION_MODE mode = get_y_mode(mi, block_idx);
    return &vp9_scan_orders[tx_size][intra_mode_to_tx_type_lookup[mode]];
  }
}

#ifdef __cplusplus
}  
#endif

#endif
