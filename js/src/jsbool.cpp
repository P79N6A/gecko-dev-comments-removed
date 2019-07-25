









































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

#include "jsinferinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::types;

Class js_BooleanClass = {
    "Boolean",
    JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Boolean),
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub
};

#if JS_HAS_TOSOURCE
#include "jsprf.h"

static JSBool
bool_toSource(JSContext *cx, uintN argc, Value *vp)
{
    bool b;
    if (!GetPrimitiveThis(cx, vp, &b))
        return false;

    char buf[32];
    JS_snprintf(buf, sizeof buf, "(new Boolean(%s))", JS_BOOLEAN_STR(b));
    JSString *str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    vp->setString(str);
    return true;
}
#endif

static JSBool
bool_toString(JSContext *cx, uintN argc, Value *vp)
{
    bool b;
    if (!GetPrimitiveThis(cx, vp, &b))
        return false;

    JSAtom *atom = cx->runtime->atomState.booleanAtoms[b ? 1 : 0];
    JSString *str = ATOM_TO_STRING(atom);
    if (!str)
        return JS_FALSE;
    vp->setString(str);
    return JS_TRUE;
}

static JSBool
bool_valueOf(JSContext *cx, uintN argc, Value *vp)
{
    bool b;
    if (!GetPrimitiveThis(cx, vp, &b))
        return false;

    vp->setBoolean(b);
    return JS_TRUE;
}

static JSFunctionSpec boolean_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN_TYPE(js_toSource_str,  bool_toSource,  0, JSFUN_PRIMITIVE_THIS, JS_TypeHandlerString),
#endif
    JS_FN_TYPE(js_toString_str,  bool_toString,  0, JSFUN_PRIMITIVE_THIS, JS_TypeHandlerString),
    JS_FN_TYPE(js_valueOf_str,   bool_valueOf,   0, JSFUN_PRIMITIVE_THIS, JS_TypeHandlerBool),
    JS_FN_TYPE(js_toJSON_str,    bool_valueOf,   0, JSFUN_PRIMITIVE_THIS, JS_TypeHandlerBool),
    JS_FS_END
};

static JSBool
Boolean(JSContext *cx, uintN argc, Value *vp)
{
    Value *argv = vp + 2;
    bool b = argc != 0 ? js_ValueToBoolean(argv[0]) : false;

    if (IsConstructing(vp)) {
        TypeObject *objType = cx->getFixedTypeObject(TYPE_OBJECT_NEW_BOOLEAN);
        JSObject *obj = NewBuiltinClassInstance(cx, &js_BooleanClass, objType);
        if (!obj)
            return false;
        obj->setPrimitiveThis(BooleanValue(b));
        vp->setObject(*obj);
    } else {
        vp->setBoolean(b);
    }
    return true;
}

static void type_NewBoolean(JSContext *cx, JSTypeFunction *jsfun, JSTypeCallsite *jssite)
{
#ifdef JS_TYPE_INFERENCE
    TypeCallsite *site = Valueify(jssite);
    if (site->isNew) {
        TypeObject *object = cx->getFixedTypeObject(TYPE_OBJECT_NEW_BOOLEAN);
        site->returnTypes->addType(cx, (jstype) object);
    } else {
        JS_TypeHandlerBool(cx, jsfun, jssite);
    }
#endif
}

JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto = js_InitClass(cx, obj, NULL, &js_BooleanClass, Boolean, 1, type_NewBoolean,
                                   NULL, boolean_methods, NULL, NULL);
    if (!proto)
        return NULL;
    proto->setPrimitiveThis(BooleanValue(false));
    return proto;
}

JSString *
js_BooleanToString(JSContext *cx, JSBool b)
{
    return ATOM_TO_STRING(cx->runtime->atomState.booleanAtoms[b ? 1 : 0]);
}


JSBool
js_BooleanToCharBuffer(JSContext *cx, JSBool b, JSCharBuffer &cb)
{
    return b ? js_AppendLiteral(cb, "true") : js_AppendLiteral(cb, "false");
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
