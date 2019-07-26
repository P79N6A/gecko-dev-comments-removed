




































#ifndef CWRS_H
#define CWRS_H

#include "arch.h"
#include "stack_alloc.h"
#include "entenc.h"
#include "entdec.h"

#ifdef CUSTOM_MODES
int log2_frac(opus_uint32 val, int frac);
#endif

void get_required_bits(opus_int16 *bits, int N, int K, int frac);

void encode_pulses(const int *_y, int N, int K, ec_enc *enc);

void decode_pulses(int *_y, int N, int K, ec_dec *dec);

#endif 
