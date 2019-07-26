




































#ifndef BANDS_H
#define BANDS_H

#include "arch.h"
#include "modes.h"
#include "entenc.h"
#include "entdec.h"
#include "rate.h"






void compute_band_energies(const CELTMode *m, const celt_sig *X, celt_ener *bandE, int end, int C, int M);









void normalise_bands(const CELTMode *m, const celt_sig * restrict freq, celt_norm * restrict X, const celt_ener *bandE, int end, int C, int M);






void denormalise_bands(const CELTMode *m, const celt_norm * restrict X, celt_sig * restrict freq, const celt_ener *bandE, int end, int C, int M);

#define SPREAD_NONE       (0)
#define SPREAD_LIGHT      (1)
#define SPREAD_NORMAL     (2)
#define SPREAD_AGGRESSIVE (3)

int spreading_decision(const CELTMode *m, celt_norm *X, int *average,
      int last_decision, int *hf_average, int *tapset_decision, int update_hf,
      int end, int C, int M);

#ifdef MEASURE_NORM_MSE
void measure_norm_mse(const CELTMode *m, float *X, float *X0, float *bandE, float *bandE0, int M, int N, int C);
#endif

void haar1(celt_norm *X, int N0, int stride);







void quant_all_bands(int encode, const CELTMode *m, int start, int end,
      celt_norm * X, celt_norm * Y, unsigned char *collapse_masks, const celt_ener *bandE, int *pulses,
      int time_domain, int fold, int dual_stereo, int intensity, int *tf_res,
      opus_int32 total_bits, opus_int32 balance, ec_ctx *ec, int M, int codedBands, opus_uint32 *seed);

void anti_collapse(const CELTMode *m, celt_norm *X_, unsigned char *collapse_masks, int LM, int C, int size,
      int start, int end, opus_val16 *logE, opus_val16 *prev1logE,
      opus_val16 *prev2logE, int *pulses, opus_uint32 seed);

opus_uint32 celt_lcg_rand(opus_uint32 seed);

#endif 
