








































#include <math.h>

#include "jsapi.h"
#include "jsstdint.h"
#include "jsarray.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jslibmath.h"
#include "jsmath.h"
#include "jsnum.h"
#include "prmjtime.h"
#include "jsdate.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jsbuiltins.h"
#include "jstracer.h"

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
    jsdouble r;
#ifdef XP_WIN
    
    if (JSDOUBLE_IS_FINITE(a) && JSDOUBLE_IS_INFINITE(b))
        r = a;
    else
#endif
        r = fmod(a, b);
    return r;
}

int32 FASTCALL
js_imod(int32 a, int32 b)
{
    if (a < 0 || b <= 0)
        return -1;
    int r = a % b;
    return r;
}





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

jsdouble FASTCALL
js_UnboxDouble(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return (jsdouble)JSVAL_TO_INT(v);
    return *JSVAL_TO_DOUBLE(v);
}

int32 FASTCALL
js_UnboxInt32(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return JSVAL_TO_INT(v);
    return js_DoubleToECMAInt32(*JSVAL_TO_DOUBLE(v));
}

int32 FASTCALL
js_DoubleToInt32(jsdouble d)
{
    return js_DoubleToECMAInt32(d);
}

uint32 FASTCALL
js_DoubleToUint32(jsdouble d)
{
    return js_DoubleToECMAUint32(d);
}

jsdouble FASTCALL
js_StringToNumber(JSContext* cx, JSString* str)
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
        return js_NaN;
    }
    return d;
}

int32 FASTCALL
js_StringToInt32(JSContext* cx, JSString* str)
{
    const jschar* bp;
    const jschar* end;
    const jschar* ep;
    jsdouble d;

    JSSTRING_CHARS_AND_END(str, bp, end);
    if (!js_strtod(cx, bp, end, &ep, &d) || js_SkipWhiteSpace(ep, end) != end)
        return 0;
    return js_DoubleToECMAInt32(d);
}

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

JSBool FASTCALL
js_AddProperty(JSContext* cx, JSObject* obj, JSScopeProperty* sprop)
{
    JSScopeProperty* sprop2 = NULL; 

    JS_ASSERT(OBJ_IS_NATIVE(obj));
    JS_ASSERT(SPROP_HAS_STUB_SETTER(sprop));

    JS_LOCK_OBJ(cx, obj);
    JSScope* scope = OBJ_SCOPE(obj);
    if (scope->object == obj) {
        JS_ASSERT(!SCOPE_HAS_PROPERTY(scope, sprop));
    } else {
        scope = js_GetMutableScope(cx, obj);
        if (!scope) {
            JS_UNLOCK_OBJ(cx, obj);
            return JS_FALSE;
        }
    }

    uint32 slot = sprop->slot;
    if (!scope->table && sprop->parent == scope->lastProp && slot == scope->map.freeslot) {
        if (slot < STOBJ_NSLOTS(obj) && !OBJ_GET_CLASS(cx, obj)->reserveSlots) {
            JS_ASSERT(JSVAL_IS_VOID(STOBJ_GET_SLOT(obj, scope->map.freeslot)));
            ++scope->map.freeslot;
        } else {
            if (!js_AllocSlot(cx, obj, &slot)) {
                JS_UNLOCK_SCOPE(cx, scope);
                return JS_FALSE;
            }

            if (slot != sprop->slot) {
                js_FreeSlot(cx, obj, slot);
                goto slot_changed;
            }
        }

        SCOPE_EXTEND_SHAPE(cx, scope, sprop);
        ++scope->entryCount;
        scope->lastProp = sprop;
        JS_UNLOCK_SCOPE(cx, scope);
        return JS_TRUE;
    }

    sprop2 = js_AddScopeProperty(cx, scope, sprop->id,
                                 sprop->getter, sprop->setter, SPROP_INVALID_SLOT,
                                 sprop->attrs, sprop->flags, sprop->shortid,
                                 NULL);
    if (sprop2 == sprop) {
        JS_UNLOCK_SCOPE(cx, scope);
        return JS_TRUE;
    }
    slot = sprop2->slot;

  slot_changed:
    JS_UNLOCK_SCOPE(cx, scope);
    return JS_FALSE;
}

