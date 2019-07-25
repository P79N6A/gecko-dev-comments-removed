









































#include "jstypes.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsinfer.h"
#include "jsversion.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"

#include "vm/GlobalObject.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

#include "vm/BooleanObject-inl.h"
#include "vm/MethodGuard-inl.h"

using namespace js;
using namespace js::types;

Class js::BooleanClass = {
    "Boolean",
    JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_HAS_CACHED_PROTO(JSProto_Boolean),    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

#if JS_HAS_TOSOURCE
static JSBool
bool_toSource(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool b, ok;
    if (!BoxedPrimitiveMethodGuard(cx, args, bool_toSource, &b, &ok))
        return ok;

    StringBuffer sb(cx);
    if (!sb.append("(new Boolean(") || !BooleanToStringBuffer(cx, b, sb) || !sb.append("))"))
        return false;

    JSString *str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}
#endif

static JSBool
bool_toString(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool b, ok;
    if (!BoxedPrimitiveMethodGuard<bool>(cx, args, bool_toString, &b, &ok))
        return ok;

    args.rval().setString(cx->runtime->atomState.booleanAtoms[b ? 1 : 0]);
    return true;
}

static JSBool
bool_valueOf(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool b, ok;
    if (!BoxedPrimitiveMethodGuard(cx, args, bool_valueOf, &b, &ok))
        return ok;

    args.rval().setBoolean(b);
    return true;
}

static JSFunctionSpec boolean_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  bool_toSource,  0, 0),
#endif
    JS_FN(js_toString_str,  bool_toString,  0, 0),
    JS_FN(js_valueOf_str,   bool_valueOf,   0, 0),
    JS_FS_END
};

static JSBool
Boolean(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool b = args.length() != 0 ? js_ValueToBoolean(args[0]) : false;

    if (IsConstructing(vp)) {
        JSObject *obj = BooleanObject::create(cx, b);
        args.rval().setObject(*obj);
    } else {
        args.rval().setBoolean(b);
    }
    return true;
}

JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = &obj->asGlobal();

    JSObject *booleanProto = global->createBlankPrototype(cx, &BooleanClass);
    if (!booleanProto)
        return NULL;
    booleanProto->setFixedSlot(BooleanObject::PRIMITIVE_VALUE_SLOT, BooleanValue(false));

    JSFunction *ctor = global->createConstructor(cx, Boolean, &BooleanClass,
                                                 CLASS_ATOM(cx, Boolean), 1);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, booleanProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, booleanProto, NULL, boolean_methods))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_Boolean, ctor, booleanProto))
        return NULL;

    return booleanProto;
}

JSString *
js_BooleanToString(JSContext *cx, JSBool b)
{
    return cx->runtime->atomState.booleanAtoms[b ? 1 : 0];
}


bool
js::BooleanToStringBuffer(JSContext *cx, JSBool b, StringBuffer &sb)
{
    return b ? sb.append("true") : sb.append("false");
}

namespace js {

bool
BooleanGetPrimitiveValueSlow(JSContext *cx, JSObject &obj, Value *vp)
{
    JS_ASSERT(ObjectClassIs(obj, ESClass_Boolean, cx));
    JS_ASSERT(obj.isProxy());

    





    InvokeArgsGuard ag;
    if (!cx->stack.pushInvokeArgs(cx, 0, &ag))
        return false;
    ag.calleev().setUndefined();
    ag.thisv().setObject(obj);
    if (!GetProxyHandler(&obj)->nativeCall(cx, &obj, &BooleanClass, bool_valueOf, ag))
        return false;
    *vp = ag.rval();
    return true;
}

}  

JSBool
js_ValueToBoolean(const Value &v)
{
    if (v.isInt32())
        return v.toInt32() != 0;
    if (v.isString())
        return v.toString()->length() != 0;
    if (v.isObject())
        return JS_TRUE;
    if (v.isNullOrUndefined())
        return JS_FALSE;
    if (v.isDouble()) {
        jsdouble d;

        d = v.toDouble();
        return !JSDOUBLE_IS_NaN(d) && d != 0;
    }
    JS_ASSERT(v.isBoolean());
    return v.toBoolean();
}
