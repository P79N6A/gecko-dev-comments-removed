








#ifndef SkScalar_DEFINED
#define SkScalar_DEFINED

#include "SkFixed.h"
#include "SkFloatingPoint.h"










#ifdef SK_SCALAR_IS_FLOAT

    



    typedef float   SkScalar;
    extern const uint32_t gIEEENotANumber;
    extern const uint32_t gIEEEInfinity;

    

    #define SK_Scalar1              (1.0f)
    

    #define SK_ScalarHalf           (0.5f)
    

    #define SK_ScalarInfinity           (*(const float*)&gIEEEInfinity)
    

    #define SK_ScalarMax            (3.402823466e+38f)
    

    #define SK_ScalarMin            (-SK_ScalarMax)
    

    #define SK_ScalarNaN      (*(const float*)(const void*)&gIEEENotANumber)
    

    static inline bool SkScalarIsNaN(float x) { return x != x; }
    
    static inline bool SkScalarIsFinite(float x) {
        uint32_t bits = SkFloat2Bits(x);    
        int exponent = bits << 1 >> 24;
        return exponent != 0xFF;
    }
#ifdef SK_DEBUG
    











    static inline float SkIntToScalar(signed int param) {
        return (float)param;
    }
    static inline float SkIntToScalar(unsigned int param) {
        return (float)param;
    }
    static inline float SkIntToScalar(signed long param) {
        return (float)param;
    }
    static inline float SkIntToScalar(unsigned long param) {
        return (float)param;
    }
    static inline float SkIntToScalar(float param) {
        






        SkASSERT(!"looks like you passed an SkScalar into SkIntToScalar");
        return (float)0;
    }
#else  
    

    #define SkIntToScalar(n)        ((float)(n))
#endif 
    

    #define SkFixedToScalar(x)      SkFixedToFloat(x)
    

    #define SkScalarToFixed(x)      SkFloatToFixed(x)

    #define SkScalarToFloat(n)      (n)
    #define SkFloatToScalar(n)      (n)

    #define SkScalarToDouble(n)      (double)(n)
    #define SkDoubleToScalar(n)      (float)(n)

    

    #define SkScalarFraction(x)     sk_float_mod(x, 1.0f)

    #define SkScalarFloorToScalar(x)    sk_float_floor(x)
    #define SkScalarCeilToScalar(x)     sk_float_ceil(x)
    #define SkScalarRoundToScalar(x)    sk_float_round(x)

    #define SkScalarFloorToInt(x)       sk_float_floor2int(x)
    #define SkScalarCeilToInt(x)        sk_float_ceil2int(x)
    #define SkScalarRoundToInt(x)       sk_float_round2int(x)

    

    #define SkScalarAbs(x)          sk_float_abs(x)
    

    #define SkScalarCopySign(x, y)  sk_float_copysign(x, y)
    

    inline SkScalar SkScalarClampMax(SkScalar x, SkScalar max) {
        return x < 0 ? 0 : x > max ? max : x;
    }
    

    inline SkScalar SkScalarPin(SkScalar x, SkScalar min, SkScalar max) {
        return x < min ? min : x > max ? max : x;
    }
    

    inline SkScalar SkScalarSquare(SkScalar x) { return x * x; }
    

    #define SkScalarMul(a, b)       ((float)(a) * (b))
    

    #define SkScalarMulAdd(a, b, c) ((float)(a) * (b) + (c))
    

    #define SkScalarMulRound(a, b) SkScalarRound((float)(a) * (b))
    

    #define SkScalarMulCeil(a, b) SkScalarCeil((float)(a) * (b))
    

    #define SkScalarMulFloor(a, b) SkScalarFloor((float)(a) * (b))
    

    #define SkScalarDiv(a, b)       ((float)(a) / (b))
    

    #define SkScalarMod(x,y)        sk_float_mod(x,y)
    

    #define SkScalarMulDiv(a, b, c) ((float)(a) * (b) / (c))
    

    #define SkScalarInvert(x)       (SK_Scalar1 / (x))
    #define SkScalarFastInvert(x)   (SK_Scalar1 / (x))
    

    #define SkScalarSqrt(x)         sk_float_sqrt(x)
    

    #define SkScalarAve(a, b)       (((a) + (b)) * 0.5f)
    

    #define SkScalarMean(a, b)      sk_float_sqrt((float)(a) * (b))
    

    #define SkScalarHalf(a)         ((a) * 0.5f)

    #define SK_ScalarSqrt2          1.41421356f
    #define SK_ScalarPI             3.14159265f
    #define SK_ScalarTanPIOver8     0.414213562f
    #define SK_ScalarRoot2Over2     0.707106781f

    #define SkDegreesToRadians(degrees) ((degrees) * (SK_ScalarPI / 180))
    float SkScalarSinCos(SkScalar radians, SkScalar* cosValue);
    #define SkScalarSin(radians)    (float)sk_float_sin(radians)
    #define SkScalarCos(radians)    (float)sk_float_cos(radians)
    #define SkScalarTan(radians)    (float)sk_float_tan(radians)
    #define SkScalarASin(val)   (float)sk_float_asin(val)
    #define SkScalarACos(val)   (float)sk_float_acos(val)
    #define SkScalarATan2(y, x) (float)sk_float_atan2(y,x)
    #define SkScalarExp(x)  (float)sk_float_exp(x)
    #define SkScalarLog(x)  (float)sk_float_log(x)

    inline SkScalar SkMaxScalar(SkScalar a, SkScalar b) { return a > b ? a : b; }
    inline SkScalar SkMinScalar(SkScalar a, SkScalar b) { return a < b ? a : b; }

    static inline bool SkScalarIsInt(SkScalar x) {
        return x == (float)(int)x;
    }
