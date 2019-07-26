





#ifndef jsmath_h
#define jsmath_h

#include "mozilla/MemoryReporting.h"

#include "NamespaceImports.h"

#ifndef M_PI
# define M_PI            3.14159265358979323846
#endif
#ifndef M_E
# define M_E             2.7182818284590452354
#endif
#ifndef M_LOG2E
# define M_LOG2E         1.4426950408889634074
#endif
#ifndef M_LOG10E
# define M_LOG10E        0.43429448190325182765
#endif
#ifndef M_LN2
# define M_LN2           0.69314718055994530942
#endif
#ifndef M_LN10
# define M_LN10          2.30258509299404568402
#endif
#ifndef M_SQRT2
# define M_SQRT2         1.41421356237309504880
#endif
#ifndef M_SQRT1_2
# define M_SQRT1_2       0.70710678118654752440
#endif

namespace js {

typedef double (*UnaryFunType)(double);

class MathCache
{
    static const unsigned SizeLog2 = 12;
    static const unsigned Size = 1 << SizeLog2;
    struct Entry { double in; UnaryFunType f; double out; };
    Entry table[Size];

  public:
    MathCache();

    unsigned hash(double x) {
        union { double d; struct { uint32_t one, two; } s; } u = { x };
        uint32_t hash32 = u.s.one ^ u.s.two;
        uint16_t hash16 = uint16_t(hash32 ^ (hash32 >> 16));
        return (hash16 & (Size - 1)) ^ (hash16 >> (16 - SizeLog2));
    }

    



    double lookup(UnaryFunType f, double x) {
        unsigned index = hash(x);
        Entry &e = table[index];
        if (e.in == x && e.f == f)
            return e.out;
        e.in = x;
        e.f = f;
        return e.out = f(x);
    }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);
};

} 





extern JSObject *
js_InitMathClass(JSContext *cx, js::HandleObject obj);

extern double
math_random_no_outparam(JSContext *cx);

extern bool
js_math_random(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_math_abs(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_math_max(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_math_min(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_math_sqrt(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_math_pow(JSContext *cx, unsigned argc, js::Value *vp);

namespace js {

extern bool
math_imul(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
RoundFloat32(JSContext *cx, Handle<Value> v, float *out);

extern bool
math_fround(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_log(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_log_impl(MathCache *cache, double x);

extern double
math_log_uncached(double x);

extern bool
math_sin(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_sin_impl(double x);

extern bool
math_cos(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_cos_impl(double x);

extern bool
math_exp(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_exp_impl(MathCache *cache, double x);

extern double
math_exp_uncached(double x);

extern bool
math_tan(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_tan_impl(MathCache *cache, double x);

extern double
math_tan_uncached(double x);

extern bool
math_log10(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_log2(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_log1p(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_expm1(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_cosh(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_sinh(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_tanh(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_acosh(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_asinh(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_atanh(JSContext *cx, unsigned argc, js::Value *vp);

extern double
ecmaHypot(double x, double y);

extern bool
math_hypot(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_trunc(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_sign(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_cbrt(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_asin(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_acos(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_atan(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_atan2(JSContext *cx, unsigned argc, Value *vp);

extern double
ecmaAtan2(double x, double y);

extern double
math_atan_impl(MathCache *cache, double x);

extern double
math_atan_uncached(double x);

extern bool
math_atan(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_asin_impl(MathCache *cache, double x);

extern double
math_asin_uncached(double x);

extern bool
math_asin(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_acos_impl(MathCache *cache, double x);

extern double
math_acos_uncached(double x);

extern bool
math_acos(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
math_ceil(JSContext *cx, unsigned argc, Value *vp);

extern double
math_ceil_impl(double x);

extern bool
math_clz32(JSContext *cx, unsigned argc, Value *vp);

extern bool
math_floor(JSContext *cx, unsigned argc, Value *vp);

extern double
math_floor_impl(double x);

extern bool
math_round(JSContext *cx, unsigned argc, Value *vp);

extern double
math_round_impl(double x);

extern float
math_roundf_impl(float x);

extern double
powi(double x, int y);

extern double
ecmaPow(double x, double y);

extern bool
math_imul(JSContext *cx, unsigned argc, Value *vp);

extern double
math_log10_impl(MathCache *cache, double x);

extern double
math_log10_uncached(double x);

extern double
math_log2_impl(MathCache *cache, double x);

extern double
math_log2_uncached(double x);

extern double
math_log1p_impl(MathCache *cache, double x);

extern double
math_log1p_uncached(double x);

extern double
math_expm1_impl(MathCache *cache, double x);

extern double
math_expm1_uncached(double x);

extern double
math_cosh_impl(MathCache *cache, double x);

extern double
math_cosh_uncached(double x);

extern double
math_sinh_impl(MathCache *cache, double x);

extern double
math_sinh_uncached(double x);

extern double
math_tanh_impl(MathCache *cache, double x);

extern double
math_tanh_uncached(double x);

extern double
math_acosh_impl(MathCache *cache, double x);

extern double
math_acosh_uncached(double x);

extern double
math_asinh_impl(MathCache *cache, double x);

extern double
math_asinh_uncached(double x);

extern double
math_atanh_impl(MathCache *cache, double x);

extern double
math_atanh_uncached(double x);

extern double
math_trunc_impl(MathCache *cache, double x);

extern double
math_trunc_uncached(double x);

extern double
math_sign_impl(MathCache *cache, double x);

extern double
math_sign_uncached(double x);

extern double
math_cbrt_impl(MathCache *cache, double x);

extern double
math_cbrt_uncached(double x);

} 

#endif 
