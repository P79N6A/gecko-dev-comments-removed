






































#ifndef __txdouble_h__
#define __txdouble_h__


#ifdef __FreeBSD__
#include <ieeefp.h>
#ifdef __alpha__
static fp_except_t allmask = FP_X_INV|FP_X_OFL|FP_X_UFL|FP_X_DZ|FP_X_IMP;
#else
static fp_except_t allmask = FP_X_INV|FP_X_OFL|FP_X_UFL|FP_X_DZ|FP_X_IMP|FP_X_DNML;
#endif
static fp_except_t oldmask = fpsetmask(~allmask);
#endif












#if defined(__arm) || defined(__arm32__) || defined(__arm26__) || defined(__arm__)
#if !defined(__VFP_FP__)
#define FPU_IS_ARM_FPA
#endif
#endif

typedef union txdpun {
    struct {
#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
        PRUint32 lo, hi;
#else
        PRUint32 hi, lo;
#endif
    } s;
    PRFloat64 d;
public:
    operator double() const {
        return d;
    }
} txdpun;

#if (__GNUC__ == 2 && __GNUC_MINOR__ > 95) || __GNUC__ > 2




#define TX_DOUBLE_HI32(x) (__extension__ ({ txdpun u; u.d = (x); u.s.hi; }))
#define TX_DOUBLE_LO32(x) (__extension__ ({ txdpun u; u.d = (x); u.s.lo; }))

#else 





#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
#define TX_DOUBLE_HI32(x)        (((PRUint32 *)&(x))[1])
#define TX_DOUBLE_LO32(x)        (((PRUint32 *)&(x))[0])
#else
#define TX_DOUBLE_HI32(x)        (((PRUint32 *)&(x))[0])
#define TX_DOUBLE_LO32(x)        (((PRUint32 *)&(x))[1])
#endif

#endif 

#define TX_DOUBLE_HI32_SIGNBIT   0x80000000
#define TX_DOUBLE_HI32_EXPMASK   0x7ff00000
#define TX_DOUBLE_HI32_MANTMASK  0x000fffff

#define TX_DOUBLE_IS_NaN(x)                                                \
((TX_DOUBLE_HI32(x) & TX_DOUBLE_HI32_EXPMASK) == TX_DOUBLE_HI32_EXPMASK && \
 (TX_DOUBLE_LO32(x) || (TX_DOUBLE_HI32(x) & TX_DOUBLE_HI32_MANTMASK)))

#ifdef IS_BIG_ENDIAN
#define TX_DOUBLE_NaN {{TX_DOUBLE_HI32_EXPMASK | TX_DOUBLE_HI32_MANTMASK,   \
                        0xffffffff}}
#else
#define TX_DOUBLE_NaN {{0xffffffff,                                         \
                        TX_DOUBLE_HI32_EXPMASK | TX_DOUBLE_HI32_MANTMASK}}
#endif

#if defined(XP_WIN)
#define TX_DOUBLE_COMPARE(LVAL, OP, RVAL)                                  \
    (!TX_DOUBLE_IS_NaN(LVAL) && !TX_DOUBLE_IS_NaN(RVAL) && (LVAL) OP (RVAL))
#else
#define TX_DOUBLE_COMPARE(LVAL, OP, RVAL) ((LVAL) OP (RVAL))
#endif

#endif 
