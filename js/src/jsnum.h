






































#ifndef jsnum_h___
#define jsnum_h___

#include <math.h>
#ifdef WIN32
#include <float.h>
#endif
#include "jsvalue.h"

#include "jsstdint.h"
#include "jsstr.h"
#include "jsobj.h"















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
    jsdpun u;
    u.d = d;
    return (u.u64 & ~JSDOUBLE_SIGNBIT) > JSDOUBLE_EXPMASK;
#endif
}

static inline int
JSDOUBLE_IS_FINITE(jsdouble d)
{
    
    jsdpun u;
    u.d = d;
    return (u.u64 & JSDOUBLE_EXPMASK) != JSDOUBLE_EXPMASK;
}

static inline int
JSDOUBLE_IS_INFINITE(jsdouble d)
{
    jsdpun u;
    u.d = d;
    return (u.u64 & ~JSDOUBLE_SIGNBIT) == JSDOUBLE_EXPMASK;
}

#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff
#define JSDOUBLE_HI32_NAN       0x7ff80000
#define JSDOUBLE_LO32_NAN       0x00000000

static inline bool
JSDOUBLE_IS_NEG(jsdouble d)
{
    jsdpun u;
    u.d = d;
    return (u.s.hi & JSDOUBLE_HI32_SIGNBIT) != 0;
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
js_FinishRuntimeNumberState(JSContext *cx);


extern js::Class js_NumberClass;

inline bool
JSObject::isNumber() const
{
    return getClass() == &js_NumberClass;
}

extern JSObject *
js_InitNumberClass(JSContext *cx, JSObject *obj);




extern const char js_Infinity_str[];
extern const char js_NaN_str[];
extern const char js_isNaN_str[];
extern const char js_isFinite_str[];
extern const char js_parseFloat_str[];
extern const char js_parseInt_str[];

extern JSString * JS_FASTCALL
js_IntToString(JSContext *cx, jsint i);






extern JSString * JS_FASTCALL
js_NumberToString(JSContext *cx, jsdouble d);

namespace js {





extern bool JS_FASTCALL
NumberValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);


extern JSFixedString *
NumberToString(JSContext *cx, jsdouble d);






struct ToCStringBuf
{
    




    static const size_t sbufSize = 34;
    char sbuf[sbufSize];
    char *dbuf;

    ToCStringBuf();
    ~ToCStringBuf();
};







extern char *
NumberToCString(JSContext *cx, ToCStringBuf *cbuf, jsdouble d, jsint base = 10);





const double DOUBLE_INTEGRAL_PRECISION_LIMIT = uint64(1) << 53;













extern bool
GetPrefixInteger(JSContext *cx, const jschar *start, const jschar *end, int base,
                 const jschar **endp, jsdouble *dp);





JS_ALWAYS_INLINE bool
ValueToNumber(JSContext *cx, const js::Value &v, double *out)
{
    if (v.isNumber()) {
        *out = v.toNumber();
        return true;
    }
    extern bool ValueToNumberSlow(JSContext *, js::Value, double *);
    return ValueToNumberSlow(cx, v, out);
}


JS_ALWAYS_INLINE bool
ValueToNumber(JSContext *cx, js::Value *vp)
{
    if (vp->isNumber())
        return true;
    double d;
    extern bool ValueToNumberSlow(JSContext *, js::Value, double *);
    if (!ValueToNumberSlow(cx, *vp, &d))
        return false;
    vp->setNumber(d);
    return true;
}






JS_ALWAYS_INLINE bool
ValueToECMAInt32(JSContext *cx, const js::Value &v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    extern bool ValueToECMAInt32Slow(JSContext *, const js::Value &, int32_t *);
    return ValueToECMAInt32Slow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ValueToECMAUint32(JSContext *cx, const js::Value &v, uint32_t *out)
{
    if (v.isInt32()) {
        *out = (uint32_t)v.toInt32();
        return true;
    }
    extern bool ValueToECMAUint32Slow(JSContext *, const js::Value &, uint32_t *);
    return ValueToECMAUint32Slow(cx, v, out);
}






JS_ALWAYS_INLINE bool
ValueToInt32(JSContext *cx, const js::Value &v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    extern bool ValueToInt32Slow(JSContext *, const js::Value &, int32_t *);
    return ValueToInt32Slow(cx, v, out);
}






JS_ALWAYS_INLINE bool
ValueToUint16(JSContext *cx, const js::Value &v, uint16_t *out)
{
    if (v.isInt32()) {
        *out = (uint16_t)v.toInt32();
        return true;
    }
    extern bool ValueToUint16Slow(JSContext *, const js::Value &, uint16_t *);
    return ValueToUint16Slow(cx, v, out);
}

}  














static inline int32
js_DoubleToECMAInt32(jsdouble d)
{
#if defined(__i386__) || defined(__i386) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
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
#elif defined (__arm__) && defined (__GNUC__)
    int32_t i;
    uint32_t    tmp0;
    uint32_t    tmp1;
    uint32_t    tmp2;
    asm (
    
    
    
    
    

    
    
    
    

    
"   mov     %1, %R4, LSR #20\n"
"   bic     %1, %1, #(1 << 11)\n"  

    
    
"   orr     %R4, %R4, #(1 << 20)\n"

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
"   sub     %1, %1, #0xff\n"
"   subs    %1, %1, #0x300\n"
"   bmi     8f\n"

    
    

    
"   subs    %3, %1, #52\n"         
"   bmi     1f\n"

    
    
    
    
    
    
"   bic     %2, %3, #0xff\n"
"   orr     %3, %3, %2, LSR #3\n"
    
    
"   mov     %Q4, %Q4, LSL %3\n"
"   b       2f\n"
"1:\n" 
    
    
"   rsb     %3, %1, #52\n"
"   mov     %Q4, %Q4, LSR %3\n"

    
    
    

"2:\n"
    
    
    
    
    
    
    
"   subs    %3, %1, #31\n"          
"   mov     %1, %R4, LSL #11\n"     
"   bmi     3f\n"

    
    
"   bic     %2, %3, #0xff\n"
"   orr     %3, %3, %2, LSR #3\n"
    
"   mov     %2, %1, LSL %3\n"
"   b       4f\n"
"3:\n" 
    
    
"   rsb     %3, %3, #0\n"          
"   mov     %2, %1, LSR %3\n"      

    
    
    

"4:\n"
    
"   orr     %Q4, %Q4, %2\n"
    
    
    
"   eor     %Q4, %Q4, %R4, ASR #31\n"
"   add     %0, %Q4, %R4, LSR #31\n"
"   b       9f\n"
"8:\n"
    
    
"   mov     %0, #0\n"
"9:\n"
    : "=r" (i), "=&r" (tmp0), "=&r" (tmp1), "=&r" (tmp2)
    : "r" (d)
    : "cc"
        );
    return i;
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
js_num_valueOf(JSContext *cx, uintN argc, js::Value *vp);

namespace js {

static JS_ALWAYS_INLINE bool
ValueFitsInInt32(const Value &v, int32_t *pi)
{
    if (v.isInt32()) {
        *pi = v.toInt32();
        return true;
    }
    return v.isDouble() && JSDOUBLE_IS_INT32(v.toDouble(), pi);
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
static JS_ALWAYS_INLINE bool
StringToNumberType(JSContext *cx, JSString *str, T *result)
{
    size_t length = str->length();
    const jschar *chars = str->getChars(NULL);
    if (!chars)
        return false;

    if (length == 1) {
        jschar c = chars[0];
        if ('0' <= c && c <= '9') {
            *result = NumberTraits<T>::toSelfType(T(c - '0'));
            return true;
        }
        if (JS_ISSPACE(c)) {
            *result = NumberTraits<T>::toSelfType(T(0));
            return true;
        }
        *result = NumberTraits<T>::NaN();
        return true;
    }

    const jschar *bp = chars;
    const jschar *end = chars + length;
    bp = js_SkipWhiteSpace(bp, end);

    
    if (end - bp >= 2 && bp[0] == '0' && (bp[1] == 'x' || bp[1] == 'X')) {
        
        const jschar *endptr;
        double d;
        if (!GetPrefixInteger(cx, bp + 2, end, 16, &endptr, &d) ||
            js_SkipWhiteSpace(endptr, end) != end) {
            *result = NumberTraits<T>::NaN();
            return true;
        }
        *result = NumberTraits<T>::toSelfType(d);
        return true;
    }

    






    const jschar *ep;
    double d;
    if (!js_strtod(cx, bp, end, &ep, &d) || js_SkipWhiteSpace(ep, end) != end) {
        *result = NumberTraits<T>::NaN();
        return true;
    }
    *result = NumberTraits<T>::toSelfType(d);
    return true;
}


static inline bool
ToInteger(JSContext *cx, const js::Value &v, jsdouble *dp)
{
    if (v.isInt32()) {
        *dp = v.toInt32();
        return true;
    }
    if (v.isDouble()) {
        *dp = v.toDouble();
    } else {
        extern bool ValueToNumberSlow(JSContext *cx, js::Value v, double *dp);
        if (!ValueToNumberSlow(cx, v, dp))
            return false;
    }
    *dp = js_DoubleToInteger(*dp);
    return true;
}

} 

#endif 
