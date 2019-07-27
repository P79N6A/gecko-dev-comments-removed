






#ifndef SkMathPriv_DEFINED
#define SkMathPriv_DEFINED

#include "SkMath.h"

#ifdef SK_BUILD_FOR_IOS


#define SK_DISCARD_DENORMALIZED_FOR_SPEED
#endif



#define SkExtractSign(n)    ((int32_t)(n) >> 31)




static inline int32_t SkApplySign(int32_t n, int32_t sign) {
    SkASSERT(sign == 0 || sign == -1);
    return (n ^ sign) - sign;
}


static inline int32_t SkCopySign32(int32_t x, int32_t y) {
    return SkApplySign(x, SkExtractSign(x ^ y));
}






static inline unsigned SkClampUMax(unsigned value, unsigned max) {
    if (value > max) {
        value = max;
    }
    return value;
}






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
