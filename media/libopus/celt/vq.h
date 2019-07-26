







































#ifndef VQ_H
#define VQ_H

#include "entenc.h"
#include "entdec.h"
#include "modes.h"












unsigned alg_quant(celt_norm *X, int N, int K, int spread, int B,
      ec_enc *enc
#ifdef RESYNTH
      , opus_val16 gain
#endif
      );









unsigned alg_unquant(celt_norm *X, int N, int K, int spread, int B,
      ec_dec *dec, opus_val16 gain);

void renormalise_vector(celt_norm *X, int N, opus_val16 gain);

int stereo_itheta(celt_norm *X, celt_norm *Y, int stereo, int N);

#endif 
