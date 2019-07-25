






































#ifndef jsmath_h___
#define jsmath_h___

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

    uintN hash(double x) {
        union { double d; struct { uint32 one, two; } s; } u = { x };
        uint32 hash32 = u.s.one ^ u.s.two;
        uint16 hash16 = (uint16)(hash32 ^ (hash32 >> 16));
        return (hash16 & (Size - 1)) ^ (hash16 >> (16 - SizeLog2));
    }

    



    double lookup(UnaryFunType f, double x) {
        uintN index = hash(x);
        Entry &e = table[index];
        if (e.in == x && e.f == f)
            return e.out;
        e.in = x;
        e.f = f;
        return (e.out = f(x));
    }
};

} 





extern js::Class js_MathClass;

extern JSObject *
js_InitMathClass(JSContext *cx, JSObject *obj);

extern bool
js_IsMathFunction(JSNative native);

extern void
js_InitRandom(JSContext *cx);

extern JSBool
js_math_ceil(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_floor(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_max(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_min(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_round(JSContext *cx, uintN argc, js::Value *vp);

extern jsdouble
js_math_ceil_impl(jsdouble x);

extern jsdouble
js_math_floor_impl(jsdouble x);

extern jsdouble
js_math_round_impl(jsdouble x);

#endif 
