






































#ifndef jsnum_h___
#define jsnum_h___

#include <math.h>

#include "jsobj.h"















#if defined(__arm) || defined(__arm32__) || defined(__arm26__) || defined(__arm__)
#if !defined(__VFP_FP__)
#define FPU_IS_ARM_FPA
#endif
#endif


#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff
#define JSDOUBLE_HI32_NAN       0x7ff80000
#define JSDOUBLE_LO32_NAN       0x00000000

#define JSDOUBLE_HI32_EXPSHIFT  20
#define JSDOUBLE_EXPBIAS        1023

union jsdpun {
    struct {
#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
        uint32_t lo, hi;
#else
        uint32_t hi, lo;
#endif
    } s;
    uint64_t u64;
    double d;
};

static inline int
JSDOUBLE_IS_NaN(double d)
{
    jsdpun u;
    u.d = d;
    return (u.u64 & JSDOUBLE_EXPMASK) == JSDOUBLE_EXPMASK &&
           (u.u64 & JSDOUBLE_MANTMASK) != 0;
}

static inline int
JSDOUBLE_IS_FINITE(double d)
{
    
    jsdpun u;
    u.d = d;
    return (u.u64 & JSDOUBLE_EXPMASK) != JSDOUBLE_EXPMASK;
}

static inline int
JSDOUBLE_IS_INFINITE(double d)
{
    jsdpun u;
    u.d = d;
    return (u.u64 & ~JSDOUBLE_SIGNBIT) == JSDOUBLE_EXPMASK;
}

static inline bool
JSDOUBLE_IS_NEG(double d)
{
    jsdpun u;
    u.d = d;
    return (u.s.hi & JSDOUBLE_HI32_SIGNBIT) != 0;
}

static inline uint32_t
JS_HASH_DOUBLE(double d)
{
    jsdpun u;
    u.d = d;
    return u.s.lo ^ u.s.hi;
}

extern double js_NaN;
extern double js_PositiveInfinity;
extern double js_NegativeInfinity;

namespace js {

extern bool
InitRuntimeNumberState(JSRuntime *rt);

extern void
FinishRuntimeNumberState(JSRuntime *rt);

} 


extern JSObject *
js_InitNumberClass(JSContext *cx, JSObject *obj);




extern const char js_isNaN_str[];
extern const char js_isFinite_str[];
extern const char js_parseFloat_str[];
extern const char js_parseInt_str[];

class JSString;
class JSFixedString;

extern JSString * JS_FASTCALL
js_IntToString(JSContext *cx, int i);






extern JSString * JS_FASTCALL
js_NumberToString(JSContext *cx, double d);

namespace js {





extern bool JS_FASTCALL
NumberValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);


extern JSFixedString *
NumberToString(JSContext *cx, double d);

extern JSFixedString *
IndexToString(JSContext *cx, uint32_t index);






struct ToCStringBuf
{
    




    static const size_t sbufSize = 34;
    char sbuf[sbufSize];
    char *dbuf;

    ToCStringBuf();
    ~ToCStringBuf();
};







extern char *
NumberToCString(JSContext *cx, ToCStringBuf *cbuf, double d, int base = 10);





const double DOUBLE_INTEGRAL_PRECISION_LIMIT = uint64_t(1) << 53;













extern bool
GetPrefixInteger(JSContext *cx, const jschar *start, const jschar *end, int base,
                 const jschar **endp, double *dp);


JS_ALWAYS_INLINE bool
ToNumber(JSContext *cx, const Value &v, double *out)
{
    if (v.isNumber()) {
        *out = v.toNumber();
        return true;
    }
    extern bool ToNumberSlow(JSContext *cx, js::Value v, double *dp);
    return ToNumberSlow(cx, v, out);
}


JS_ALWAYS_INLINE bool
ToNumber(JSContext *cx, Value *vp)
{
    if (vp->isNumber())
        return true;
    double d;
    extern bool ToNumberSlow(JSContext *cx, js::Value v, double *dp);
    if (!ToNumberSlow(cx, *vp, &d))
        return false;
    vp->setNumber(d);
    return true;
}






JS_ALWAYS_INLINE bool
ToInt32(JSContext *cx, const js::Value &v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    extern bool ToInt32Slow(JSContext *cx, const js::Value &v, int32_t *ip);
    return ToInt32Slow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ToUint32(JSContext *cx, const js::Value &v, uint32_t *out)
{
    if (v.isInt32()) {
        *out = (uint32_t)v.toInt32();
        return true;
    }
    extern bool ToUint32Slow(JSContext *cx, const js::Value &v, uint32_t *ip);
    return ToUint32Slow(cx, v, out);
}






JS_ALWAYS_INLINE bool
NonstandardToInt32(JSContext *cx, const js::Value &v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    extern bool NonstandardToInt32Slow(JSContext *cx, const js::Value &v, int32_t *ip);
    return NonstandardToInt32Slow(cx, v, out);
}






JS_ALWAYS_INLINE bool
ValueToUint16(JSContext *cx, const js::Value &v, uint16_t *out)
{
    if (v.isInt32()) {
        *out = uint16_t(v.toInt32());
        return true;
    }
    extern bool ValueToUint16Slow(JSContext *cx, const js::Value &v, uint16_t *out);
    return ValueToUint16Slow(cx, v, out);
}

JSBool
num_parseInt(JSContext *cx, unsigned argc, Value *vp);

}  














static inline int32_t
js_DoubleToECMAInt32(double d)
{
#if defined(__i386__) || defined(__i386) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
    jsdpun du, duh, two32;
    uint32_t di_h, u_tmp, expon, shift_amount;
    int32_t mask32;

    











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

    return int32_t(du.d);
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
    int32_t i;
    double two32, two31;

    if (!JSDOUBLE_IS_FINITE(d))
        return 0;

    i = (int32_t) d;
    if ((double) i == d)
        return i;

    two32 = 4294967296.0;
    two31 = 2147483648.0;
    d = fmod(d, two32);
    d = (d >= 0) ? floor(d) : ceil(d) + two32;
    return (int32_t) (d >= two31 ? d - two32 : d);
#endif
}

inline uint32_t
js_DoubleToECMAUint32(double d)
{
    return uint32_t(js_DoubleToECMAInt32(d));
}





static inline double
js_DoubleToInteger(double d)
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
          const jschar **ep, double *dp);

extern JSBool
js_num_valueOf(JSContext *cx, unsigned argc, js::Value *vp);

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










static JS_ALWAYS_INLINE bool
IsDefinitelyIndex(const Value &v, uint32_t *indexp)
{
    if (v.isInt32() && v.toInt32() >= 0) {
        *indexp = v.toInt32();
        return true;
    }

    int32_t i;
    if (v.isDouble() && JSDOUBLE_IS_INT32(v.toDouble(), &i) && i >= 0) {
        *indexp = uint32_t(i);
        return true;
    }

    return false;
}


static inline bool
ToInteger(JSContext *cx, const js::Value &v, double *dp)
{
    if (v.isInt32()) {
        *dp = v.toInt32();
        return true;
    }
    if (v.isDouble()) {
        *dp = v.toDouble();
    } else {
        extern bool ToNumberSlow(JSContext *cx, Value v, double *dp);
        if (!ToNumberSlow(cx, v, dp))
            return false;
    }
    *dp = js_DoubleToInteger(*dp);
    return true;
}

} 

#endif 
