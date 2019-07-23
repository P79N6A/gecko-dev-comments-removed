








































#include <math.h>

#include "jsapi.h"
#include "jsstdint.h"
#include "jsarray.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jslibmath.h"
#include "jsmath.h"
#include "jsnum.h"
#include "prmjtime.h"
#include "jsdate.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jsbuiltins.h"
#include "jstracer.h"
#include "jsvector.h"

#include "jsatominlines.h"

using namespace avmplus;
using namespace nanojit;

extern jsdouble js_NaN;

JS_FRIEND_API(void)
js_SetTraceableNativeFailed(JSContext *cx)
{
    js_SetBuiltinError(cx);
}






jsdouble FASTCALL
js_dmod(jsdouble a, jsdouble b)
{
    if (b == 0.0) {
        jsdpun u;
        u.s.hi = JSDOUBLE_HI32_EXPMASK | JSDOUBLE_HI32_MANTMASK;
        u.s.lo = 0xffffffff;
        return u.d;
    }
    return js_fmod(a, b);
}
JS_DEFINE_CALLINFO_2(extern, DOUBLE, js_dmod, DOUBLE, DOUBLE, 1, 1)

int32 FASTCALL
js_imod(int32 a, int32 b)
{
    if (a < 0 || b <= 0)
        return -1;
    int r = a % b;
    return r;
}
JS_DEFINE_CALLINFO_2(extern, INT32, js_imod, INT32, INT32, 1, 1)





jsval FASTCALL
js_BoxDouble(JSContext* cx, jsdouble d)
{
    int32 i;
    if (JSDOUBLE_IS_INT(d, i) && INT_FITS_IN_JSVAL(i))
        return INT_TO_JSVAL(i);
    JS_ASSERT(JS_ON_TRACE(cx));
    jsval v; 
    if (!js_NewDoubleInRootedValue(cx, d, &v))
        return JSVAL_ERROR_COOKIE;
    return v;
}
JS_DEFINE_CALLINFO_2(extern, JSVAL, js_BoxDouble, CONTEXT, DOUBLE, 1, 1)

jsval FASTCALL
js_BoxInt32(JSContext* cx, int32 i)
{
    if (JS_LIKELY(INT_FITS_IN_JSVAL(i)))
        return INT_TO_JSVAL(i);
    JS_ASSERT(JS_ON_TRACE(cx));
    jsval v; 
    jsdouble d = (jsdouble)i;
    if (!js_NewDoubleInRootedValue(cx, d, &v))
        return JSVAL_ERROR_COOKIE;
    return v;
}
JS_DEFINE_CALLINFO_2(extern, JSVAL, js_BoxInt32, CONTEXT, INT32, 1, 1)

jsdouble FASTCALL
js_UnboxDouble(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return (jsdouble)JSVAL_TO_INT(v);
    return *JSVAL_TO_DOUBLE(v);
}
JS_DEFINE_CALLINFO_1(extern, DOUBLE, js_UnboxDouble, JSVAL, 1, 1)

int32 FASTCALL
js_UnboxInt32(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return JSVAL_TO_INT(v);
    return js_DoubleToECMAInt32(*JSVAL_TO_DOUBLE(v));
}
JS_DEFINE_CALLINFO_1(extern, INT32, js_UnboxInt32, JSVAL, 1, 1)

int32 FASTCALL
js_DoubleToInt32(jsdouble d)
{
    return js_DoubleToECMAInt32(d);
}
JS_DEFINE_CALLINFO_1(extern, INT32, js_DoubleToInt32, DOUBLE, 1, 1)

uint32 FASTCALL
js_DoubleToUint32(jsdouble d)
{
    return js_DoubleToECMAUint32(d);
}
JS_DEFINE_CALLINFO_1(extern, UINT32, js_DoubleToUint32, DOUBLE, 1, 1)

jsdouble FASTCALL
js_StringToNumber(JSContext* cx, JSString* str)
{
    const jschar* bp;
    const jschar* end;
    const jschar* ep;
    jsdouble d;

    str->getCharsAndEnd(bp, end);
    if ((!js_strtod(cx, bp, end, &ep, &d) ||
         js_SkipWhiteSpace(ep, end) != end) &&
        (!js_strtointeger(cx, bp, end, &ep, 0, &d) ||
         js_SkipWhiteSpace(ep, end) != end)) {
        return js_NaN;
    }
    return d;
}
JS_DEFINE_CALLINFO_2(extern, DOUBLE, js_StringToNumber, CONTEXT, STRING, 1, 1)