#else
    typedef SkFixed SkScalar;

    #define SK_Scalar1              SK_Fixed1
    #define SK_ScalarHalf           SK_FixedHalf
    #define SK_ScalarInfinity   SK_FixedMax
    #define SK_ScalarMax            SK_FixedMax
    #define SK_ScalarMin            SK_FixedMin
    #define SK_ScalarNaN            SK_FixedNaN
    #define SkScalarIsNaN(x)        ((x) == SK_FixedNaN)
    #define SkScalarIsFinite(x)     ((x) != SK_FixedNaN)

    #define SkIntToScalar(n)        SkIntToFixed(n)
    #define SkFixedToScalar(x)      (x)
    #define SkScalarToFixed(x)      (x)
    #ifdef SK_CAN_USE_FLOAT
        #define SkScalarToFloat(n)  SkFixedToFloat(n)
        #define SkFloatToScalar(n)  SkFloatToFixed(n)

        #define SkScalarToDouble(n) SkFixedToDouble(n)
        #define SkDoubleToScalar(n) SkDoubleToFixed(n)
    #endif
    #define SkScalarFraction(x)     SkFixedFraction(x)

    #define SkScalarFloorToScalar(x)    SkFixedFloorToFixed(x)
    #define SkScalarCeilToScalar(x)     SkFixedCeilToFixed(x)
    #define SkScalarRoundToScalar(x)    SkFixedRoundToFixed(x)

    #define SkScalarFloorToInt(x)       SkFixedFloorToInt(x)
    #define SkScalarCeilToInt(x)        SkFixedCeilToInt(x)
    #define SkScalarRoundToInt(x)       SkFixedRoundToInt(x)

    #define SkScalarAbs(x)          SkFixedAbs(x)
    #define SkScalarCopySign(x, y)  SkCopySign32(x, y)
    #define SkScalarClampMax(x, max) SkClampMax(x, max)
    #define SkScalarPin(x, min, max) SkPin32(x, min, max)
    #define SkScalarSquare(x)       SkFixedSquare(x)
    #define SkScalarMul(a, b)       SkFixedMul(a, b)
    #define SkScalarMulAdd(a, b, c) SkFixedMulAdd(a, b, c)
    #define SkScalarMulRound(a, b)  SkFixedMulCommon(a, b, SK_FixedHalf)
    #define SkScalarMulCeil(a, b)   SkFixedMulCommon(a, b, SK_Fixed1 - 1)
    #define SkScalarMulFloor(a, b)  SkFixedMulCommon(a, b, 0)
    #define SkScalarDiv(a, b)       SkFixedDiv(a, b)
    #define SkScalarMod(a, b)       SkFixedMod(a, b)
    #define SkScalarMulDiv(a, b, c) SkMulDiv(a, b, c)
    #define SkScalarInvert(x)       SkFixedInvert(x)
    #define SkScalarFastInvert(x)   SkFixedFastInvert(x)
    #define SkScalarSqrt(x)         SkFixedSqrt(x)
    #define SkScalarAve(a, b)       SkFixedAve(a, b)
    #define SkScalarMean(a, b)      SkFixedMean(a, b)
    #define SkScalarHalf(a)         ((a) >> 1)

    #define SK_ScalarSqrt2          SK_FixedSqrt2
    #define SK_ScalarPI             SK_FixedPI
    #define SK_ScalarTanPIOver8     SK_FixedTanPIOver8
    #define SK_ScalarRoot2Over2     SK_FixedRoot2Over2

    #define SkDegreesToRadians(degrees)     SkFractMul(degrees, SK_FractPIOver180)
    #define SkScalarSinCos(radians, cosPtr) SkFixedSinCos(radians, cosPtr)
    #define SkScalarSin(radians)    SkFixedSin(radians)
    #define SkScalarCos(radians)    SkFixedCos(radians)
    #define SkScalarTan(val)        SkFixedTan(val)
    #define SkScalarASin(val)       SkFixedASin(val)
    #define SkScalarACos(val)       SkFixedACos(val)
    #define SkScalarATan2(y, x)     SkFixedATan2(y,x)
    #define SkScalarExp(x)          SkFixedExp(x)
    #define SkScalarLog(x)          SkFixedLog(x)

    #define SkMaxScalar(a, b)       SkMax32(a, b)
    #define SkMinScalar(a, b)       SkMin32(a, b)

    static inline bool SkScalarIsInt(SkFixed x) {
        return 0 == (x & 0xffff);
    }
