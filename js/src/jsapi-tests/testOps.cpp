








#include "jsapi-tests/tests.h"

static bool
my_convert(JSContext* context, JS::HandleObject obj, JSType type, JS::MutableHandleValue rval)
{
    if (type == JSTYPE_VOID || type == JSTYPE_STRING || type == JSTYPE_NUMBER || type == JSTYPE_BOOLEAN) {
        rval.set(JS_NumberValue(123));
        return true;
    }
    return false;
}

static const JSClass myClass = {
    "MyClass",
    0,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, my_convert
};

static bool
createMyObject(JSContext* context, unsigned argc, jsval* vp)
{
    JS_BeginRequest(context);

    

    JSObject* myObject = JS_NewObject(context, &myClass);
    *vp = OBJECT_TO_JSVAL(myObject);

    JS_EndRequest(context);

    return true;
}

static const JSFunctionSpec s_functions[] =
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
        CHECK(JS_CallFunctionName(cx, global, "main", JS::HandleValueArray::empty(),
                                  &rval));
        CHECK_SAME(rval, INT_TO_JSVAL(123));
    }
    return true;
}
END_TEST(testOps_bug559006)

