






#ifndef SkMathPriv_DEFINED
#define SkMathPriv_DEFINED

#include "SkMath.h"



#define SkExtractSign(n)    ((int32_t)(n) >> 31)




static inline int32_t SkApplySign(int32_t n, int32_t sign) {
    SkASSERT(sign == 0 || sign == -1);
    return (n ^ sign) - sign;
}


static inline int32_t SkCopySign32(int32_t x, int32_t y) {
    return SkApplySign(x, SkExtractSign(x ^ y));
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





int32_t SkMulShift(int32_t a, int32_t b, unsigned shift);



int32_t SkCubeRootBits(int32_t value, int bitBias);






static inline U8CPU SkMulDiv255Trunc(U8CPU a, U8CPU b) {
    SkASSERT((uint8_t)a == a);
    SkASSERT((uint8_t)b == b);
    unsigned prod = SkMulS16(a, b) + 1;
    return (prod + (prod >> 8)) >> 8;
}




static inline U8CPU SkMulDiv255Ceiling(U8CPU a, U8CPU b) {
    SkASSERT((uint8_t)a == a);
    SkASSERT((uint8_t)b == b);
    unsigned prod = SkMulS16(a, b) + 255;
    return (prod + (prod >> 8)) >> 8;
}



static inline unsigned SkDiv255Round(unsigned prod) {
    prod += 128;
    return (prod + (prod >> 8)) >> 8;
}

#endif

