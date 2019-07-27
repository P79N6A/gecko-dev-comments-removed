






#include "SkFloatBits.h"
#include "SkMathPriv.h"














#define EXP_BIAS            (127+23)
#define MATISSA_MAGIC_BIG   (1 << 23)

static inline int unpack_exp(uint32_t packed) {
    return (packed << 1 >> 24);
}

#if 0

static inline int unpack_matissa(uint32_t packed) {
    
    return (packed & ~0xFF000000) | MATISSA_MAGIC_BIG;
}
#endif


static inline int unpack_matissa_dirty(uint32_t packed) {
    return packed & ~0xFF000000;
}


int32_t SkFloatBits_toIntCast(int32_t packed) {
    int exp = unpack_exp(packed) - EXP_BIAS;
    int value = unpack_matissa_dirty(packed) | MATISSA_MAGIC_BIG;

    if (exp >= 0) {
        if (exp > 7) {    
            value = SK_MaxS32;
        } else {
            value <<= exp;
        }
    } else {
        exp = -exp;
        if (exp > 25) {   
            exp = 25;
        }
        value >>= exp;
    }
    return SkApplySign(value, SkExtractSign(packed));
}


int32_t SkFloatBits_toIntFloor(int32_t packed) {
    
    if ((packed << 1) == 0) {
        return 0;
    }

    int exp = unpack_exp(packed) - EXP_BIAS;
    int value = unpack_matissa_dirty(packed) | MATISSA_MAGIC_BIG;

    if (exp >= 0) {
        if (exp > 7) {    
            value = SK_MaxS32;
        } else {
            value <<= exp;
        }
        
        return SkApplySign(value, SkExtractSign(packed));
    } else {
        
        value = SkApplySign(value, SkExtractSign(packed));
        exp = -exp;
        if (exp > 25) {   
#ifdef SK_DISCARD_DENORMALIZED_FOR_SPEED
        
        
        
            if (exp > 149) {
                return 0;
            }
#else
            exp = 25;
#endif
        }
        
        return value >> exp;
    }
}


int32_t SkFloatBits_toIntRound(int32_t packed) {
    
    if ((packed << 1) == 0) {
        return 0;
    }

    int exp = unpack_exp(packed) - EXP_BIAS;
    int value = unpack_matissa_dirty(packed) | MATISSA_MAGIC_BIG;

    if (exp >= 0) {
        if (exp > 7) {    
            value = SK_MaxS32;
        } else {
            value <<= exp;
        }
        
        return SkApplySign(value, SkExtractSign(packed));
    } else {
        
        value = SkApplySign(value, SkExtractSign(packed));
        exp = -exp;
        if (exp > 25) {   
            exp = 25;
        }
        int add = 1 << (exp - 1);
        return (value + add) >> exp;
    }
}


int32_t SkFloatBits_toIntCeil(int32_t packed) {
    
    if ((packed << 1) == 0) {
        return 0;
    }

    int exp = unpack_exp(packed) - EXP_BIAS;
    int value = unpack_matissa_dirty(packed) | MATISSA_MAGIC_BIG;

    if (exp >= 0) {
        if (exp > 7) {    
            value = SK_MaxS32;
        } else {
            value <<= exp;
        }
        
        return SkApplySign(value, SkExtractSign(packed));
    } else {
        
        value = SkApplySign(value, SkExtractSign(packed));
        exp = -exp;
        if (exp > 25) {   
#ifdef SK_DISCARD_DENORMALIZED_FOR_SPEED
        
        
        
            if (exp > 149) {
                return 0;
            }
            return 0 < value;
#else
            exp = 25;
#endif
        }
        int add = (1 << exp) - 1;
        return (value + add) >> exp;
    }
}

float SkIntToFloatCast(int32_t value) {
    if (0 == value) {
        return 0;
    }

    int shift = EXP_BIAS;

    
    int sign = SkExtractSign(value);
    value = SkApplySign(value, sign);

    if (value >> 24) {    
        int bias = 8 - SkCLZ(value);
        SkDebugf("value = %d, bias = %d\n", value, bias);
        SkASSERT(bias > 0 && bias < 8);
        value >>= bias; 
        shift += bias;
    } else {
        int zeros = SkCLZ(value << 8);
        SkASSERT(zeros >= 0 && zeros <= 23);
        value <<= zeros;
        shift -= zeros;
    }

    
    SkASSERT((value >> 23) == 1);
    SkASSERT(shift >= 0 && shift <= 255);

    SkFloatIntUnion data;
    data.fSignBitInt = (sign << 31) | (shift << 23) | (value & ~MATISSA_MAGIC_BIG);
    return data.fFloat;
}

float SkIntToFloatCast_NoOverflowCheck(int32_t value) {
    if (0 == value) {
        return 0;
    }

    int shift = EXP_BIAS;

    
    int sign = SkExtractSign(value);
    value = SkApplySign(value, sign);

    int zeros = SkCLZ(value << 8);
    value <<= zeros;
    shift -= zeros;

    SkFloatIntUnion data;
    data.fSignBitInt = (sign << 31) | (shift << 23) | (value & ~MATISSA_MAGIC_BIG);
    return data.fFloat;
}
