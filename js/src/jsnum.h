






































#ifndef jsnum_h___
#define jsnum_h___

#include <math.h>
#if defined(XP_WIN) || defined(XP_OS2)
#include <float.h>
#endif
#ifdef SOLARIS
#include <ieeefp.h>
#endif

#include "jsstdint.h"
#include "jsstr.h"















#if defined(__arm) || defined(__arm32__) || defined(__arm26__) || defined(__arm__)
#if !defined(__VFP_FP__)
#define FPU_IS_ARM_FPA
#endif
#endif

typedef union jsdpun {
    struct {
#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
        uint32 lo, hi;
#else
        uint32 hi, lo;
#endif
    } s;
    uint64   u64;
    jsdouble d;
} jsdpun;

static inline int
JSDOUBLE_IS_NaN(jsdouble d)
{
#ifdef WIN32
    return _isnan(d);
#else
    return isnan(d);
#endif
}

static inline int
JSDOUBLE_IS_FINITE(jsdouble d)
{
#ifdef WIN32
    return _finite(d);
#else
    return finite(d);
#endif
}

static inline int
JSDOUBLE_IS_INFINITE(jsdouble d)
{
#ifdef WIN32
    int c = _fpclass(d);
    return c == _FPCLASS_NINF || c == _FPCLASS_PINF;
#elif defined(SOLARIS)
    return !finite(d) && !isnan(d);
#else
    return isinf(d);
#endif
}

static inline int
JSDOUBLE_IS_NEGZERO(jsdouble d)
{
#ifdef WIN32
    return (d == 0 && (_fpclass(d) & _FPCLASS_NZ));
#elif defined(SOLARIS)
    return (d == 0 && copysign(1, d) < 0);
#else
    return (d == 0 && signbit(d));
#endif
}

#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff

static inline bool
JSDOUBLE_IS_INT32(jsdouble d, int32_t& i)
{
    if (JSDOUBLE_IS_NEGZERO(d))
        return false;
    return d == (i = int32_t(d));
}

static inline bool
JSDOUBLE_IS_NEG(jsdouble d)
{
#ifdef WIN32
    return JSDOUBLE_IS_NEGZERO(d) || d < 0;
#elif defined(SOLARIS)
    return copysign(1, d) < 0;
#else
    return signbit(d);
#endif
}

static inline uint32
JS_HASH_DOUBLE(jsdouble d)
{
    jsdpun u;
    u.d = d;
    return u.s.lo ^ u.s.hi;
}

#if defined(XP_WIN)
#define JSDOUBLE_COMPARE(LVAL, OP, RVAL, IFNAN)                               \
    ((JSDOUBLE_IS_NaN(LVAL) || JSDOUBLE_IS_NaN(RVAL))                         \
     ? (IFNAN)                                                                \
     : (LVAL) OP (RVAL))
#else
#define JSDOUBLE_COMPARE(LVAL, OP, RVAL, IFNAN) ((LVAL) OP (RVAL))
#endif

extern jsdouble js_NaN;
extern jsdouble js_PositiveInfinity;
extern jsdouble js_NegativeInfinity;


extern JSBool
js_InitRuntimeNumberState(JSContext *cx);

extern void
js_TraceRuntimeNumberState(JSTracer *trc);

extern void
js_FinishRuntimeNumberState(JSContext *cx);


extern js::Class js_NumberClass;

extern JSObject *
js_InitNumberClass(JSContext *cx, JSObject *obj);




extern const char js_Infinity_str[];
extern const char js_NaN_str[];
extern const char js_isNaN_str[];
extern const char js_isFinite_str[];
extern const char js_parseFloat_str[];
extern const char js_parseInt_str[];


extern JSString * JS_FASTCALL
js_NumberToString(JSContext *cx, jsdouble d);





extern JSBool JS_FASTCALL
js_NumberValueToCharBuffer(JSContext *cx, jsval v, JSCharBuffer &cb);

