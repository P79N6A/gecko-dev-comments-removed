









#include "tests.h"

static JSBool
my_convert(JSContext* context, JS::HandleObject obj, JSType type, JS::MutableHandleValue rval)
{
    if (type == JSTYPE_VOID || type == JSTYPE_STRING || type == JSTYPE_NUMBER || type == JSTYPE_BOOLEAN) {
        rval.set(JS_NumberValue(123));
        return JS_TRUE;
    }
    return JS_FALSE;
}

static JSClass myClass = {
    "MyClass",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, my_convert
};

static JSBool
createMyObject(JSContext* context, unsigned argc, jsval *vp)
{
    JS_BeginRequest(context);

    

    JSObject* myObject = JS_NewObject(context, &myClass, NULL, NULL);
    *vp = OBJECT_TO_JSVAL(myObject);

    JS_EndRequest(context);

    return JS_TRUE;
}

static JSFunctionSpec s_functions[] =
{
    JS_FN("createMyObject", createMyObject, 0, 0),
    JS_FS_END
};

BEGIN_TEST(testOps_bug559006)
{
    CHECK(JS_DefineFunctions(cx, global, s_functions));

    EXEC("function main() { while(1) return 0 + createMyObject(); }");

    for (int i = 0; i < 9; i++) {
        JS::RootedValue rval(cx);
        CHECK(JS_CallFunctionName(cx, global, "main", 0, NULL, rval.address()));
        CHECK_SAME(rval, INT_TO_JSVAL(123));
    }
    return true;
}
END_TEST(testOps_bug559006)

