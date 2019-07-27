








#ifndef SkFloatingPoint_DEFINED
#define SkFloatingPoint_DEFINED

#include "SkTypes.h"

#include <math.h>
#include <float.h>


#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

#include "SkFloatBits.h"




static inline float sk_float_pow(float base, float exp) {
    return powf(base, exp);
}

static inline float sk_float_copysign(float x, float y) {

#if (!defined(_MSC_VER) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    return copysign(x, y);


#elif defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
    return copysignf(x, y);


#elif defined(_MSC_VER)
    return (float)_copysign(x, y);


#else
    int32_t xbits = SkFloat2Bits(x);
    int32_t ybits = SkFloat2Bits(y);
    return SkBits2Float((xbits & 0x7FFFFFFF) | (ybits & 0x80000000));
#endif
}

#ifdef SK_BUILD_FOR_WINCE
    #define sk_float_sqrt(x)        (float)::sqrt(x)
    #define sk_float_sin(x)         (float)::sin(x)
    #define sk_float_cos(x)         (float)::cos(x)
    #define sk_float_tan(x)         (float)::tan(x)
    #define sk_float_acos(x)        (float)::acos(x)
    #define sk_float_asin(x)        (float)::asin(x)
    #define sk_float_atan2(y,x)     (float)::atan2(y,x)
    #define sk_float_abs(x)         (float)::fabs(x)
    #define sk_float_mod(x,y)       (float)::fmod(x,y)
    #define sk_float_exp(x)         (float)::exp(x)
    #define sk_float_log(x)         (float)::log(x)
    #define sk_float_floor(x)       (float)::floor(x)
    #define sk_float_ceil(x)        (float)::ceil(x)
#else
    #define sk_float_sqrt(x)        sqrtf(x)
    #define sk_float_sin(x)         sinf(x)
    #define sk_float_cos(x)         cosf(x)
    #define sk_float_tan(x)         tanf(x)
    #define sk_float_floor(x)       floorf(x)
    #define sk_float_ceil(x)        ceilf(x)
#ifdef SK_BUILD_FOR_MAC
    #define sk_float_acos(x)        static_cast<float>(acos(x))
    #define sk_float_asin(x)        static_cast<float>(asin(x))
#else
    #define sk_float_acos(x)        acosf(x)
    #define sk_float_asin(x)        asinf(x)
#endif
    #define sk_float_atan2(y,x)     atan2f(y,x)
    #define sk_float_abs(x)         fabsf(x)
    #define sk_float_mod(x,y)       fmodf(x,y)
    #define sk_float_exp(x)         expf(x)
    #define sk_float_log(x)         logf(x)
#endif

#ifdef SK_BUILD_FOR_WIN
    #define sk_float_isfinite(x)    _finite(x)
    #define sk_float_isnan(x)       _isnan(x)
    static inline int sk_float_isinf(float x) {
        int32_t bits = SkFloat2Bits(x);
        return (bits << 1) == (0xFF << 24);
    }
#else
    #define sk_float_isfinite(x)    isfinite(x)
    #define sk_float_isnan(x)       isnan(x)
    #define sk_float_isinf(x)       isinf(x)
#endif

#define sk_double_isnan(a)          sk_float_isnan(a)

#ifdef SK_USE_FLOATBITS
    #define sk_float_floor2int(x)   SkFloatToIntFloor(x)
    #define sk_float_round2int(x)   SkFloatToIntRound(x)
    #define sk_float_ceil2int(x)    SkFloatToIntCeil(x)
#else
    #define sk_float_floor2int(x)   (int)sk_float_floor(x)
    #define sk_float_round2int(x)   (int)sk_float_floor((x) + 0.5f)
    #define sk_float_ceil2int(x)    (int)sk_float_ceil(x)
#endif

extern const uint32_t gIEEENotANumber;
extern const uint32_t gIEEEInfinity;
extern const uint32_t gIEEENegativeInfinity;

#define SK_FloatNaN                 (*SkTCast<const float*>(&gIEEENotANumber))
#define SK_FloatInfinity            (*SkTCast<const float*>(&gIEEEInfinity))
#define SK_FloatNegativeInfinity    (*SkTCast<const float*>(&gIEEENegativeInfinity))

#if defined(__SSE__)
#include <xmmintrin.h>
#elif defined(__ARM_NEON__)
#include <arm_neon.h>
#endif



static inline float sk_float_rsqrt(const float x) {





#if defined(__SSE__)
    float result;
    _mm_store_ss(&result, _mm_rsqrt_ss(_mm_set_ss(x)));
    return result;
#elif defined(__ARM_NEON__)
    
    const float32x2_t xx = vdup_n_f32(x);  
    float32x2_t estimate = vrsqrte_f32(xx);

    
    const float32x2_t estimate_sq = vmul_f32(estimate, estimate);
    estimate = vmul_f32(estimate, vrsqrts_f32(xx, estimate_sq));
    return vget_lane_f32(estimate, 0);  
#else
    
    int i = *SkTCast<int*>(&x);
    i = 0x5f3759df - (i>>1);
    float estimate = *SkTCast<float*>(&i);

    
    const float estimate_sq = estimate*estimate;
    estimate *= (1.5f-0.5f*x*estimate_sq);
    return estimate;
#endif
}

#endif
