








































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
#include "jsobjinlines.h"
#include "jsscopeinlines.h"

using namespace avmplus;
using namespace nanojit;
using namespace js;

JS_FRIEND_API(void)
js_SetTraceableNativeFailed(JSContext *cx)
{
    SetBuiltinError(cx);
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

JSBool FASTCALL
js_TryUnboxInt32(jsval v, int32* i32p)
{
    if (JS_LIKELY(JSVAL_IS_INT(v))) {
        *i32p = JSVAL_TO_INT(v);
        return JS_TRUE;
    }
    if (!JSVAL_IS_DOUBLE(v))
        return JS_FALSE;
    int32 i;
    jsdouble d = *JSVAL_TO_DOUBLE(v);
    if (!JSDOUBLE_IS_INT(d, i))
        return JS_FALSE;
    *i32p = i;
    return JS_TRUE;
}
JS_DEFINE_CALLINFO_2(extern, BOOL, js_TryUnboxInt32, JSVAL, INT32PTR, 1, 1)

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

JSBool FASTCALL
js_AddProperty(JSContext* cx, JSObject* obj, JSScopeProperty* sprop)
{
    JS_LOCK_OBJ(cx, obj);

    uint32 slot = sprop->slot;
    JSScope* scope = OBJ_SCOPE(obj);
    if (slot != scope->freeslot)
        goto exit_trace;
    JS_ASSERT(sprop->parent == scope->lastProperty());

    if (scope->isSharedEmpty()) {
        scope = js_GetMutableScope(cx, obj);
        if (!scope)
            goto exit_trace;
    } else {
        JS_ASSERT(!scope->hasProperty(sprop));
    }

    if (!scope->table) {
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
        JSScopeProperty *sprop2 =
            scope->addProperty(cx, sprop->id, sprop->getter(), sprop->setter(),
                               SPROP_INVALID_SLOT, sprop->attrs, sprop->getFlags(),
                               sprop->shortid);
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
    if (!obj)
        return ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_OBJECT]);
    return ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[obj->typeOf(cx)]);
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

    JSObject* closure = js_NewGCObject(cx);
    if (!closure)
        return NULL;

    closure->initSharingEmptyScope(&js_FunctionClass, proto, parent,
                                   reinterpret_cast<jsval>(fun));
    return closure;
}
JS_DEFINE_CALLINFO_4(extern, OBJECT, js_NewNullClosure, CONTEXT, OBJECT, OBJECT, OBJECT, 0, 0)

JS_REQUIRES_STACK JSBool FASTCALL
js_PopInterpFrame(JSContext* cx, InterpState* state)
{
    JS_ASSERT(cx->fp && cx->fp->down);
    JSInlineFrame* ifp = (JSInlineFrame*)cx->fp;

    




    if (ifp->hookData)
        return JS_FALSE;
    if (cx->version != ifp->callerVersion)
        return JS_FALSE;
    if (cx->fp->flags & JSFRAME_CONSTRUCTING)
        return JS_FALSE;
    if (cx->fp->imacpc)
        return JS_FALSE;
    if (cx->fp->blockChain)
        return JS_FALSE;

    cx->fp->putActivationObjects(cx);
    
    
    if (cx->fp->script->staticLevel < JS_DISPLAY_SIZE)
        cx->display[cx->fp->script->staticLevel] = cx->fp->displaySave;

    
    cx->fp = cx->fp->down;
    JS_ASSERT(cx->fp->regs == &ifp->callerRegs);
    cx->fp->regs = ifp->frame.regs;

    JS_ARENA_RELEASE(&cx->stackPool, ifp->mark);

    
    *state->inlineCallCountp = *state->inlineCallCountp - 1;
    return JS_TRUE;
}
JS_DEFINE_CALLINFO_2(extern, BOOL, js_PopInterpFrame, CONTEXT, INTERPSTATE, 0, 0)

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