static JSBool
HasProperty(JSContext* cx, JSObject* obj, jsid id)
{
    
    for (JSObject* pobj = obj; pobj; pobj = OBJ_GET_PROTO(cx, pobj)) {
        if (pobj->map->ops->lookupProperty != js_LookupProperty)
            return JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID);
        JSClass* clasp = OBJ_GET_CLASS(cx, pobj);
        if (clasp->resolve != JS_ResolveStub && clasp != &js_StringClass)
            return JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID);
    }

    JSObject* obj2;
    JSProperty* prop;
    if (!js_LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_QUALIFIED, &obj2, &prop))
        return JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID);
    if (prop)
        OBJ_DROP_PROPERTY(cx, obj2, prop);
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

JSBool FASTCALL
js_HasNamedPropertyInt32(JSContext* cx, JSObject* obj, int32 index)
{
    jsid id;
    if (!js_Int32ToId(cx, index, &id))
        return JSVAL_TO_BOOLEAN(JSVAL_VOID);

    return HasProperty(cx, obj, id);
}

jsval FASTCALL
js_CallGetter(JSContext* cx, JSObject* obj, JSScopeProperty* sprop)
{
    JS_ASSERT(!SPROP_HAS_STUB_GETTER(sprop));
    jsval v;
    if (!js_GetSprop(cx, sprop, obj, &v))
        return JSVAL_ERROR_COOKIE;
    return v;
}

JSString* FASTCALL
js_TypeOfObject(JSContext* cx, JSObject* obj)
{
    JSType type = JS_TypeOfValue(cx, OBJECT_TO_JSVAL(obj));
    return ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[type]);
}

JSString* FASTCALL
js_TypeOfBoolean(JSContext* cx, int32 unboxed)
{
    
    jsval boxed = PSEUDO_BOOLEAN_TO_JSVAL(unboxed);
    JS_ASSERT(JSVAL_IS_VOID(boxed) || JSVAL_IS_BOOLEAN(boxed));
    JSType type = JS_TypeOfValue(cx, boxed);
    return ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[type]);
}

jsdouble FASTCALL
js_BooleanOrUndefinedToNumber(JSContext* cx, int32 unboxed)
{
    if (unboxed == JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID))
        return js_NaN;
    JS_ASSERT(unboxed == JS_TRUE || unboxed == JS_FALSE);
    return unboxed;
}

JSString* FASTCALL
js_BooleanOrUndefinedToString(JSContext *cx, int32 unboxed)
{
    JS_ASSERT(uint32(unboxed) <= 2);
    return ATOM_TO_STRING(cx->runtime->atomState.booleanAtoms[unboxed]);
}

JSObject* FASTCALL
js_Arguments(JSContext* cx)
{
    return NULL;
}

JSObject* FASTCALL
js_NewNullClosure(JSContext* cx, JSObject* funobj, JSObject* proto, JSObject *parent)
{
    JS_ASSERT(HAS_FUNCTION_CLASS(funobj));

    JSFunction *fun = (JSFunction*) funobj;
    JS_ASSERT(GET_FUNCTION_PRIVATE(cx, funobj) == fun);

    JS_ASSERT(JS_ON_TRACE(cx));
    JSObject* closure = (JSObject*) js_NewGCThing(cx, GCX_OBJECT, sizeof(JSObject));
    if (!closure)
        return NULL;

    closure->classword = jsuword(&js_FunctionClass);
    closure->fslots[JSSLOT_PROTO] = OBJECT_TO_JSVAL(proto);
    closure->fslots[JSSLOT_PARENT] = OBJECT_TO_JSVAL(parent);
    closure->fslots[JSSLOT_PRIVATE] = PRIVATE_TO_JSVAL(fun);
    for (unsigned i = JSSLOT_PRIVATE + 1; i != JS_INITIAL_NSLOTS; ++i)
        closure->fslots[i] = JSVAL_VOID;

    closure->map = js_HoldObjectMap(cx, proto->map);
    closure->dslots = NULL;
    return closure;
}

#define BUILTIN1 JS_DEFINE_CALLINFO_1
#define BUILTIN2 JS_DEFINE_CALLINFO_2
#define BUILTIN3 JS_DEFINE_CALLINFO_3
#define BUILTIN4 JS_DEFINE_CALLINFO_4
#define BUILTIN5 JS_DEFINE_CALLINFO_5
#include "builtins.tbl"
