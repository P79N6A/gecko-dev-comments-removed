







































#ifndef _FIXPOINT_H_
#define _FIXPOINT_H_

#include <limits.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#ifndef DEFAULT_RADIX
#define DEFAULT_RADIX 12
#endif


typedef int32 fixed32;


#define FLOAT2FIX_ANY(x,radix) \
	(((x)<0.0) ? \
	((fixed32)((x)*(float32)(1<<(radix)) - 0.5)) \
	: ((fixed32)((x)*(float32)(1<<(radix)) + 0.5)))
#define FLOAT2FIX(x) FLOAT2FIX_ANY(x,DEFAULT_RADIX)

#define FIX2FLOAT_ANY(x,radix) ((float32)(x)/(1<<(radix)))
#define FIX2FLOAT(x) FIX2FLOAT_ANY(x,DEFAULT_RADIX)








#if defined(__arm__) && !defined(__thumb__)





#define FIXMUL(a,b) FIXMUL_ANY(a,b,DEFAULT_RADIX)
#define FIXMUL_ANY(a,b,r) ({				\
      int cl, ch, _a = a, _b = b;			\
      __asm__ ("smull %0, %1, %2, %3\n"			\
	   "mov %0, %0, lsr %4\n"			\
	   "orr %0, %0, %1, lsl %5\n"			\
	   : "=&r" (cl), "=&r" (ch)			\
	   : "r" (_a), "r" (_b), "i" (r), "i" (32-(r)));\
      cl; })

#elif defined(_MSC_VER) || (defined(HAVE_LONG_LONG) && SIZEOF_LONG_LONG == 8) 

#define FIXMUL(a,b) FIXMUL_ANY(a,b,DEFAULT_RADIX)
#define FIXMUL_ANY(a,b,radix) ((fixed32)(((int64)(a)*(b))>>(radix)))

#else

#define FIXMUL(a,b) FIXMUL_ANY(a,b,DEFAULT_RADIX)
#define FIXMUL_ANY(a,b,radix) ({ \
	int32 _ah, _bh; \
	uint32 _al, _bl, _t, c; \
	_ah = ((int32)(a)) >> 16; \
	_bh = ((int32)(b)) >> 16; \
	_al = ((uint32)(a)) & 0xffff; \
	_bl = ((uint32)(b)) & 0xffff; \
	_t = _ah * _bl + _al * _bh; \
	c = (fixed32)(((_al * _bl) >> (radix)) \
		      + ((_ah * _bh) << (32 - (radix))) \
		      + ((radix) > 16 ? (_t >> (radix - 16)) : (_t << (16 - radix)))); \
	c;})
#endif



#define MIN_FIXLOG -2829416  /* log(1e-300) * (1<<DEFAULT_RADIX) */
#define MIN_FIXLOG2 -4081985 /* log2(1e-300) * (1<<DEFAULT_RADIX) */

#define FIXLN_2		((fixed32)(0.693147180559945 * (1<<DEFAULT_RADIX)))

#define FIXLN(x) (fixlog(x) - (FIXLN_2 * DEFAULT_RADIX))





int32 fixlog(uint32 x);




int32 fixlog2(uint32 x);

#ifdef __cplusplus
}
#endif


#endif
