





#ifndef jsnum_h___
#define jsnum_h___

#include "mozilla/FloatingPoint.h"

#include <math.h>

#include "jsobj.h"

#include "vm/NumericConversions.h"

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

} 

#endif 
