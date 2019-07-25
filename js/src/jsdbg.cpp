





































#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

using namespace js;

static bool
NotImplemented(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "API");
    return false;
}

static JSBool
Debug(JSContext *cx, uintN argc, Value *vp)
{
    return NotImplemented(cx);
}

static Class DebugClass = {
    "Debug", 0,
    PropertyStub, PropertyStub, PropertyStub, StrictPropertyStub,
    EnumerateStub, ResolveStub, ConvertStub
};

extern JS_PUBLIC_API(JSBool)
JS_DefineDebugObject(JSContext *cx, JSObject *obj)
{
    JSObject *objProto;
    if (!js_GetClassPrototype(cx, obj, JSProto_Object, &objProto))
        return NULL;

    return !!js_InitClass(cx, obj, objProto, &DebugClass, Debug, 1,
                          NULL, NULL, NULL, NULL);
}
