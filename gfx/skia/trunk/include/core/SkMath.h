








#ifndef SkMath_DEFINED
#define SkMath_DEFINED

#include "SkTypes.h"






static inline bool sk_64_isS32(int64_t value) {
    return (int32_t)value == value;
}






static inline int32_t sk_64_asS32(int64_t value) {
    SkASSERT(sk_64_isS32(value));
    return (int32_t)value;
}




static inline int64_t sk_64_mul(int64_t a, int64_t b) {
    return a * b;
}








static inline int32_t SkMulDiv(int32_t numer1, int32_t numer2, int32_t denom) {
    SkASSERT(denom);

    int64_t tmp = sk_64_mul(numer1, numer2) / denom;
    return sk_64_asS32(tmp);
}






int32_t SkDivBits(int32_t numer, int32_t denom, int shift);




int32_t SkSqrtBits(int32_t value, int bitBias);



#define SkSqrt32(n)         SkSqrtBits(n, 15)


int SkCLZ_portable(uint32_t);

#ifndef SkCLZ
    #if defined(_MSC_VER) && _MSC_VER >= 1400
        #include <intrin.h>

        static inline int SkCLZ(uint32_t mask) {
            if (mask) {
                DWORD index;
                _BitScanReverse(&index, mask);
                return index ^ 0x1F;
            } else {
                return 32;
            }
        }
    #elif defined(SK_CPU_ARM32) || defined(__GNUC__) || defined(__clang__)
        static inline int SkCLZ(uint32_t mask) {
            
            return mask ? __builtin_clz(mask) : 32;
        }
    #else
        #define SkCLZ(x)    SkCLZ_portable(x)
    #endif
#endif




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








#ifdef SK_ARM_HAS_EDSP
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





static inline unsigned SkMul16ShiftRound(U16CPU a, U16CPU b, int shift) {
    SkASSERT(a <= 32767);
    SkASSERT(b <= 32767);
    SkASSERT(shift > 0 && shift <= 8);
    unsigned prod = SkMulS16(a, b) + (1 << (shift - 1));
    return (prod + (prod >> shift)) >> shift;
}





static inline U8CPU SkMulDiv255Round(U16CPU a, U16CPU b) {
    SkASSERT(a <= 32767);
    SkASSERT(b <= 32767);
    unsigned prod = SkMulS16(a, b) + 128;
    return (prod + (prod >> 8)) >> 8;
}




template <typename In, typename Out>
inline void SkTDivMod(In numer, In denom, Out* div, Out* mod) {
#ifdef SK_CPU_ARM32
    
    
    
    
    
    
    const In d = numer/denom;
    *div = static_cast<Out>(d);
    *mod = static_cast<Out>(numer-d*denom);
#else
    
    *div = static_cast<Out>(numer/denom);
    *mod = static_cast<Out>(numer%denom);
#endif
}

#endif
