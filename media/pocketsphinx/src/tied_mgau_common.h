









































#ifndef __TIED_MGAU_COMMON_H__
#define __TIED_MGAU_COMMON_H__

#include <sphinxbase/logmath.h>
#include <sphinxbase/fixpoint.h>

#define MGAU_MIXW_VERSION	"1.0"   /* Sphinx-3 file format version for mixw */
#define MGAU_PARAM_VERSION	"1.0"   /* Sphinx-3 file format version for mean/var */
#define NONE		-1
#define WORST_DIST	(int32)(0x80000000)


#ifdef FIXED_POINT
#define GMMSUB(a,b) \
	(((a)-(b) > a) ? (INT_MIN) : ((a)-(b)))

#define GMMADD(a,b) \
	(((a)+(b) < a) ? (INT_MAX) : ((a)+(b)))
#else
#define GMMSUB(a,b) ((a)-(b))
#define GMMADD(a,b) ((a)+(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define LOGMATH_INLINE static inline
#elif defined(_MSC_VER)
#define LOGMATH_INLINE __inline
#else
#define LOGMATH_INLINE static
#endif




#define MAX_NEG_MIXW 159 /**< Maximum negated mixture weight value. */
#define MAX_NEG_ASCR 96  /**< Maximum negated acoustic score value. */


















LOGMATH_INLINE int
fast_logmath_add(logmath_t *lmath, int mlx, int mly)
{
    logadd_t *t = LOGMATH_TABLE(lmath);
    int d, r;

    
    if (mlx > mly) {
        d = (mlx - mly);
        r = mly;
    }
    else {
        d = (mly - mlx);
        r = mlx;
    }

    return r - (((uint8 *)t->table)[d]);
}

#endif 
