









































#include "jsstddef.h"
#include "jstypes.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"

JSClass js_BooleanClass = {
    "Boolean",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_Boolean),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub,
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
                js_boolean_strs[JSVAL_TO_BOOLEAN(v) ? 1 : 0]);
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
    JS_FN(js_toSource_str,  bool_toSource,  0, 0, JSFUN_THISP_BOOLEAN,0),
#endif
    JS_FN(js_toString_str,  bool_toString,  0, 0, JSFUN_THISP_BOOLEAN,0),
    JS_FN(js_valueOf_str,   bool_valueOf,   0, 0, JSFUN_THISP_BOOLEAN,0),
    JS_FS_END
};

static JSBool
Boolean(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool b;
    jsval bval;

    if (argc != 0) {
        if (!js_ValueToBoolean(cx, argv[0], &b))
            return JS_FALSE;
        bval = BOOLEAN_TO_JSVAL(b);
    } else {
        bval = JSVAL_FALSE;
    }
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        *rval = bval;
        return JS_TRUE;
    }
    OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, bval);
    return JS_TRUE;
}

JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;

    proto = JS_InitClass(cx, obj, NULL, &js_BooleanClass, Boolean, 1,
                        NULL, boolean_methods, NULL, NULL);
    if (!proto)
        return NULL;
    OBJ_SET_SLOT(cx, proto, JSSLOT_PRIVATE, JSVAL_FALSE);
    return proto;
}

JSString *
js_BooleanToString(JSContext *cx, JSBool b)
{
    return ATOM_TO_STRING(cx->runtime->atomState.booleanAtoms[b ? 1 : 0]);
}

JSBool
js_ValueToBoolean(JSContext *cx, jsval v, JSBool *bp)
{
    JSBool b;
    jsdouble d;

    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v)) {
        b = JS_FALSE;
    } else if (JSVAL_IS_OBJECT(v)) {
        b = JS_TRUE;
    } else if (JSVAL_IS_STRING(v)) {
        b = JSSTRING_LENGTH(JSVAL_TO_STRING(v)) ? JS_TRUE : JS_FALSE;
    } else if (JSVAL_IS_INT(v)) {
        b = JSVAL_TO_INT(v) ? JS_TRUE : JS_FALSE;
    } else if (JSVAL_IS_DOUBLE(v)) {
        d = *JSVAL_TO_DOUBLE(v);
        b = (!JSDOUBLE_IS_NaN(d) && d != 0) ? JS_TRUE : JS_FALSE;
    } else {
        JS_ASSERT(JSVAL_IS_BOOLEAN(v));
        b = JSVAL_TO_BOOLEAN(v);
    }

    *bp = b;
    return JS_TRUE;
}
