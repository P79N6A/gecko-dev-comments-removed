






































#ifndef jsnum_h___
#define jsnum_h___










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

#if (__GNUC__ == 2 && __GNUC_MINOR__ > 95) || __GNUC__ > 2





#define JSDOUBLE_HI32(x) (__extension__ ({ jsdpun u; u.d = (x); u.s.hi; }))
#define JSDOUBLE_LO32(x) (__extension__ ({ jsdpun u; u.d = (x); u.s.lo; }))
#define JSDOUBLE_SET_HI32(x, y) \
    (__extension__ ({ jsdpun u; u.d = (x); u.s.hi = (y); (x) = u.d; }))
#define JSDOUBLE_SET_LO32(x, y) \
    (__extension__ ({ jsdpun u; u.d = (x); u.s.lo = (y); (x) = u.d; }))

#else 






#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[1])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[0])
#else
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[0])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[1])
#endif

#define JSDOUBLE_SET_HI32(x, y) (JSDOUBLE_HI32(x)=(y))
#define JSDOUBLE_SET_LO32(x, y) (JSDOUBLE_LO32(x)=(y))

#endif 

#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff

#define JSDOUBLE_IS_NaN(x)                                                    \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) == JSDOUBLE_HI32_EXPMASK &&   \
     (JSDOUBLE_LO32(x) || (JSDOUBLE_HI32(x) & JSDOUBLE_HI32_MANTMASK)))

#define JSDOUBLE_IS_INFINITE(x)                                               \
    ((JSDOUBLE_HI32(x) & ~JSDOUBLE_HI32_SIGNBIT) == JSDOUBLE_HI32_EXPMASK &&  \
     !JSDOUBLE_LO32(x))

#define JSDOUBLE_IS_FINITE(x)                                                 \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) != JSDOUBLE_HI32_EXPMASK)

#define JSDOUBLE_IS_NEGZERO(d)  (JSDOUBLE_HI32(d) == JSDOUBLE_HI32_SIGNBIT && \
                                 JSDOUBLE_LO32(d) == 0)







#define JSDOUBLE_IS_INT(d, i) (JSDOUBLE_IS_FINITE(d)                          \
                               && !JSDOUBLE_IS_NEGZERO(d)                     \
                               && ((d) == (i = (jsint)(d))))

#if defined(XP_WIN)
#define JSDOUBLE_COMPARE(LVAL, OP, RVAL, IFNAN)                               \
    ((JSDOUBLE_IS_NaN(LVAL) || JSDOUBLE_IS_NaN(RVAL))                         \
     ? (IFNAN)                                                                \
     : (LVAL) OP (RVAL))
#else
#define JSDOUBLE_COMPARE(LVAL, OP, RVAL, IFNAN) ((LVAL) OP (RVAL))
#endif

extern jsdouble js_NaN;


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
js_NumberValueToCharBuffer(JSContext *cx, jsval v, JSCharVector &cb);








extern jsdouble
js_ValueToNumber(JSContext *cx, jsval* vp);









extern int32
js_ValueToECMAInt32(JSContext *cx, jsval *vp);

extern uint32
js_ValueToECMAUint32(JSContext *cx, jsval *vp);




extern int32
js_DoubleToECMAInt32(jsdouble d);

extern uint32
js_DoubleToECMAUint32(jsdouble d);









extern int32
js_ValueToInt32(JSContext *cx, jsval *vp);






extern uint16
js_ValueToUint16(JSContext *cx, jsval *vp);





extern jsdouble
js_DoubleToInteger(jsdouble d);











extern JSBool
js_strtod(JSContext *cx, const jschar *s, const jschar *send,
          const jschar **ep, jsdouble *dp);










extern JSBool
js_strtointeger(JSContext *cx, const jschar *s, const jschar *send,
                const jschar **ep, jsint radix, jsdouble *dp);

JS_END_EXTERN_C

#endif 
