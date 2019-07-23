








































#include <math.h>

#include "jsapi.h"
#include "jsarray.h"
#include "jsbool.h"
#include "jsnum.h"
#include "jsgc.h"
#include "jscntxt.h"
#include "nanojit/avmplus.h"
#include "nanojit/nanojit.h"
#include "jsmath.h"
#include "jsstr.h"
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
    return js_DoubleToECMAInt32(*JSVAL_TO_DOUBLE(v));
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

bool FASTCALL builtin_Array_dense_setelem(JSContext *cx, JSObject *obj, jsint i, jsval v)
{
    JS_ASSERT(OBJ_IS_DENSE_ARRAY(cx, obj));

    jsuint length = ARRAY_DENSE_LENGTH(obj);
    if ((jsuint)i < length) {
        if (obj->dslots[i] == JSVAL_HOLE) {
            if (i >= obj->fslots[JSSLOT_ARRAY_LENGTH])
                obj->fslots[JSSLOT_ARRAY_LENGTH] = i + 1;
            obj->fslots[JSSLOT_ARRAY_COUNT]++;
        }
        obj->dslots[i] = v;
        return true;
    }
    return OBJ_SET_PROPERTY(cx, obj, INT_TO_JSID(i), &v) ? true : false;
}

JSString* FASTCALL
builtin_String_p_substring(JSContext *cx, JSString *str, jsint begin, jsint end)
{
    JS_ASSERT(end >= begin);
    return js_NewDependentString(cx, str, (size_t)begin, (size_t)(end - begin));
}

JSString* FASTCALL
builtin_String_p_substring_1(JSContext *cx, JSString *str, jsint begin)
{
    jsint end = JSSTRING_LENGTH(str);
    JS_ASSERT(end >= begin);
    return js_NewDependentString(cx, str, (size_t)begin, (size_t)(end - begin));
}

JSString* FASTCALL
builtin_ConcatStrings(JSContext* cx, JSString* left, JSString* right)
{
    
    return js_ConcatStrings(cx, left, right);
}

JSString* FASTCALL
builtin_String_getelem(JSContext* cx, JSString* str, jsint i)
{
    if ((size_t)i >= JSSTRING_LENGTH(str))
        return NULL;
    
    str = js_GetUnitString(cx, str, (size_t)i);
    return str;
}

JSString* FASTCALL
builtin_String_fromCharCode(JSContext* cx, jsint i)
{
    jschar c = (jschar)i;
    
    
    
    return js_NewStringCopyN(cx, &c, 1);
}

jsint FASTCALL
builtin_String_p_charCodeAt(JSString* str, jsint i)
{
    if (i < 0 || (jsint)JSSTRING_LENGTH(str) <= i)
        return -1;
    return JSSTRING_CHARS(str)[i];
}

jsdouble FASTCALL
builtin_Math_random(JSRuntime* rt)
{
    JS_LOCK_RUNTIME(rt);
    js_random_init(rt);
    jsdouble z = js_random_nextDouble(rt);
    JS_UNLOCK_RUNTIME(rt);
    return z;
}

bool FASTCALL
builtin_EqualStrings(JSString* str1, JSString* str2)
{
    return js_EqualStrings(str1, str2);
}

jsdouble FASTCALL
builtin_StringToNumber(JSContext* cx, JSString *str)
{
    const jschar* bp;
    const jschar* end;
    const jschar* ep;
    jsdouble d;

    JSSTRING_CHARS_AND_END(str, bp, end);
    if ((!js_strtod(cx, bp, end, &ep, &d) ||
         js_SkipWhiteSpace(ep, end) != end) &&
        (!js_strtointeger(cx, bp, end, &ep, 0, &d) ||
         js_SkipWhiteSpace(ep, end) != end)) {
        return *cx->runtime->jsNaN;
    }
    return d;
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
#define BUILTIN4(op, at0, at1, at2, at3, atr, tr, t0, t1, t2, t3, cse, fold)    \
    { (intptr_t)&builtin_##op, (at0 << 8) | (at1 << 6) | (at2 << 4) | (at3 << 2) | atr, cse, fold NAME(op) },

struct CallInfo builtins[] = {
#include "builtins.tbl"
};
