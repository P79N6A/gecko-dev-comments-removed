









#ifndef VP9_ENCODER_VP9_QUANTIZE_H_
#define VP9_ENCODER_VP9_QUANTIZE_H_

#include "./vpx_config.h"
#include "vp9/encoder/vp9_block.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  DECLARE_ALIGNED(16, int16_t, y_quant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, y_quant_shift[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, y_zbin[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, y_round[QINDEX_RANGE][8]);

  
  
  DECLARE_ALIGNED(16, int16_t, y_quant_fp[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_quant_fp[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, y_round_fp[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_round_fp[QINDEX_RANGE][8]);

  DECLARE_ALIGNED(16, int16_t, uv_quant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_quant_shift[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_zbin[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_round[QINDEX_RANGE][8]);
} QUANTS;

void vp9_quantize_dc(const tran_low_t *coeff_ptr,
                     int n_coeffs, int skip_block,
                     const int16_t *round_ptr, const int16_t quant_ptr,
                     tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                     const int16_t dequant_ptr, uint16_t *eob_ptr);
void vp9_quantize_dc_32x32(const tran_low_t *coeff_ptr, int skip_block,
                           const int16_t *round_ptr, const int16_t quant_ptr,
                           tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                           const int16_t dequant_ptr, uint16_t *eob_ptr);
void vp9_regular_quantize_b_4x4(MACROBLOCK *x, int plane, int block,
                                const int16_t *scan, const int16_t *iscan);

#if CONFIG_VP9_HIGHBITDEPTH
void vp9_highbd_quantize_dc(const tran_low_t *coeff_ptr,
                            int n_coeffs, int skip_block,
                            const int16_t *round_ptr, const int16_t quant_ptr,
                            tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                            const int16_t dequant_ptr, uint16_t *eob_ptr);
void vp9_highbd_quantize_dc_32x32(const tran_low_t *coeff_ptr,
                                  int skip_block,
                                  const int16_t *round_ptr,
                                  const int16_t quant_ptr,
                                  tran_low_t *qcoeff_ptr,
                                  tran_low_t *dqcoeff_ptr,
                                  const int16_t dequant_ptr,
                                  uint16_t *eob_ptr);
#endif

struct VP9_COMP;
struct VP9Common;

void vp9_frame_init_quantizer(struct VP9_COMP *cpi);

void vp9_init_plane_quantizers(struct VP9_COMP *cpi, MACROBLOCK *x);

void vp9_init_quantizer(struct VP9_COMP *cpi);

void vp9_set_quantizer(struct VP9Common *cm, int q);

int vp9_quantizer_to_qindex(int quantizer);

int vp9_qindex_to_quantizer(int qindex);

#ifdef __cplusplus
}  
#endif

#endif
