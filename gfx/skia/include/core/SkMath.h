








#ifndef SkMath_DEFINED
#define SkMath_DEFINED

#include "SkTypes.h"


int SkCLZ_portable(uint32_t);





int32_t SkMulShift(int32_t a, int32_t b, unsigned shift);





int32_t SkMulDiv(int32_t numer1, int32_t numer2, int32_t denom);





int32_t SkDivBits(int32_t numer, int32_t denom, int shift);



int32_t SkSqrtBits(int32_t value, int bitBias);



#define SkSqrt32(n)         SkSqrtBits(n, 15)



int32_t SkCubeRootBits(int32_t value, int bitBias);



#define SkExtractSign(n)    ((int32_t)(n) >> 31)




static inline int32_t SkApplySign(int32_t n, int32_t sign) {
    SkASSERT(sign == 0 || sign == -1);
    return (n ^ sign) - sign;
}


static inline int32_t SkCopySign32(int32_t x, int32_t y) {
    return SkApplySign(x, SkExtractSign(x ^ y));
}



static inline int SkClampPos(int value) {
    return value & ~(value >> 31);
}







static inline int SkClampMax(int value, int max) {
    
    SkASSERT(max >= 0);
    if (value < 0) {
        value = 0;
    }
    if (value > max) {
        value = max;
    }
    return value;
}






static inline unsigned SkClampUMax(unsigned value, unsigned max) {
#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (value > max) {
        value = max;
    }
    return value;
#else
    int diff = max - value;
    
    diff &= diff >> 31;

    return value + diff;
#endif
}



#if defined(__arm__)
    #define SkCLZ(x)    __builtin_clz(x)
#endif

#ifndef SkCLZ
    #define SkCLZ(x)    SkCLZ_portable(x)
#endif







static inline int SkNextPow2(int value) {
    SkASSERT(value > 0);
    return 1 << (32 - SkCLZ(value - 1));
}









static inline int SkNextLog2(uint32_t value) {
    SkASSERT(value != 0);
    return 32 - SkCLZ(value - 1);
}




static inline bool SkIsPow2(int value) {
    return (value & (value - 1)) == 0;
}







#if defined(__arm__) \
  && !defined(__thumb__) \
  && !defined(__ARM_ARCH_4T__) \
  && !defined(__ARM_ARCH_5T__)
    static inline int32_t SkMulS16(S16CPU x, S16CPU y) {
        SkASSERT((int16_t)x == x);
        SkASSERT((int16_t)y == y);
        int32_t product;
        asm("smulbb %0, %1, %2 \n"
            : "=r"(product)
            : "r"(x), "r"(y)
            );
        return product;
    }
#else
    #ifdef SK_DEBUG
        static inline int32_t SkMulS16(S16CPU x, S16CPU y) {
            SkASSERT((int16_t)x == x);
            SkASSERT((int16_t)y == y);
            return x * y;
        }
    #else
        #define SkMulS16(x, y)  ((x) * (y))
    #endif
#endif




static inline U8CPU SkMulDiv255Trunc(U8CPU a, U8CPU b) {
    SkASSERT((uint8_t)a == a);
    SkASSERT((uint8_t)b == b);
    unsigned prod = SkMulS16(a, b) + 1;
    return (prod + (prod >> 8)) >> 8;
}




static inline U8CPU SkMulDiv255Round(U8CPU a, U8CPU b) {
    SkASSERT((uint8_t)a == a);
    SkASSERT((uint8_t)b == b);
    unsigned prod = SkMulS16(a, b) + 128;
    return (prod + (prod >> 8)) >> 8;
}




static inline U8CPU SkMulDiv255Ceiling(U8CPU a, U8CPU b) {
    SkASSERT((uint8_t)a == a);
    SkASSERT((uint8_t)b == b);
    unsigned prod = SkMulS16(a, b) + 255;
    return (prod + (prod >> 8)) >> 8;
}




static inline unsigned SkMul16ShiftRound(unsigned a, unsigned b, int shift) {
    SkASSERT(a <= 32767);
    SkASSERT(b <= 32767);
    SkASSERT(shift > 0 && shift <= 8);
    unsigned prod = SkMulS16(a, b) + (1 << (shift - 1));
    return (prod + (prod >> shift)) >> shift;
}



static inline unsigned SkDiv255Round(unsigned prod) {
    prod += 128;
    return (prod + (prod >> 8)) >> 8;
}

#endif