namespace js {





JS_ALWAYS_INLINE bool
ValueToNumber(JSContext *cx, js::Value *vp, double *out)
{
    if (vp->isInt32()) {
        *out = vp->asInt32();
        return true;
    }
    if (vp->isDouble()) {
        *out = vp->asDouble();
        return true;
    }
    extern bool ValueToNumberSlow(JSContext *, js::Value *, double *);
    return ValueToNumberSlow(cx, vp, out);
}


JS_ALWAYS_INLINE bool
ValueToNumber(JSContext *cx, js::Value *vp)
{
    if (vp->isInt32())
        return true;
    if (vp->isDouble())
        return true;
    double _;
    extern bool ValueToNumberSlow(JSContext *, js::Value *, double *);
    return ValueToNumberSlow(cx, vp, &_);
}






JS_ALWAYS_INLINE bool
ValueToECMAInt32(JSContext *cx, js::Value *vp, int32_t *out)
{
    if (vp->isInt32()) {
        *out = vp->asInt32();
        return true;
    }
    extern bool ValueToECMAInt32Slow(JSContext *, js::Value *, int32_t *);
    return ValueToECMAInt32Slow(cx, vp, out);
}

JS_ALWAYS_INLINE bool
ValueToECMAUint32(JSContext *cx, js::Value *vp, uint32_t *out)
{
    if (vp->isInt32()) {
        *out = (uint32_t)vp->asInt32();
        return true;
    }
    extern bool ValueToECMAUint32Slow(JSContext *, js::Value *, uint32_t *);
    return ValueToECMAUint32Slow(cx, vp, out);
}






JS_ALWAYS_INLINE bool
ValueToInt32(JSContext *cx, js::Value *vp, int32_t *out)
{
    if (vp->isInt32()) {
        *out = vp->asInt32();
        return true;
    }
    extern bool ValueToInt32Slow(JSContext *, js::Value *, int32_t *);
    return ValueToInt32Slow(cx, vp, out);
}






JS_ALWAYS_INLINE bool
ValueToUint16(JSContext *cx, js::Value *vp, uint16_t *out)
{
    if (vp->isInt32()) {
        *out = (uint16_t)vp->asInt32();
        return true;
    }
    extern bool ValueToUint16Slow(JSContext *, js::Value *, uint16_t *);
    return ValueToUint16Slow(cx, vp, out);
}

}  














static inline int32
js_DoubleToECMAInt32(jsdouble d)
{
#if defined(__i386__) || defined(__i386)
    jsdpun du, duh, two32;
    uint32 di_h, u_tmp, expon, shift_amount;
    int32 mask32;

    











    du.d = d;
    di_h = du.s.hi;

    u_tmp = (di_h & 0x7ff00000) - 0x3ff00000;
    if (u_tmp >= (0x45300000-0x3ff00000)) {
        
        return 0;
    }

    if (u_tmp < 0x01f00000) {
        
        return int32_t(d);
    }

    if (u_tmp > 0x01f00000) {
        
        expon = u_tmp >> 20;
        shift_amount = expon - 21;
        duh.u64 = du.u64;
        mask32 = 0x80000000;
        if (shift_amount < 32) {
            mask32 >>= shift_amount;
            duh.s.hi = du.s.hi & mask32;
            duh.s.lo = 0;
        } else {
            mask32 >>= (shift_amount-32);
            duh.s.hi = du.s.hi;
            duh.s.lo = du.s.lo & mask32;
        }
        du.d -= duh.d;
    }

    di_h = du.s.hi;

    
    u_tmp = (di_h & 0x7ff00000);
    if (u_tmp >= 0x41e00000) {
        
        expon = u_tmp >> 20;
        shift_amount = expon - (0x3ff - 11);
        mask32 = 0x80000000;
        if (shift_amount < 32) {
            mask32 >>= shift_amount;
            du.s.hi &= mask32;
            du.s.lo = 0;
        } else {
            mask32 >>= (shift_amount-32);
            du.s.lo &= mask32;
        }
        two32.s.hi = 0x41f00000 ^ (du.s.hi & 0x80000000);
        two32.s.lo = 0;
        du.d -= two32.d;
    }

    return int32(du.d);
#else
    int32 i;
    jsdouble two32, two31;

    if (!JSDOUBLE_IS_FINITE(d))
        return 0;

    i = (int32) d;
    if ((jsdouble) i == d)
        return i;

    two32 = 4294967296.0;
    two31 = 2147483648.0;
    d = fmod(d, two32);
    d = (d >= 0) ? floor(d) : ceil(d) + two32;
    return (int32) (d >= two31 ? d - two32 : d);
#endif
}

