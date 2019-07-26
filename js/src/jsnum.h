





#ifndef jsnum_h___
#define jsnum_h___

#include "mozilla/FloatingPoint.h"

#include <math.h>

#include "jsobj.h"

#include "vm/NumericConversions.h"

extern double js_PositiveInfinity;
extern double js_NegativeInfinity;

namespace js {

extern bool
InitRuntimeNumberState(JSRuntime *rt);

#if !ENABLE_INTL_API
extern void
FinishRuntimeNumberState(JSRuntime *rt);
#endif

} 


extern JSObject *
js_InitNumberClass(JSContext *cx, js::HandleObject obj);




extern const char js_isNaN_str[];
extern const char js_isFinite_str[];
extern const char js_parseFloat_str[];
extern const char js_parseInt_str[];

class JSString;






template <js::AllowGC allowGC>
extern JSString *
js_NumberToString(JSContext *cx, double d);

namespace js {

template <AllowGC allowGC>
extern JSFlatString *
Int32ToString(JSContext *cx, int32_t i);





extern bool JS_FASTCALL
NumberValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);


extern JSFlatString *
NumberToString(JSContext *cx, double d);

extern JSFlatString *
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
ToNumber(JSContext *cx, Value *vp)
{
#ifdef DEBUG
    {
        SkipRoot skip(cx, vp);
        MaybeCheckStackRoots(cx);
    }
#endif

    if (vp->isNumber())
        return true;
    double d;
    extern bool ToNumberSlow(JSContext *cx, js::Value v, double *dp);
    if (!ToNumberSlow(cx, *vp, &d))
        return false;

    vp->setNumber(d);
    return true;
}

JSBool
num_parseInt(JSContext *cx, unsigned argc, Value *vp);

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
    return v.isDouble() && MOZ_DOUBLE_IS_INT32(v.toDouble(), pi);
}










static JS_ALWAYS_INLINE bool
IsDefinitelyIndex(const Value &v, uint32_t *indexp)
{
    if (v.isInt32() && v.toInt32() >= 0) {
        *indexp = v.toInt32();
        return true;
    }

    int32_t i;
    if (v.isDouble() && MOZ_DOUBLE_IS_INT32(v.toDouble(), &i) && i >= 0) {
        *indexp = uint32_t(i);
        return true;
    }

    return false;
}


static inline bool
ToInteger(JSContext *cx, const js::Value &v, double *dp)
{
#ifdef DEBUG
    {
        SkipRoot skip(cx, &v);
        MaybeCheckStackRoots(cx);
    }
#endif

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
    *dp = ToInteger(*dp);
    return true;
}

inline bool
SafeAdd(int32_t one, int32_t two, int32_t *res)
{
    *res = one + two;
    int64_t ores = (int64_t)one + (int64_t)two;
    return ores == (int64_t)*res;
}

inline bool
SafeSub(int32_t one, int32_t two, int32_t *res)
{
    *res = one - two;
    int64_t ores = (int64_t)one - (int64_t)two;
    return ores == (int64_t)*res;
}

inline bool
SafeMul(int32_t one, int32_t two, int32_t *res)
{
    *res = one * two;
    int64_t ores = (int64_t)one * (int64_t)two;
    return ores == (int64_t)*res;
}

} 

#endif 
