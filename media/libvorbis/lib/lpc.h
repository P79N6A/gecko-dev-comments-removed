
















#ifndef _V_LPC_H_
#define _V_LPC_H_

#include "vorbis/codec.h"


extern float vorbis_lpc_from_data(float *data,float *lpc,int n,int m);

extern void vorbis_lpc_predict(float *coeff,float *prime,int m,
                               float *data,long n);

#endif
