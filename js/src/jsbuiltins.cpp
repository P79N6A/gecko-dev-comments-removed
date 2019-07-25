








































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
#include "jscntxtinlines.h"

using namespace avmplus;
using namespace nanojit;
using namespace js;

JS_FRIEND_API(void)
js_SetTraceableNativeFailed(JSContext *cx)
{
    



    SetBuiltinError(JS_TRACE_MONITOR_FROM_CONTEXT(cx));
}






jsdouble FASTCALL
js_dmod(jsdouble a, jsdouble b)
{
    if (b == 0.0) {
        jsdpun u;
        u.s.hi = JSDOUBLE_HI32_NAN;
        u.s.lo = JSDOUBLE_LO32_NAN;
        return u.d;
    }
    return js_fmod(a, b);
}
JS_DEFINE_CALLINFO_2(extern, DOUBLE, js_dmod, DOUBLE, DOUBLE, 1, ACCSET_NONE)

int32 FASTCALL
js_imod(int32 a, int32 b)
{
    if (a < 0 || b <= 0)
        return -1;
    int r = a % b;
    return r;
}
JS_DEFINE_CALLINFO_2(extern, INT32, js_imod, INT32, INT32, 1, ACCSET_NONE)

#if JS_BITS_PER_WORD == 32

jsdouble FASTCALL
js_UnboxDouble(uint32 tag, uint32 payload)
{
    if (tag == JSVAL_TAG_INT32)
        return (double)(int32)payload;

    jsval_layout l;
    l.s.tag = (JSValueTag)tag;
    l.s.payload.u32 = payload;
    return l.asDouble;
}
JS_DEFINE_CALLINFO_2(extern, DOUBLE, js_UnboxDouble, UINT32, UINT32, 1, ACCSET_NONE)

int32 FASTCALL
js_UnboxInt32(uint32 tag, uint32 payload)
{
    if (tag == JSVAL_TAG_INT32)
        return (int32)payload;

    jsval_layout l;
    l.s.tag = (JSValueTag)tag;
    l.s.payload.u32 = payload;
    return js_DoubleToECMAInt32(l.asDouble);
}
JS_DEFINE_CALLINFO_2(extern, INT32, js_UnboxInt32, UINT32, UINT32, 1, ACCSET_NONE)

#elif JS_BITS_PER_WORD == 64

jsdouble FASTCALL
js_UnboxDouble(Value v)
{
    if (v.isInt32())
        return (jsdouble)v.toInt32();
    return v.toDouble();
}
JS_DEFINE_CALLINFO_1(extern, DOUBLE, js_UnboxDouble, JSVAL, 1, ACCSET_NONE)

int32 FASTCALL
js_UnboxInt32(Value v)
{
    if (v.isInt32())
        return v.toInt32();
    return js_DoubleToECMAInt32(v.toDouble());
}
JS_DEFINE_CALLINFO_1(extern, INT32, js_UnboxInt32, VALUE, 1, ACCSET_NONE)

#endif

int32 FASTCALL
js_DoubleToInt32(jsdouble d)
{
    return js_DoubleToECMAInt32(d);
}
JS_DEFINE_CALLINFO_1(extern, INT32, js_DoubleToInt32, DOUBLE, 1, ACCSET_NONE)

uint32 FASTCALL
js_DoubleToUint32(jsdouble d)
{
    return js_DoubleToECMAUint32(d);
}
JS_DEFINE_CALLINFO_1(extern, UINT32, js_DoubleToUint32, DOUBLE, 1, ACCSET_NONE)







jsdouble FASTCALL
js_StringToNumber(JSContext* cx, JSString* str, JSBool *ok)
{
    double out = 0;  
    *ok = StringToNumberType<jsdouble>(cx, str, &out);
    return out;
}
JS_DEFINE_CALLINFO_3(extern, DOUBLE, js_StringToNumber, CONTEXT, STRING, BOOLPTR,
                     0, ACCSET_STORE_ANY)

int32 FASTCALL
js_StringToInt32(JSContext* cx, JSString* str, JSBool *ok)
{
    int32 out = 0;  
    *ok = StringToNumberType<int32>(cx, str, &out);
    return out;
}
JS_DEFINE_CALLINFO_3(extern, INT32, js_StringToInt32, CONTEXT, STRING, BOOLPTR,
                     0, ACCSET_STORE_ANY)


