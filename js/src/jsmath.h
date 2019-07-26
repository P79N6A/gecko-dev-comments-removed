





#ifndef jsmath_h
#define jsmath_h

#include "mozilla/MemoryReporting.h"

#include "jsapi.h"

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
        return (e.out = f(x));
    }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);
};

} 





extern JSObject *
js_InitMathClass(JSContext *cx, js::HandleObject obj);

extern double
math_random_no_outparam(JSContext *cx);

extern JSBool
js_math_random(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_abs(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_ceil(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_floor(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_max(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_min(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_round(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_sqrt(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
js_math_pow(JSContext *cx, unsigned argc, js::Value *vp);

extern double
js_math_ceil_impl(double x);

extern double
js_math_floor_impl(double x);

namespace js {

extern JSBool
math_exp(JSContext *cx, unsigned argc, Value *vp);

extern JSBool
math_imul(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
math_log(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_log_impl(MathCache *cache, double x);

extern JSBool
math_sin(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_sin_impl(MathCache *cache, double x);

extern JSBool
math_cos(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_cos_impl(MathCache *cache, double x);

extern JSBool
math_exp(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_exp_impl(MathCache *cache, double x);

extern JSBool
math_tan(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_tan_impl(MathCache *cache, double x);

extern JSBool
math_asin(JSContext *cx, unsigned argc, Value *vp);

extern JSBool
math_acos(JSContext *cx, unsigned argc, Value *vp);

extern JSBool
math_atan(JSContext *cx, unsigned argc, Value *vp);

extern JSBool
math_atan2(JSContext *cx, unsigned argc, Value *vp);

extern double
ecmaAtan2(double x, double y);

extern double
math_atan_impl(MathCache *cache, double x);

extern JSBool
math_atan(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_asin_impl(MathCache *cache, double x);

extern JSBool
math_asin(JSContext *cx, unsigned argc, js::Value *vp);

extern double
math_acos_impl(MathCache *cache, double x);

extern JSBool
math_acos(JSContext *cx, unsigned argc, js::Value *vp);

extern double
powi(double x, int y);

extern double
ecmaPow(double x, double y);

extern JSBool
math_imul(JSContext *cx, unsigned argc, Value *vp);

} 

#endif 
