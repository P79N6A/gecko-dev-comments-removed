






#ifndef SkScalar_DEFINED
#define SkScalar_DEFINED

#include "SkFixed.h"
#include "SkFloatingPoint.h"



typedef float   SkScalar;



#define SK_Scalar1              (1.0f)


#define SK_ScalarHalf           (0.5f)


#define SK_ScalarInfinity       SK_FloatInfinity


#define SK_ScalarNegativeInfinity       SK_FloatNegativeInfinity


#define SK_ScalarMax            (3.402823466e+38f)


#define SK_ScalarMin            (-SK_ScalarMax)


#define SK_ScalarNaN            SK_FloatNaN


static inline bool SkScalarIsNaN(float x) { return x != x; }


static inline bool SkScalarIsFinite(float x) {
    
    
    
    
    float prod = x * 0;
    
    
    return prod == prod;
}



#define SkIntToScalar(n)        ((float)(n))


#define SkFixedToScalar(x)      SkFixedToFloat(x)


#define SkScalarToFixed(x)      SkFloatToFixed(x)

#define SkScalarToFloat(n)      (n)
#ifndef SK_SCALAR_TO_FLOAT_EXCLUDED
#define SkFloatToScalar(n)      (n)
#endif

#define SkScalarToDouble(n)      (double)(n)
#define SkDoubleToScalar(n)      (float)(n)



#define SkScalarFraction(x)     sk_float_mod(x, 1.0f)

#define SkScalarFloorToScalar(x)    sk_float_floor(x)
#define SkScalarCeilToScalar(x)     sk_float_ceil(x)
#define SkScalarRoundToScalar(x)    sk_float_floor((x) + 0.5f)

#define SkScalarFloorToInt(x)       sk_float_floor2int(x)
#define SkScalarCeilToInt(x)        sk_float_ceil2int(x)
#define SkScalarRoundToInt(x)       sk_float_round2int(x)
#define SkScalarTruncToInt(x)       static_cast<int>(x)















static inline int SkDScalarRoundToInt(SkScalar x) {
    double xx = x;
    xx += 0.5;
    return (int)floor(xx);
}



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


#define SkScalarDiv(a, b)       ((float)(a) / (b))


#define SkScalarMod(x,y)        sk_float_mod(x,y)


#define SkScalarMulDiv(a, b, c) ((float)(a) * (b) / (c))


#define SkScalarInvert(x)       (SK_Scalar1 / (x))
#define SkScalarFastInvert(x)   (SK_Scalar1 / (x))


#define SkScalarSqrt(x)         sk_float_sqrt(x)


#define SkScalarPow(b, e)       sk_float_pow(b, e)


#define SkScalarAve(a, b)       (((a) + (b)) * 0.5f)


#define SkScalarHalf(a)         ((a) * 0.5f)

#define SK_ScalarSqrt2          1.41421356f
#define SK_ScalarPI             3.14159265f
#define SK_ScalarTanPIOver8     0.414213562f
#define SK_ScalarRoot2Over2     0.707106781f

#define SkDegreesToRadians(degrees) ((degrees) * (SK_ScalarPI / 180))
#define SkRadiansToDegrees(radians) ((radians) * (180 / SK_ScalarPI))
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


#ifdef SK_SUPPORT_DEPRECATED_SCALARROUND
#   define SkScalarFloor(x)    SkScalarFloorToInt(x)
#   define SkScalarCeil(x)     SkScalarCeilToInt(x)
#   define SkScalarRound(x)    SkScalarRoundToInt(x)
#endif







static inline int SkScalarSignAsInt(SkScalar x) {
    return x < 0 ? -1 : (x > 0);
}


static inline SkScalar SkScalarSignAsScalar(SkScalar x) {
    return x < 0 ? -SK_Scalar1 : ((x > 0) ? SK_Scalar1 : 0);
}

#define SK_ScalarNearlyZero         (SK_Scalar1 / (1 << 12))

static inline bool SkScalarNearlyZero(SkScalar x,
                                    SkScalar tolerance = SK_ScalarNearlyZero) {
    SkASSERT(tolerance >= 0);
    return SkScalarAbs(x) <= tolerance;
}

static inline bool SkScalarNearlyEqual(SkScalar x, SkScalar y,
                                     SkScalar tolerance = SK_ScalarNearlyZero) {
    SkASSERT(tolerance >= 0);
    return SkScalarAbs(x-y) <= tolerance;
}







static inline SkScalar SkScalarInterp(SkScalar A, SkScalar B, SkScalar t) {
    SkASSERT(t >= 0 && t <= SK_Scalar1);
    return A + (B - A) * t;
}











SkScalar SkScalarInterpFunc(SkScalar searchKey, const SkScalar keys[],
                            const SkScalar values[], int length);




static inline bool SkScalarsEqual(const SkScalar a[], const SkScalar b[], int n) {
    SkASSERT(n >= 0);
    for (int i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

#endif
