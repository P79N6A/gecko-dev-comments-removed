






#include "jsapi-tests/tests.h"

BEGIN_TEST(testNullRoot)
{
    JSObject *obj = nullptr;
    CHECK(JS_AddObjectRoot(cx, &obj));

    JSString *str = nullptr;
    CHECK(JS_AddStringRoot(cx, &str));

    JSScript *scr = nullptr;
    CHECK(JS_AddNamedScriptRoot(cx, &scr, "testNullRoot's scr"));

    
    JS_GC(cx->runtime());

    JS_RemoveObjectRoot(cx, &obj);
    JS_RemoveStringRoot(cx, &str);
    JS_RemoveScriptRoot(cx, &scr);
    return true;
}
END_TEST(testNullRoot)
