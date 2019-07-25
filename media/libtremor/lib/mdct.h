
















#ifndef _OGG_mdct_H_
#define _OGG_mdct_H_

#include "ivorbiscodec.h"
#include "misc.h"

#define DATA_TYPE ogg_int32_t
#define REG_TYPE  register ogg_int32_t

#ifdef _LOW_ACCURACY_
#define cPI3_8 (0x0062)
#define cPI2_8 (0x00b5)
#define cPI1_8 (0x00ed)
#else
#define cPI3_8 (0x30fbc54d)
#define cPI2_8 (0x5a82799a)
#define cPI1_8 (0x7641af3d)
#endif

extern void mdct_forward(int n, DATA_TYPE *in, DATA_TYPE *out);
extern void mdct_backward(int n, DATA_TYPE *in, DATA_TYPE *out);

#endif