int32 FASTCALL
js_StringToInt32(JSContext* cx, JSString* str)
{
    const jschar* bp;
    const jschar* end;
    const jschar* ep;
    jsdouble d;
    
    if (str->length() == 1) {
        jschar c = str->chars()[0];
        if ('0' <= c && c <= '9')
            return c - '0';
        return 0;	
    }

    str->getCharsAndEnd(bp, end);
    if ((!js_strtod(cx, bp, end, &ep, &d) ||
         js_SkipWhiteSpace(ep, end) != end) &&
        (!js_strtointeger(cx, bp, end, &ep, 0, &d) ||
         js_SkipWhiteSpace(ep, end) != end)) {
        return 0;
    }
    return js_DoubleToECMAInt32(d);
}
JS_DEFINE_CALLINFO_2(extern, INT32, js_StringToInt32, CONTEXT, STRING, 1, 1)

SideExit* FASTCALL
js_CallTree(InterpState* state, Fragment* f)
{
    union { NIns *code; GuardRecord* (FASTCALL *func)(InterpState*, Fragment*); } u;

    u.code = f->code();
    JS_ASSERT(u.code);

    GuardRecord* rec;
#if defined(JS_NO_FASTCALL) && defined(NANOJIT_IA32)
    SIMULATE_FASTCALL(rec, state, NULL, u.func);
#else
    rec = u.func(state, NULL);
#endif
    VMSideExit* lr = (VMSideExit*)rec->exit;

    if (lr->exitType == NESTED_EXIT) {
        


        if (!state->lastTreeCallGuard) {
            state->lastTreeCallGuard = lr;
            FrameInfo** rp = (FrameInfo**)state->rp;
            state->rpAtLastTreeCall = rp + lr->calldepth;
        }
    } else {
        


        state->lastTreeExitGuard = lr;
    }

    return lr;
}
JS_DEFINE_CALLINFO_2(extern, SIDEEXIT, js_CallTree, INTERPSTATE, FRAGMENT, 0, 0)

JSBool FASTCALL
js_AddProperty(JSContext* cx, JSObject* obj, JSScopeProperty* sprop)
{
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    JS_LOCK_OBJ(cx, obj);

    JSScope* scope = OBJ_SCOPE(obj);
    uint32 slot;
    if (scope->owned()) {
        JS_ASSERT(!scope->has(sprop));
    } else {
        scope = js_GetMutableScope(cx, obj);
        if (!scope)
            goto exit_trace;
    }

    slot = sprop->slot;
    if (!scope->table && sprop->parent == scope->lastProp && slot == scope->freeslot) {
        if (slot < STOBJ_NSLOTS(obj) && !OBJ_GET_CLASS(cx, obj)->reserveSlots) {
            JS_ASSERT(JSVAL_IS_VOID(STOBJ_GET_SLOT(obj, scope->freeslot)));
            ++scope->freeslot;
        } else {
            if (!js_AllocSlot(cx, obj, &slot))
                goto exit_trace;

            if (slot != sprop->slot) {
                js_FreeSlot(cx, obj, slot);
                goto exit_trace;
            }
        }

        scope->extend(cx, sprop);
    } else {
        JSScopeProperty *sprop2 = scope->add(cx, sprop->id,
                                             sprop->getter, sprop->setter,
                                             SPROP_INVALID_SLOT, sprop->attrs,
                                             sprop->flags, sprop->shortid);
        if (sprop2 != sprop)
            goto exit_trace;
    }

    if (js_IsPropertyCacheDisabled(cx))
        goto exit_trace;

    JS_UNLOCK_SCOPE(cx, scope);
    return JS_TRUE;

  exit_trace:
    JS_UNLOCK_SCOPE(cx, scope);
    return JS_FALSE;
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_AddProperty, CONTEXT, OBJECT, SCOPEPROP, 0, 0)

static JSBool
HasProperty(JSContext* cx, JSObject* obj, jsid id)
{
    
    for (JSObject* pobj = obj; pobj; pobj = OBJ_GET_PROTO(cx, pobj)) {
        if (pobj->map->ops->lookupProperty != js_LookupProperty)
            return JSVAL_TO_SPECIAL(JSVAL_VOID);
        JSClass* clasp = OBJ_GET_CLASS(cx, pobj);
        if (clasp->resolve != JS_ResolveStub && clasp != &js_StringClass)
            return JSVAL_TO_SPECIAL(JSVAL_VOID);
    }

    JSObject* obj2;
    JSProperty* prop;
    if (js_LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_QUALIFIED, &obj2, &prop) < 0)
        return JSVAL_TO_SPECIAL(JSVAL_VOID);
    if (prop)
        obj2->dropProperty(cx, prop);
    return prop != NULL;
}

JSBool FASTCALL
js_HasNamedProperty(JSContext* cx, JSObject* obj, JSString* idstr)
{
    jsid id;
    if (!js_ValueToStringId(cx, STRING_TO_JSVAL(idstr), &id))
        return JSVAL_TO_BOOLEAN(JSVAL_VOID);

    return HasProperty(cx, obj, id);
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_HasNamedProperty, CONTEXT, OBJECT, STRING, 0, 0)

