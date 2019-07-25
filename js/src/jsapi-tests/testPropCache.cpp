



#include "tests.h"

static int g_counter;

static JSBool
CounterAdd(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    g_counter++;
    return JS_TRUE;
}

static JSClass CounterClass = {
    "Counter",  
    0,  
    CounterAdd, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

BEGIN_TEST(testPropCache_bug505798)
{
    g_counter = 0;
    EXEC("var x = {};");
    CHECK(JS_DefineObject(cx, global, "y", &CounterClass, NULL, JSPROP_ENUMERATE));
    EXEC("var arr = [x, y];\n"
         "for (var i = 0; i < arr.length; i++)\n"
         "    arr[i].p = 1;\n");
    CHECK_EQUAL(g_counter, 1);
    return true;
}
END_TEST(testPropCache_bug505798)
