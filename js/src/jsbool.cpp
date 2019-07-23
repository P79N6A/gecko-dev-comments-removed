









































#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"
#include "jsvector.h"


JS_STATIC_ASSERT(!(JSVAL_TRUE & JSVAL_HOLE_FLAG));
JS_STATIC_ASSERT(!(JSVAL_FALSE & JSVAL_HOLE_FLAG));
JS_STATIC_ASSERT(!(JSVAL_VOID & JSVAL_HOLE_FLAG));
JS_STATIC_ASSERT((JSVAL_HOLE & JSVAL_HOLE_FLAG));
JS_STATIC_ASSERT((JSVAL_HOLE & ~JSVAL_HOLE_FLAG) == JSVAL_VOID);
JS_STATIC_ASSERT(!(JSVAL_ARETURN & JSVAL_HOLE_FLAG));

JSClass js_BooleanClass = {
    "Boolean",
    JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_HAS_CACHED_PROTO(JSProto_Boolean),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

#if JS_HAS_TOSOURCE
#include "jsprf.h"

static JSBool
bool_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    jsval v;
    char buf[32];
    JSString *str;

    if (!js_GetPrimitiveThis(cx, vp, &js_BooleanClass, &v))
        return JS_FALSE;
    JS_ASSERT(JSVAL_IS_BOOLEAN(v));
    JS_snprintf(buf, sizeof buf, "(new %s(%s))",
                js_BooleanClass.name,
                JS_BOOLEAN_STR(JSVAL_TO_BOOLEAN(v)));
    str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}
#endif

static JSBool
bool_toString(JSContext *cx, uintN argc, jsval *vp)
{
    jsval v;
    JSAtom *atom;
    JSString *str;

    if (!js_GetPrimitiveThis(cx, vp, &js_BooleanClass, &v))
        return JS_FALSE;
    JS_ASSERT(JSVAL_IS_BOOLEAN(v));
    atom = cx->runtime->atomState.booleanAtoms[JSVAL_TO_BOOLEAN(v) ? 1 : 0];
    str = ATOM_TO_STRING(atom);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
bool_valueOf(JSContext *cx, uintN argc, jsval *vp)
{
    return js_GetPrimitiveThis(cx, vp, &js_BooleanClass, vp);
}

static JSFunctionSpec boolean_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  bool_toSource,  0, JSFUN_THISP_BOOLEAN),
#endif
    JS_FN(js_toString_str,  bool_toString,  0, JSFUN_THISP_BOOLEAN),
    JS_FN(js_valueOf_str,   bool_valueOf,   0, JSFUN_THISP_BOOLEAN),
    JS_FN(js_toJSON_str,    bool_valueOf,   0, JSFUN_THISP_BOOLEAN),
    JS_FS_END
};

static JSBool
Boolean(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval bval;

    bval = (argc != 0)
           ? BOOLEAN_TO_JSVAL(js_ValueToBoolean(argv[0]))
           : JSVAL_FALSE;
    if (!JS_IsConstructing(cx))
        *rval = bval;
    else
        obj->fslots[JSSLOT_PRIVATE] = bval;
    return true;
}

JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;

    proto = JS_InitClass(cx, obj, NULL, &js_BooleanClass, Boolean, 1,
                        NULL, boolean_methods, NULL, NULL);
    if (!proto)
        return NULL;
    proto->fslots[JSSLOT_PRIVATE] = JSVAL_FALSE;
    return proto;
}

JSString *
js_BooleanToString(JSContext *cx, JSBool b)
{
    return ATOM_TO_STRING(cx->runtime->atomState.booleanAtoms[b ? 1 : 0]);
}


JSBool
js_BooleanToCharBuffer(JSContext *cx, JSBool b, JSCharVector &buf)
{
    return b ? js_AppendLiteral(buf, "true") : js_AppendLiteral(buf, "false");
}

JSBool
js_ValueToBoolean(jsval v)
{
    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v))
        return JS_FALSE;
    if (JSVAL_IS_OBJECT(v))
        return JS_TRUE;
    if (JSVAL_IS_STRING(v))
        return JSVAL_TO_STRING(v)->length() != 0;
    if (JSVAL_IS_INT(v))
        return JSVAL_TO_INT(v) != 0;
    if (JSVAL_IS_DOUBLE(v)) {
        jsdouble d;

        d = *JSVAL_TO_DOUBLE(v);
        return !JSDOUBLE_IS_NaN(d) && d != 0;
    }
    JS_ASSERT(JSVAL_IS_BOOLEAN(v));
    return JSVAL_TO_BOOLEAN(v);
}
