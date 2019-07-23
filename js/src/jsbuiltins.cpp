






































#include <math.h>

#include "nanojit/avmplus.h"
#include "nanojit/nanojit.h"
#include "jstracer.h"
#include "jsapi.h"
#include "jsnum.h"
#include "jsgc.h"
#include "jscntxt.h"

FASTCALL jsdouble builtin_dmod(jsdouble a, jsdouble b)
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





FASTCALL jsval builtin_BoxDouble(JSContext* cx, jsdouble d)
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

FASTCALL jsval builtin_BoxInt32(JSContext* cx, jsint i)
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

FASTCALL jsdouble builtin_UnboxDouble(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return (jsdouble)JSVAL_TO_INT(v);
    return *JSVAL_TO_DOUBLE(v);
}

FASTCALL jsint builtin_UnboxInt32(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return JSVAL_TO_INT(v);
    jsint i;
    if (JSVAL_IS_DOUBLE(v) && JSDOUBLE_IS_INT(*JSVAL_TO_DOUBLE(v), i))
        return i;
    return INT32_ERROR_COOKIE;
}

FASTCALL int32 builtin_doubleToInt32(jsdouble d)
{
    return js_DoubleToECMAInt32(d);
}

FASTCALL int32 builtin_doubleToUint32(jsdouble d)
{
    return js_DoubleToECMAUint32(d);
}
