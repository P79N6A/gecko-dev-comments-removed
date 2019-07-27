






#include "jsapi-tests/tests.h"

static int g_counter;

static bool
CounterAdd(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp)
{
    g_counter++;
    return true;
}

static const JSClass CounterClass = {
    "Counter",  
    0,  
    CounterAdd
};

BEGIN_TEST(testPropCache_bug505798)
{
    g_counter = 0;
    EXEC("var x = {};");
    CHECK(JS_DefineObject(cx, global, "y", &CounterClass, JSPROP_ENUMERATE));
    EXEC("var arr = [x, y];\n"
         "for (var i = 0; i < arr.length; i++)\n"
         "    arr[i].p = 1;\n");
    CHECK_EQUAL(g_counter, 1);
    return true;
}
END_TEST(testPropCache_bug505798)
