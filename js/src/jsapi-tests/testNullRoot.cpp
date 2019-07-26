






#include "jsapi-tests/tests.h"

BEGIN_TEST(testNullRoot)
{
    JS::RootedObject obj(cx);
    CHECK(JS_AddObjectRoot(cx, obj.address()));

    JS::RootedString str(cx);
    CHECK(JS_AddStringRoot(cx, str.address()));

    JS::RootedScript script(cx);
    CHECK(JS_AddNamedScriptRoot(cx, script.address(), "testNullRoot's script"));

    
    JS_GC(cx->runtime());

    JS_RemoveObjectRoot(cx, obj.address());
    JS_RemoveStringRoot(cx, str.address());
    JS_RemoveScriptRoot(cx, script.address());
    return true;
}
END_TEST(testNullRoot)
