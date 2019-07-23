






































#ifndef jsnum_h___
#define jsnum_h___

#include <math.h>
#if defined(XP_WIN) || defined(XP_OS2)
#include <float.h>
#endif
#ifdef SOLARIS
#include <ieeefp.h>
#endif
#ifdef HAVE_SSE2
#ifdef __GNUC__
#include <xmmintrin.h>
#else
#include <emmintrin.h>
#endif
#endif










JS_BEGIN_EXTERN_C






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


#if defined(HAVE_SSE2) && defined (_MSC_VER) && (_MSC_VER < 1500)
static inline __m128i
_mm_castpd_si128(__m128d v) {
    return *(__m128i *)&v;
}
#endif

static inline bool
JSDOUBLE_IS_INT(jsdouble d, jsint& i)
{
#ifdef HAVE_SSE2
    if (js_use_SSE2) {
        __m128d xd = _mm_set_sd(d);      
        int ii = _mm_cvtsd_si32(xd);     
        __m128d xdi = _mm_setzero_pd();  
        xdi = _mm_cvtsi32_sd(xdi, ii);   
        __m128i xcmp = _mm_cmpeq_epi32(_mm_castpd_si128(xd),   
                                       _mm_castpd_si128(xdi)); 
        int m = _mm_movemask_epi8(xcmp); 
        if ((m & 0xff) != 0xff)          
            return false;
        i = ii;
        return true;
    }
#endif
    if (JSDOUBLE_IS_NEGZERO(d))
        return false;
    return d == (i = jsint(d));
}

static inline int
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


extern JSClass js_NumberClass;

extern JSObject *
js_InitNumberClass(JSContext *cx, JSObject *obj);




extern const char js_Infinity_str[];
extern const char js_NaN_str[];
extern const char js_isNaN_str[];
extern const char js_isFinite_str[];
extern const char js_parseFloat_str[];
extern const char js_parseInt_str[];




extern JSBool
js_NewNumberInRootedValue(JSContext *cx, jsdouble d, jsval *vp);





extern JSBool
js_NewWeaklyRootedNumber(JSContext *cx, jsdouble d, jsval *vp);


extern JSString * JS_FASTCALL
js_NumberToString(JSContext *cx, jsdouble d);





extern JSBool JS_FASTCALL
js_NumberValueToCharBuffer(JSContext *cx, jsval v, JSCharBuffer &cb);








extern jsdouble
js_ValueToNumber(JSContext *cx, jsval* vp);









extern int32
js_ValueToECMAInt32(JSContext *cx, jsval *vp);

extern uint32
js_ValueToECMAUint32(JSContext *cx, jsval *vp);














static inline int32
js_DoubleToECMAInt32(jsdouble d)
{
#ifdef __i386__
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

extern uint32
js_DoubleToECMAUint32(jsdouble d);









extern int32
js_ValueToInt32(JSContext *cx, jsval *vp);






extern uint16
js_ValueToUint16(JSContext *cx, jsval *vp);





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

JS_END_EXTERN_C

#endif 