#endif


#define SkScalarFloor(x)    SkScalarFloorToInt(x)
#define SkScalarCeil(x)     SkScalarCeilToInt(x)
#define SkScalarRound(x)    SkScalarRoundToInt(x)







static inline int SkScalarSignAsInt(SkScalar x) {
    return x < 0 ? -1 : (x > 0);
}


static inline SkScalar SkScalarSignAsScalar(SkScalar x) {
    return x < 0 ? -SK_Scalar1 : ((x > 0) ? SK_Scalar1 : 0);
}

#define SK_ScalarNearlyZero         (SK_Scalar1 / (1 << 12))




static inline bool SkScalarNearlyZero(SkScalar x,
                                    SkScalar tolerance = SK_ScalarNearlyZero) {
    SkASSERT(tolerance > 0);
    return SkScalarAbs(x) < tolerance;
}

static inline bool SkScalarNearlyEqual(SkScalar x, SkScalar y,
                                     SkScalar tolerance = SK_ScalarNearlyZero) {
    SkASSERT(tolerance > 0);
    return SkScalarAbs(x-y) < tolerance;
}







static inline SkScalar SkScalarInterp(SkScalar A, SkScalar B, SkScalar t) {
    SkASSERT(t >= 0 && t <= SK_Scalar1);
    return A + SkScalarMul(B - A, t);
}











SkScalar SkScalarInterpFunc(SkScalar searchKey, const SkScalar keys[],
                            const SkScalar values[], int length);

#endif
