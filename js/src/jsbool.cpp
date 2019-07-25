









































#include "jstypes.h"
#include "jsstdint.h"
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
#include "jsvector.h"

#include "vm/GlobalObject.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

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
#include "jsprf.h"

static JSBool
bool_toSource(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool b, ok;
    if (!BoxedPrimitiveMethodGuard(cx, args, bool_toSource, &b, &ok))
        return ok;

    char buf[32];
    JS_snprintf(buf, sizeof buf, "(new Boolean(%s))", JS_BOOLEAN_STR(b));
    JSString *str = JS_NewStringCopyZ(cx, buf);
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
    Value *argv = vp + 2;
    bool b = argc != 0 ? js_ValueToBoolean(argv[0]) : false;

    if (IsConstructing(vp)) {
        JSObject *obj = NewBuiltinClassInstance(cx, &BooleanClass);
        if (!obj)
            return false;
        obj->setPrimitiveThis(BooleanValue(b));
        vp->setObject(*obj);
    } else {
        vp->setBoolean(b);
    }
    return true;
}

JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = obj->asGlobal();

    JSObject *booleanProto = global->createBlankPrototype(cx, &BooleanClass);
    if (!booleanProto)
        return NULL;
    booleanProto->setPrimitiveThis(BooleanValue(false));

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

    





    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, 0, &args))
        return false;
    args.calleev().setUndefined();
    args.thisv().setObject(obj);
    if (!GetProxyHandler(&obj)->nativeCall(cx, &obj, &BooleanClass, bool_valueOf, args))
        return false;
    *vp = args.rval();
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
