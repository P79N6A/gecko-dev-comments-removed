








#ifndef SkFloatBits_DEFINED
#define SkFloatBits_DEFINED

#include "SkTypes.h"





static inline int32_t SkSignBitTo2sCompliment(int32_t x) {
    if (x < 0) {
        x &= 0x7FFFFFFF;
        x = -x;
    }
    return x;
}




static inline int32_t Sk2sComplimentToSignBit(int32_t x) {
    int sign = x >> 31;
    
    x = (x ^ sign) - sign;
    
    x |= sign << 31;
    return x;
}




int32_t SkFloatBits_toIntCast(int32_t floatBits);




SK_API int32_t SkFloatBits_toIntFloor(int32_t floatBits);




SK_API int32_t SkFloatBits_toIntRound(int32_t floatBits);




SK_API int32_t SkFloatBits_toIntCeil(int32_t floatBits);


#ifdef SK_CAN_USE_FLOAT

union SkFloatIntUnion {
    float   fFloat;
    int32_t fSignBitInt;
};


static inline int32_t SkFloat2Bits(float x) {
    SkFloatIntUnion data;
    data.fFloat = x;
    return data.fSignBitInt;
}


static inline float SkBits2Float(int32_t floatAsBits) {
    SkFloatIntUnion data;
    data.fSignBitInt = floatAsBits;
    return data.fFloat;
}






static inline int32_t SkFloatAs2sCompliment(float x) {
    return SkSignBitTo2sCompliment(SkFloat2Bits(x));
}




static inline float Sk2sComplimentAsFloat(int32_t x) {
    return SkBits2Float(Sk2sComplimentToSignBit(x));
}



float SkIntToFloatCast(int x);
float SkIntToFloatCast_NoOverflowCheck(int x);




static inline int32_t SkFloatToIntCast(float x) {
    return SkFloatBits_toIntCast(SkFloat2Bits(x));
}




static inline int32_t SkFloatToIntFloor(float x) {
    return SkFloatBits_toIntFloor(SkFloat2Bits(x));
}




static inline int32_t SkFloatToIntRound(float x) {
    return SkFloatBits_toIntRound(SkFloat2Bits(x));
}




static inline int32_t SkFloatToIntCeil(float x) {
    return SkFloatBits_toIntCeil(SkFloat2Bits(x));
}

#endif



#ifdef SK_SCALAR_IS_FLOAT
    #define SkScalarAs2sCompliment(x)    SkFloatAs2sCompliment(x)
    #define Sk2sComplimentAsScalar(x)    Sk2sComplimentAsFloat(x)
#else
    #define SkScalarAs2sCompliment(x)    (x)
    #define Sk2sComplimentAsScalar(x)    (x)
#endif

#endif