static inline JSBool
AddPropertyHelper(JSContext* cx, JSObject* obj, Shape* shape, bool isDefinitelyAtom)
{
    JS_ASSERT(shape->previous() == obj->lastProperty());

    if (obj->nativeEmpty()) {
        if (!obj->ensureClassReservedSlotsForEmptyObject(cx))
            return false;
    }

    uint32 slot;
    slot = shape->slot;
    JS_ASSERT(slot == obj->slotSpan());

    if (slot < obj->numSlots()) {
        JS_ASSERT(obj->getSlot(slot).isUndefined());
    } else {
        if (!obj->allocSlot(cx, &slot))
            return false;
        JS_ASSERT(slot == shape->slot);
    }

    obj->extend(cx, shape, isDefinitelyAtom);
    return !js_IsPropertyCacheDisabled(cx);
}

JSBool FASTCALL
js_AddProperty(JSContext* cx, JSObject* obj, Shape* shape)
{
    return AddPropertyHelper(cx, obj, shape, false);
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_AddProperty, CONTEXT, OBJECT, SHAPE, 0, ACCSET_STORE_ANY)

JSBool FASTCALL
js_AddAtomProperty(JSContext* cx, JSObject* obj, Shape* shape)
{
    return AddPropertyHelper(cx, obj, shape, true);
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_AddAtomProperty, CONTEXT, OBJECT, SHAPE, 0, ACCSET_STORE_ANY)

static JSBool
HasProperty(JSContext* cx, JSObject* obj, jsid id)
{
    
    for (JSObject* pobj = obj; pobj; pobj = pobj->getProto()) {
        if (pobj->getOps()->lookupProperty)
            return JS_NEITHER;
        Class* clasp = pobj->getClass();
        if (clasp->resolve != JS_ResolveStub && clasp != &js_StringClass)
            return JS_NEITHER;
    }

    JSObject* obj2;
    JSProperty* prop;
    if (!LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_QUALIFIED, &obj2, &prop))
        return JS_NEITHER;
    return prop != NULL;
}

JSBool FASTCALL
js_HasNamedProperty(JSContext* cx, JSObject* obj, JSString* idstr)
{
    JSAtom *atom = js_AtomizeString(cx, idstr);
    if (!atom)
        return JS_NEITHER;

    return HasProperty(cx, obj, ATOM_TO_JSID(atom));
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_HasNamedProperty, CONTEXT, OBJECT, STRING,
                     0, ACCSET_STORE_ANY)

JSBool FASTCALL
js_HasNamedPropertyInt32(JSContext* cx, JSObject* obj, int32 index)
{
    jsid id;
    if (!js_Int32ToId(cx, index, &id))
        return JS_NEITHER;

    return HasProperty(cx, obj, id);
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_HasNamedPropertyInt32, CONTEXT, OBJECT, INT32,
                     0, ACCSET_STORE_ANY)

JSString* FASTCALL
js_TypeOfObject(JSContext* cx, JSObject* obj)
{
    JS_ASSERT(obj);
    return cx->runtime->atomState.typeAtoms[obj->typeOf(cx)];
}
JS_DEFINE_CALLINFO_2(extern, STRING, js_TypeOfObject, CONTEXT, OBJECT, 1, ACCSET_NONE)

JSString* FASTCALL
js_BooleanIntToString(JSContext *cx, int32 unboxed)
{
    JS_ASSERT(uint32(unboxed) <= 1);
    return cx->runtime->atomState.booleanAtoms[unboxed];
}
JS_DEFINE_CALLINFO_2(extern, STRING, js_BooleanIntToString, CONTEXT, INT32, 1, ACCSET_NONE)

JSObject* FASTCALL
js_NewNullClosure(JSContext* cx, JSObject* funobj, JSObject* proto, JSObject* parent)
{
    JS_ASSERT(funobj->isFunction());
    JS_ASSERT(proto->isFunction());
    JS_ASSERT(JS_ON_TRACE(cx));

    JSFunction *fun = (JSFunction*) funobj;
    JS_ASSERT(GET_FUNCTION_PRIVATE(cx, funobj) == fun);

    JSObject* closure = js_NewGCObject(cx, gc::FINALIZE_OBJECT2);
    if (!closure)
        return NULL;

    if (!closure->initSharingEmptyShape(cx, &js_FunctionClass, proto, parent,
                                        fun, gc::FINALIZE_OBJECT2)) {
        return NULL;
    }
    return closure;
}
JS_DEFINE_CALLINFO_4(extern, OBJECT, js_NewNullClosure, CONTEXT, OBJECT, OBJECT, OBJECT,
                     0, ACCSET_STORE_ANY)

