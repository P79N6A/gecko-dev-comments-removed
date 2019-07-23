




































#ifndef prbit_h___
#define prbit_h___

#include "prtypes.h"
PR_BEGIN_EXTERN_C




typedef unsigned long prbitmap_t;

#define PR_TEST_BIT(_map,_bit) \
    ((_map)[(_bit)>>PR_BITS_PER_LONG_LOG2] & (1L << ((_bit) & (PR_BITS_PER_LONG-1))))
#define PR_SET_BIT(_map,_bit) \
    ((_map)[(_bit)>>PR_BITS_PER_LONG_LOG2] |= (1L << ((_bit) & (PR_BITS_PER_LONG-1))))
#define PR_CLEAR_BIT(_map,_bit) \
    ((_map)[(_bit)>>PR_BITS_PER_LONG_LOG2] &= ~(1L << ((_bit) & (PR_BITS_PER_LONG-1))))




NSPR_API(PRIntn) PR_CeilingLog2(PRUint32 i); 




NSPR_API(PRIntn) PR_FloorLog2(PRUint32 i); 





#define PR_CEILING_LOG2(_log2,_n)   \
  PR_BEGIN_MACRO                    \
    PRUint32 j_ = (PRUint32)(_n); 	\
    (_log2) = 0;                    \
    if ((j_) & ((j_)-1))            \
	(_log2) += 1;               \
    if ((j_) >> 16)                 \
	(_log2) += 16, (j_) >>= 16; \
    if ((j_) >> 8)                  \
	(_log2) += 8, (j_) >>= 8;   \
    if ((j_) >> 4)                  \
	(_log2) += 4, (j_) >>= 4;   \
    if ((j_) >> 2)                  \
	(_log2) += 2, (j_) >>= 2;   \
    if ((j_) >> 1)                  \
	(_log2) += 1;               \
  PR_END_MACRO







#define PR_FLOOR_LOG2(_log2,_n)   \
  PR_BEGIN_MACRO                    \
    PRUint32 j_ = (PRUint32)(_n); 	\
    (_log2) = 0;                    \
    if ((j_) >> 16)                 \
	(_log2) += 16, (j_) >>= 16; \
    if ((j_) >> 8)                  \
	(_log2) += 8, (j_) >>= 8;   \
    if ((j_) >> 4)                  \
	(_log2) += 4, (j_) >>= 4;   \
    if ((j_) >> 2)                  \
	(_log2) += 2, (j_) >>= 2;   \
    if ((j_) >> 1)                  \
	(_log2) += 1;               \
  PR_END_MACRO















#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64) || \
    defined(_M_X64))
#include <stdlib.h>
#pragma intrinsic(_rotl, _rotr)
#define PR_ROTATE_LEFT32(a, bits) _rotl(a, bits)
#define PR_ROTATE_RIGHT32(a, bits) _rotr(a, bits)
#else
#define PR_ROTATE_LEFT32(a, bits) (((a) << (bits)) | ((a) >> (32 - (bits))))
#define PR_ROTATE_RIGHT32(a, bits) (((a) >> (bits)) | ((a) << (32 - (bits))))
#endif

PR_END_EXTERN_C
#endif 