JSBool FASTCALL
js_HasNamedPropertyInt32(JSContext* cx, JSObject* obj, int32 index)
{
    jsid id;
    if (!js_Int32ToId(cx, index, &id))
        return JSVAL_TO_BOOLEAN(JSVAL_VOID);

    return HasProperty(cx, obj, id);
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_HasNamedPropertyInt32, CONTEXT, OBJECT, INT32, 0, 0)

JSString* FASTCALL
js_TypeOfObject(JSContext* cx, JSObject* obj)
{
    JSType type = JS_TypeOfValue(cx, OBJECT_TO_JSVAL(obj));
    return ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[type]);
}
JS_DEFINE_CALLINFO_2(extern, STRING, js_TypeOfObject, CONTEXT, OBJECT, 1, 1)

JSString* FASTCALL
js_TypeOfBoolean(JSContext* cx, int32 unboxed)
{
    
    jsval boxed = SPECIAL_TO_JSVAL(unboxed);
    JS_ASSERT(JSVAL_IS_VOID(boxed) || JSVAL_IS_BOOLEAN(boxed));
    JSType type = JS_TypeOfValue(cx, boxed);
    return ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[type]);
}
JS_DEFINE_CALLINFO_2(extern, STRING, js_TypeOfBoolean, CONTEXT, INT32, 1, 1)

jsdouble FASTCALL
js_BooleanOrUndefinedToNumber(JSContext* cx, int32 unboxed)
{
    if (unboxed == JSVAL_TO_SPECIAL(JSVAL_VOID))
        return js_NaN;
    JS_ASSERT(unboxed == JS_TRUE || unboxed == JS_FALSE);
    return unboxed;
}
JS_DEFINE_CALLINFO_2(extern, DOUBLE, js_BooleanOrUndefinedToNumber, CONTEXT, INT32, 1, 1)

JSString* FASTCALL
js_BooleanOrUndefinedToString(JSContext *cx, int32 unboxed)
{
    JS_ASSERT(uint32(unboxed) <= 2);
    return ATOM_TO_STRING(cx->runtime->atomState.booleanAtoms[unboxed]);
}
JS_DEFINE_CALLINFO_2(extern, STRING, js_BooleanOrUndefinedToString, CONTEXT, INT32, 1, 1)

JSObject* FASTCALL
js_NewNullClosure(JSContext* cx, JSObject* funobj, JSObject* proto, JSObject* parent)
{
    JS_ASSERT(HAS_FUNCTION_CLASS(funobj));
    JS_ASSERT(HAS_FUNCTION_CLASS(proto));
    JS_ASSERT(JS_ON_TRACE(cx));

    JSFunction *fun = (JSFunction*) funobj;
    JS_ASSERT(GET_FUNCTION_PRIVATE(cx, funobj) == fun);

    JSObject* closure = js_NewGCObject(cx, GCX_OBJECT);
    if (!closure)
        return NULL;

    JSScope *scope = OBJ_SCOPE(proto)->getEmptyScope(cx, &js_FunctionClass);
    if (!scope) {
        closure->map = NULL;
        return NULL;
    }

    closure->map = &scope->map;
    closure->init(&js_FunctionClass, proto, parent,
                  reinterpret_cast<jsval>(fun));
    return closure;
}
JS_DEFINE_CALLINFO_4(extern, OBJECT, js_NewNullClosure, CONTEXT, OBJECT, OBJECT, OBJECT, 0, 0)

JSString* FASTCALL
js_ConcatN(JSContext *cx, JSString **strArray, uint32 size)
{
    
    size_t numChar = 1;
    for (uint32 i = 0; i < size; ++i) {
        size_t before = numChar;
        numChar += strArray[i]->length();
        if (numChar < before)
            return NULL;
    }


    
    if (numChar & js::tl::MulOverflowMask<sizeof(jschar)>::result)
        return NULL;
    jschar *buf = (jschar *)cx->malloc(numChar * sizeof(jschar));
    if (!buf)
        return NULL;

    
    jschar *ptr = buf;
    for (uint32 i = 0; i < size; ++i) {
        const jschar *chars;
        size_t length;
        strArray[i]->getCharsAndLength(chars, length);
        js_strncpy(ptr, chars, length);
        ptr += length;
    }
    *ptr = '\0';

    
    JSString *str = js_NewString(cx, buf, numChar - 1);
    if (!str)
        cx->free(buf);
    return str;
}
JS_DEFINE_CALLINFO_3(extern, STRING, js_ConcatN, CONTEXT, STRINGPTR, UINT32, 0, 0)