uint32
js_DoubleToECMAUint32(jsdouble d);





static inline jsdouble
js_DoubleToInteger(jsdouble d)
{
    if (d == 0)
        return d;

    if (!JSDOUBLE_IS_FINITE(d)) {
        if (JSDOUBLE_IS_NaN(d))
            return 0;
        return d;
    }

    JSBool neg = (d < 0);
    d = floor(neg ? -d : d);

    return neg ? -d : d;
}











extern JSBool
js_strtod(JSContext *cx, const jschar *s, const jschar *send,
          const jschar **ep, jsdouble *dp);










extern JSBool
js_strtointeger(JSContext *cx, const jschar *s, const jschar *send,
                const jschar **ep, jsint radix, jsdouble *dp);

namespace js {

static JS_ALWAYS_INLINE bool
ValueFitsInInt32(const Value &v, int32_t *pi)
{
    if (v.isInt32()) {
        *pi = v.asInt32();
        return true;
    }
    return v.isDouble() && JSDOUBLE_IS_INT32(v.asDouble(), *pi);
}

static JS_ALWAYS_INLINE void
Uint32ToValue(uint32_t u, Value *vp)
{
    if (JS_UNLIKELY(u > INT32_MAX))
        vp->setDouble(u);
    else
        vp->setInt32((int32_t)u);
}

template<typename T> struct NumberTraits { };
template<> struct NumberTraits<int32> {
  static JS_ALWAYS_INLINE int32 NaN() { return 0; }
  static JS_ALWAYS_INLINE int32 toSelfType(int32 i) { return i; }
  static JS_ALWAYS_INLINE int32 toSelfType(jsdouble d) { return js_DoubleToECMAUint32(d); }
};
template<> struct NumberTraits<jsdouble> {
  static JS_ALWAYS_INLINE jsdouble NaN() { return js_NaN; }
  static JS_ALWAYS_INLINE jsdouble toSelfType(int32 i) { return i; }
  static JS_ALWAYS_INLINE jsdouble toSelfType(jsdouble d) { return d; }
};

template<typename T>
static JS_ALWAYS_INLINE T
StringToNumberType(JSContext *cx, JSString *str)
{
    if (str->length() == 1) {
        jschar c = str->chars()[0];
        if ('0' <= c && c <= '9')
            return NumberTraits<T>::toSelfType(T(c - '0'));
        if (JS_ISSPACE(c))
            return NumberTraits<T>::toSelfType(T(0));
        return NumberTraits<T>::NaN();
    }

    const jschar* bp;
    const jschar* end;
    const jschar* ep;
    jsdouble d;

    str->getCharsAndEnd(bp, end);
    bp = js_SkipWhiteSpace(bp, end);

    
    if (end - bp >= 2 && bp[0] == '0' && (bp[1] == 'x' || bp[1] == 'X')) {
        
        if (!js_strtointeger(cx, bp, end, &ep, 16, &d) ||
            js_SkipWhiteSpace(ep, end) != end) {
            return NumberTraits<T>::NaN();
        }
        return NumberTraits<T>::toSelfType(d);
    }

    






    if (!js_strtod(cx, bp, end, &ep, &d) ||
        js_SkipWhiteSpace(ep, end) != end) {
        return NumberTraits<T>::NaN();
    }

    return NumberTraits<T>::toSelfType(d);
}
}

#endif 
