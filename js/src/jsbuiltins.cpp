








































#include <math.h>

#include "jsapi.h"
#include "jsnum.h"
#include "jsgc.h"
#include "jscntxt.h"
#include "nanojit/avmplus.h"
#include "nanojit/nanojit.h"
#include "jstracer.h"

using namespace nanojit;

jsdouble FASTCALL builtin_dmod(jsdouble a, jsdouble b)
{
    if (b == 0.0) {
        jsdpun u;
        u.s.hi = JSDOUBLE_HI32_EXPMASK | JSDOUBLE_HI32_MANTMASK;
        u.s.lo = 0xffffffff;
        return u.d;
    }
    jsdouble r;
#ifdef XP_WIN
    
    if (!(JSDOUBLE_IS_FINITE(a) && JSDOUBLE_IS_INFINITE(b)))
        r = a;
    else
#endif
        r = fmod(a, b);
    return r;
}





jsval FASTCALL builtin_BoxDouble(JSContext* cx, jsdouble d)
{
    jsint i;
    if (JSDOUBLE_IS_INT(d, i))
        return INT_TO_JSVAL(i);
    if (!cx->doubleFreeList) 
        return JSVAL_ERROR_COOKIE;
    jsval v; 
    if (!js_NewDoubleInRootedValue(cx, d, &v))
        return JSVAL_ERROR_COOKIE;
    return v;
}

jsval FASTCALL builtin_BoxInt32(JSContext* cx, jsint i)
{
    if (JS_LIKELY(INT_FITS_IN_JSVAL(i)))
        return INT_TO_JSVAL(i);
    if (!cx->doubleFreeList) 
        return JSVAL_ERROR_COOKIE;
    jsval v; 
    jsdouble d = (jsdouble)i;
    if (!js_NewDoubleInRootedValue(cx, d, &v))
        return JSVAL_ERROR_COOKIE;
    return v;
} 

jsdouble FASTCALL builtin_UnboxDouble(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return (jsdouble)JSVAL_TO_INT(v);
    return *JSVAL_TO_DOUBLE(v);
}

jsint FASTCALL builtin_UnboxInt32(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return JSVAL_TO_INT(v);
    jsint i;
    if (JSVAL_IS_DOUBLE(v) && JSDOUBLE_IS_INT(*JSVAL_TO_DOUBLE(v), i))
        return i;
    return INT32_ERROR_COOKIE;
}

int32 FASTCALL builtin_doubleToInt32(jsdouble d)
{
    return js_DoubleToECMAInt32(d);
}

int32 FASTCALL builtin_doubleToUint32(jsdouble d)
{
    return js_DoubleToECMAUint32(d);
}

jsdouble FASTCALL builtin_Math_sin(jsdouble d)
{
    return sin(d);
}

jsdouble FASTCALL builtin_Math_cos(jsdouble d)
{
    return cos(d);
}

jsdouble FASTCALL builtin_Math_pow(jsdouble d, jsdouble p)
{
#ifdef NOTYET
    
    if (!JSDOUBLE_IS_FINITE(p) && (d == 1.0 || d == -1.0))
        return NaN;
#endif
    if (p == 0)
        return 1.0;
    return pow(d, p);
}

jsdouble FASTCALL builtin_Math_sqrt(jsdouble d)
{
    return sqrt(d);
}

JSObject* FASTCALL builtin_get_this(JSContext* cx)
{
    JSObject *obj;
    JS_COMPUTE_THIS(cx, cx->fp, obj);
    return obj;
  error:
    return NULL; 
}

#define LO ARGSIZE_LO
#define F  ARGSIZE_F
#define Q  ARGSIZE_Q

#ifdef DEBUG
#define NAME(op) ,#op
#else
#define NAME(op)
#endif

#define BUILTIN1(op, at0, atr, tr, t0, cse, fold) \
    { (intptr_t)&builtin_##op, (at0 << 2) | atr, cse, fold NAME(op) },
#define BUILTIN2(op, at0, at1, atr, tr, t0, t1, cse, fold) \
    { (intptr_t)&builtin_##op, (at0 << 4) | (at1 << 2) | atr, cse, fold NAME(op) },
#define BUILTIN3(op, at0, at1, at2, atr, tr, t0, t1, t2, cse, fold) \
    { (intptr_t)&builtin_##op, (at0 << 6) | (at1 << 4) | (at2 << 2) | atr, cse, fold NAME(op) },

struct CallInfo builtins[] = {
#include "builtins.tbl"
};
